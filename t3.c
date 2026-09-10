/* 单元测试: 从 main_app.c 提取 OSD+后处理代码, stub 掉 rknn/RGN 平台依赖 */
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum { RKNN_TENSOR_NCHW = 0, RKNN_TENSOR_NHWC = 1 } rknn_tensor_format;
typedef enum { RKNN_TENSOR_FLOAT32 = 0 } rknn_tensor_type;
typedef struct {
    int index;
    uint32_t n_dims;
    char name[32];
    uint32_t dims[16];
    rknn_tensor_format fmt;
    rknn_tensor_type type;
    float scale;
    int32_t zp;
} rknn_tensor_attr;
typedef struct {
    uint8_t want_float;
    uint8_t is_prealloc;
    uint32_t index;
    uint32_t size;
    void *buf;
} rknn_output;
typedef int rknn_context;

#define RK_SUCCESS 0
#define RK_TRUE 1
#define RK_FALSE 0
typedef uint8_t RK_U8;
typedef int RK_S32;
typedef int RK_BOOL;
typedef enum { RK_ID_VPSS = 0, RK_ID_VO = 1 } MOD_ID_E;
typedef enum { OVERLAY_RGN = 0 } RGN_TYPE_E;
typedef enum { RK_FMT_ARGB8888 = 0 } PIXEL_FORMAT_E;
typedef struct { MOD_ID_E enModId; RK_S32 s32DevId; RK_S32 s32ChnId; } MPP_CHN_S;
typedef struct { uint32_t u32Width; uint32_t u32Height; } SIZE_S;
typedef struct { RK_S32 s32X; RK_S32 s32Y; } POINT_S;
typedef struct { PIXEL_FORMAT_E enPixelFmt; uint32_t u32CanvasNum; SIZE_S stSize; } OVERLAY_ATTR_S;
typedef struct { RGN_TYPE_E enType; union { OVERLAY_ATTR_S stOverlay; } unAttr; } RGN_ATTR_S;
typedef struct { POINT_S stPoint; uint32_t u32Layer; } OVERLAY_CHN_ATTR_S;
typedef struct {
    RK_BOOL bShow;
    RGN_TYPE_E enType;
    union { OVERLAY_CHN_ATTR_S stOverlayChn; } unChnAttr;
} RGN_CHN_ATTR_S;
typedef struct { PIXEL_FORMAT_E enPixelFormat; uint32_t u32Width; uint32_t u32Height; RK_U8 *pData; } BITMAP_S;
typedef int RGN_HANDLE;

static int RK_MPI_RGN_Create(RGN_HANDLE h, RGN_ATTR_S *a) { (void)h; (void)a; return RK_SUCCESS; }
static int RK_MPI_RGN_AttachToChn(RGN_HANDLE h, MPP_CHN_S *c, RGN_CHN_ATTR_S *a) { (void)h; (void)c; (void)a; return RK_SUCCESS; }
static int RK_MPI_RGN_DetachFromChn(RGN_HANDLE h, MPP_CHN_S *c) { (void)h; (void)c; return RK_SUCCESS; }
static int RK_MPI_RGN_Destroy(RGN_HANDLE h) { (void)h; return RK_SUCCESS; }
static int RK_MPI_RGN_SetBitMap(RGN_HANDLE h, BITMAP_S *b) { (void)h; (void)b; return RK_SUCCESS; }

static volatile bool g_rknn_detect_running;
static pthread_t g_rknn_detect_thread;
static rknn_context g_rknn_detect_ctx;
static uint32_t g_rknn_input_width;
static uint32_t g_rknn_input_height;
static rknn_tensor_format g_rknn_input_format;
static uint32_t g_rknn_output_count;
static rknn_tensor_attr g_rknn_output_attrs[16];

static void rknn_dump_output_attrs(void) {
	uint32_t i;

	for (i = 0; i < g_rknn_output_count && i < 16; i++) {
		printf("RKNN output[%u]: name=%s n_dims=%u dims=", i,
		       g_rknn_output_attrs[i].index < 16 ? g_rknn_output_attrs[i].name : "unknown",
		       g_rknn_output_attrs[i].n_dims);
		for (uint32_t j = 0; j < g_rknn_output_attrs[i].n_dims; j++)
			printf("%s%u", j ? "x" : "", g_rknn_output_attrs[i].dims[j]);
		printf(" type=%d fmt=%d scale=%f zp=%d\n",
		       g_rknn_output_attrs[i].type, g_rknn_output_attrs[i].fmt,
		       g_rknn_output_attrs[i].scale, g_rknn_output_attrs[i].zp);
	}
}

/* ---------------------------------------------------------------------------
 * 推理结果 OSD 叠加到 DISP 流
 *
 * DISP 链路: vi.2(chn_id=4, 480x270 NV12) -> [display] vpss_grp=2/vpss_chn=0 -> VO。
 * 实现方式: 创建 rockit RGN OVERLAY(ARGB8888) 并挂到显示 VPSS 通道,
 * 每次推理完成后把检测框 + 类别 index/名称 重绘到位图并 RK_MPI_RGN_SetBitMap 刷新。
 * 检测框坐标从模型输入分辨率线性映射到 480x270 显示分辨率。
 * ------------------------------------------------------------------------- */
#define DISP_OSD_RGN_HANDLE     32
#define DISP_OSD_ATTACH_TARGET   0      /* 0: 挂到 VPSS(显示)  1: 挂到 VO */
#define DISP_OSD_VPSS_GRP        2      /* 与 [display] vpss_grp 一致 */
#define DISP_OSD_VPSS_CHN        0      /* 与 [display] vpss_chn 一致 */
#define DISP_OSD_VO_DEV          0      /* 与 [display] vo_device/vo_layer 一致 */
#define DISP_OSD_VO_CHN          0      /* 与 [display] vo_chn 一致 */
#define DISP_OSD_WIDTH           480    /* 与 [display] width 一致 */
#define DISP_OSD_HEIGHT          270    /* 与 [display] height 一致 */
#define DISP_OSD_LINE_THICK      2      /* 检测框线宽(像素) */
#define DISP_OSD_FONT_SCALE      2      /* 标签文字放大倍数(5x7 字体) */
#define DISP_OSD_RETRY_MS        1000   /* RGN 挂载失败后的重试间隔(ms) */

/* 后处理选择:
 *   0: 自动 (>=6 个 4 维输出按空间尺寸成组 -> 解耦头; >=3 个输出 -> yolov5 3 分支;
 *      1 个输出 -> yolov8 单输出)
 *   1: yolov5, 3 个分支输出(原始输出, 需要用 anchors 解码)
 *   2: yolov5, 单个已解码输出([1,N,5+nc], obj/cls 需 sigmoid)
 *   3: yolov8, 单个输出([1,4+nc,N], 分数已解码, 无需 sigmoid)
 *   4: 解耦头, 每个尺度层多个输出(reg DFL 64ch + cls nc ch [+ sum 1ch]), NCHW
 * 模型输出不匹配时按 rknn_dump_output_attrs() 打印的 dims 调整。 */
#define CVR_RKNN_POSTPROC        0
#define DET_MAX_BOXES            32
#define DET_CONF_THRESHOLD       0.25f
#define DET_NMS_THRESHOLD        0.45f
#define DFL_BINS                 16     /* 解耦头 DFL 回归 bins (4 边 x 16 = 64ch) */
#define DECOUPLED_USE_OBJ        1     /* 解耦头 1ch 为 score_sum(各类概率和), 仅做格子预筛; 无此分支置 0 */

typedef struct {
	float x1, y1, x2, y2; /* 模型输入像素坐标 */
	float conf;
	int cls;              /* OSD 上显示的类别 index */
} det_box_t;

static det_box_t g_det_boxes[DET_MAX_BOXES];
static uint32_t *g_disp_osd_canvas;
static bool g_disp_osd_attached;
static int64_t g_disp_osd_last_try_ms;

static const uint32_t g_osd_palette[8] = {
	0xFF00FF00, 0xFF00CCFF, 0xFFFF6600, 0xFFFFFF00,
	0xFFFF00FF, 0xFF00FFFF, 0xFFFF3333, 0xFF66FF66,
};

/* 5x7 点阵字体, 数字(用于类别 index) */
static const uint8_t g_osd_font_5x7[10][7] = {
	{0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}, /* 0 */
	{0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, /* 1 */
	{0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}, /* 2 */
	{0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E}, /* 3 */
	{0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, /* 4 */
	{0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}, /* 5 */
	{0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}, /* 6 */
	{0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, /* 7 */
	{0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, /* 8 */
	{0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}, /* 9 */
};

/* 5x7 点阵字体, 大写 A-Z(用于显示类别名称) */
static const uint8_t g_osd_font_upper[26][7] = {
	{0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, /* A */
	{0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}, /* B */
	{0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}, /* C */
	{0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}, /* D */
	{0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}, /* E */
	{0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}, /* F */
	{0x0E, 0x11, 0x10, 0x13, 0x11, 0x11, 0x0E}, /* G */
	{0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, /* H */
	{0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}, /* I */
	{0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}, /* J */
	{0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, /* K */
	{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}, /* L */
	{0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}, /* M */
	{0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}, /* N */
	{0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, /* O */
	{0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}, /* P */
	{0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}, /* Q */
	{0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}, /* R */
	{0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}, /* S */
	{0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, /* T */
	{0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, /* U */
	{0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}, /* V */
	{0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}, /* W */
	{0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}, /* X */
	{0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}, /* Y */
	{0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}, /* Z */
};

/* COCO 80 类名称, 与模型输出的类别 index 对应(OSD 上显示为大写) */
static const char *g_coco_names[80] = {
	"person", "bicycle", "car", "motorcycle", "airplane", "bus", "train",
	"truck", "boat", "traffic light", "fire hydrant", "stop sign",
	"parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
	"elephant", "bear", "zebra", "giraffe", "backpack", "umbrella",
	"handbag", "tie", "suitcase", "frisbee", "skis", "snowboard",
	"sports ball", "kite", "baseball bat", "baseball glove", "skateboard",
	"surfboard", "tennis racket", "bottle", "wine glass", "cup", "fork",
	"knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange",
	"broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair",
	"couch", "potted plant", "bed", "dining table", "toilet", "tv",
	"laptop", "mouse", "remote", "keyboard", "cell phone", "microwave",
	"oven", "toaster", "sink", "refrigerator", "book", "clock", "vase",
	"scissors", "teddy bear", "hair drier", "toothbrush",
};

/* 取字符点阵: 数字/大写字母, 其余(空格等)返回 NULL */
static const uint8_t *osd_font_glyph(char c) {
	if (c >= '0' && c <= '9')
		return g_osd_font_5x7[c - '0'];
	if (c >= 'A' && c <= 'Z')
		return g_osd_font_upper[c - 'A'];
	return NULL;
}

static int64_t osd_now_ms(void) {
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* 在 ARGB8888 画布上填充矩形(自动裁剪到画布范围) */
static void osd_fill_rect(int x, int y, int w, int h, uint32_t argb) {
	int x0 = x < 0 ? 0 : x;
	int y0 = y < 0 ? 0 : y;
	int x1 = x + w;
	int y1 = y + h;

	if (x1 > DISP_OSD_WIDTH)
		x1 = DISP_OSD_WIDTH;
	if (y1 > DISP_OSD_HEIGHT)
		y1 = DISP_OSD_HEIGHT;
	for (int j = y0; j < y1; j++) {
		uint32_t *row = g_disp_osd_canvas + (size_t)j * DISP_OSD_WIDTH;
		for (int i = x0; i < x1; i++)
			row[i] = argb;
	}
}

/* 画检测框(四边, 线宽 DISP_OSD_LINE_THICK) */
static void osd_draw_box(int x1, int y1, int x2, int y2, uint32_t color) {
	int t = DISP_OSD_LINE_THICK;

	osd_fill_rect(x1, y1, x2 - x1, t, color);     /* 上 */
	osd_fill_rect(x1, y2 - t, x2 - x1, t, color); /* 下 */
	osd_fill_rect(x1, y1, t, y2 - y1, color);     /* 左 */
	osd_fill_rect(x2 - t, y1, t, y2 - y1, color); /* 右 */
}

/* 在 (x,y) 处绘制标签文本(带半透明黑色背景), 支持 0-9/A-Z/空格 */
static void osd_draw_label(int x, int y, const char *text, uint32_t color) {
	int s = DISP_OSD_FONT_SCALE;
	int pad = 2;
	int len = (int)strlen(text);
	int w, cx;

	if (len <= 0)
		return;
	w = len * 5 * s + (len - 1) * s + pad * 2;
	/* 右边界钳位, 尽量避免标签被裁掉 */
	if (x + w > DISP_OSD_WIDTH)
		x = DISP_OSD_WIDTH - w;
	if (x < 0)
		x = 0;
	osd_fill_rect(x, y, w, 7 * s + pad * 2, 0xA0000000);
	cx = x + pad;
	for (int i = 0; i < len; i++) {
		const uint8_t *glyph = osd_font_glyph(text[i]);
		if (glyph) {
			for (int gy = 0; gy < 7; gy++) {
				for (int gx = 0; gx < 5; gx++) {
					if (glyph[gy] & (0x10 >> gx))
						osd_fill_rect(cx + gx * s, y + pad + gy * s, s, s, color);
				}
			}
		}
		cx += 5 * s + s;
	}
}

/* 绘制 "类别index 类别名" 标签, 名称转大写; 超出名称表范围只显示 index */
static void osd_draw_class_label(int x, int y, int cls, uint32_t color) {
	char label[32];
	char *p;

	if (cls >= 0 && cls < (int)(sizeof(g_coco_names) / sizeof(g_coco_names[0])))
		snprintf(label, sizeof(label), "%d %s", cls, g_coco_names[cls]);
	else
		snprintf(label, sizeof(label), "%d", cls);
	for (p = label; *p; p++) {
		if (*p >= 'a' && *p <= 'z')
			*p = (char)(*p - 'a' + 'A');
	}
	osd_draw_label(x, y, label, color);
}

/* ------------------------- 模型输出后处理 ------------------------- */

static float sigmoid_f(float x) {
	return 1.0f / (1.0f + expf(-x));
}

static float clampf(float v, float lo, float hi) {
	return v < lo ? lo : (v > hi ? hi : v);
}

static float box_iou(const det_box_t *a, const det_box_t *b) {
	float iw = fminf(a->x2, b->x2) - fmaxf(a->x1, b->x1);
	float ih = fminf(a->y2, b->y2) - fmaxf(a->y1, b->y1);
	float inter, union_area;

	if (iw <= 0.0f || ih <= 0.0f)
		return 0.0f;
	inter = iw * ih;
	union_area = (a->x2 - a->x1) * (a->y2 - a->y1) +
		     (b->x2 - b->x1) * (b->y2 - b->y1) - inter;
	return inter / (union_area + 1e-9f);
}

/* 按置信度降序做贪心 NMS */
static void det_nms(det_box_t *boxes, int *count) {
	int n = *count;
	int keep[DET_MAX_BOXES];
	int m = 0, i, j, k;

	for (i = 1; i < n; i++) {
		det_box_t key = boxes[i];
		j = i - 1;
		while (j >= 0 && boxes[j].conf < key.conf) {
			boxes[j + 1] = boxes[j];
			j--;
		}
		boxes[j + 1] = key;
	}
	for (i = 0; i < n; i++) {
		bool suppressed = false;
		for (k = 0; k < m; k++) {
			if (box_iou(&boxes[i], &boxes[keep[k]]) > DET_NMS_THRESHOLD) {
				suppressed = true;
				break;
			}
		}
		if (!suppressed)
			keep[m++] = i;
	}
	for (i = 0; i < m; i++)
		boxes[i] = boxes[keep[i]];
	*count = m;
}

static const int yolov5_anchors[3][6] = {
	{10, 13, 16, 30, 33, 23},
	{30, 61, 62, 45, 59, 119},
	{116, 90, 156, 198, 373, 326},
};

/* yolov5 单个分支输出解码, 支持 NCHW/NHWC 布局 */
static int yolov5_branch_decode(const float *data, const rknn_tensor_attr *attr,
								int box_cnt) {
	uint32_t H, W, C, stride_w, stride_h, plane;
	int nc, level, a, c, h, w;
	bool nchw;

	if (attr->n_dims < 4)
		return box_cnt;
	if (attr->fmt == RKNN_TENSOR_NCHW) {
		C = attr->dims[1];
		H = attr->dims[2];
		W = attr->dims[3];
	} else {
		H = attr->dims[1];
		W = attr->dims[2];
		C = attr->dims[3];
	}
	if (C < 18 || (C % 3) != 0)
		return box_cnt;
	nc = C / 3 - 5;
	nchw = (attr->fmt == RKNN_TENSOR_NCHW);
	plane = H * W;
	stride_w = W ? g_rknn_input_width / W : 0;
	stride_h = H ? g_rknn_input_height / H : 0;
	if (nc < 1 || !stride_w || !stride_h)
		return box_cnt;
	level = stride_w >= 32 ? 2 : (stride_w >= 16 ? 1 : 0);

	for (h = 0; h < (int)H && box_cnt < DET_MAX_BOXES; h++) {
		for (w = 0; w < (int)W && box_cnt < DET_MAX_BOXES; w++) {
			for (a = 0; a < 3; a++) {
#define Y5_VAL(j)                                                            \
	(nchw ? data[(size_t)(a * (5 + nc) + (j)) * plane + (size_t)(h * W + w)] \
	      : data[(size_t)(h * W + w) * C + a * (5 + nc) + (j)])
				float obj = sigmoid_f(Y5_VAL(4));
				float best = 0.0f;
				int best_cls = 0;
				float cx, cy, bw, bh;

				for (c = 0; c < nc; c++) {
					float score = sigmoid_f(Y5_VAL(5 + c)) * obj;
					if (score > best) {
						best = score;
						best_cls = c;
					}
				}
				if (best <= DET_CONF_THRESHOLD)
					continue;
				cx = (sigmoid_f(Y5_VAL(0)) * 2.0f - 0.5f + w) * stride_w;
				cy = (sigmoid_f(Y5_VAL(1)) * 2.0f - 0.5f + h) * stride_h;
				bw = powf(sigmoid_f(Y5_VAL(2)) * 2.0f, 2.0f) *
				     yolov5_anchors[level][a * 2];
				bh = powf(sigmoid_f(Y5_VAL(3)) * 2.0f, 2.0f) *
				     yolov5_anchors[level][a * 2 + 1];
				g_det_boxes[box_cnt].x1 = cx - bw / 2;
				g_det_boxes[box_cnt].y1 = cy - bh / 2;
				g_det_boxes[box_cnt].x2 = cx + bw / 2;
				g_det_boxes[box_cnt].y2 = cy + bh / 2;
				g_det_boxes[box_cnt].conf = best;
				g_det_boxes[box_cnt].cls = best_cls;
				box_cnt++;
			}
		}
	}
#undef Y5_VAL
	return box_cnt;
}

/* 单输出解码: yolov5([1,N,5+nc] 已解码框) 或 yolov8([1,4+nc,N] 分数已解码) */
static int yolo_single_decode(const float *data, const rknn_tensor_attr *attr,
							  int box_cnt, bool yolov8) {
	uint32_t d1, d2, rows, len;
	int nc, base, r, c;
	bool chn_major;

	if (attr->n_dims < 3)
		return box_cnt;
	d1 = attr->dims[1];
	d2 = attr->dims[2];
	rows = d1 > d2 ? d1 : d2;
	len = d1 > d2 ? d2 : d1;
	/* [1,len,rows](如 yolov8 [1,84,8400])为通道在前, [1,rows,len](如 yolov5 [1,25200,85])为行在前 */
	chn_major = (d1 < d2);
	nc = (int)len - (yolov8 ? 4 : 5);
	base = yolov8 ? 4 : 5;
	if (nc < 1)
		return box_cnt;

	for (r = 0; r < (int)rows && box_cnt < DET_MAX_BOXES; r++) {
		float cx, cy, bw, bh, best = 0.0f;
		int best_cls = 0;

#define YS_VAL(j) (chn_major ? data[(size_t)(j) * rows + r] : data[(size_t)r * len + (j)])
		if (yolov8) {
			for (c = 0; c < nc; c++) {
				if (YS_VAL(base + c) > best) {
					best = YS_VAL(base + c);
					best_cls = c;
				}
			}
		} else {
			float obj = sigmoid_f(YS_VAL(4));
			for (c = 0; c < nc; c++) {
				float score = sigmoid_f(YS_VAL(5 + c)) * obj;
				if (score > best) {
					best = score;
					best_cls = c;
				}
			}
		}
		if (best <= DET_CONF_THRESHOLD)
			continue;
		cx = YS_VAL(0);
		cy = YS_VAL(1);
		bw = YS_VAL(2);
		bh = YS_VAL(3);
		g_det_boxes[box_cnt].x1 = cx - bw / 2;
		g_det_boxes[box_cnt].y1 = cy - bh / 2;
		g_det_boxes[box_cnt].x2 = cx + bw / 2;
		g_det_boxes[box_cnt].y2 = cy + bh / 2;
		g_det_boxes[box_cnt].conf = best;
		g_det_boxes[box_cnt].cls = best_cls;
		box_cnt++;
	}
#undef YS_VAL
	return box_cnt;
}

/* ---------------- 解耦头后处理 (9 输出: 每层 reg DFL 64ch + cls nc ch + sum 1ch) -----
 * 输出按空间尺寸成组(80x80/40x40/20x20, 对应 stride 8/16/32), 每组 NCHW:
 *   reg: 1x64xHxW, DFL 分布(4 边 x 16 bins), softmax 取期望得到格子为单位的 l/t/r/b
 *   cls: 1xncxHxW, 类别概率(量化 zp=-128, 模型已做 sigmoid, 勿再 sigmoid)
 *   sum: 1x1xHxW,  score_sum(各类概率之和, RKOPT 导出的加速分支), 仅做格子预筛
 * conf = max_cls(与官方 postprocess.cc 一致); 勿乘 score_sum, 否则中等置信度框被压掉
 * anchor 在格子中心: x1=(col+0.5-l)*stride ... (yolov6/v8 风格)
 * ------------------------------------------------------------------------- */

/* DFL: 16 个 bin 的 logit 经 softmax 后取期望值(相邻 bin 间隔 stride 个 float) */
static float dfl_expect(const float *p, size_t stride) {
	float e[DFL_BINS], sum = 0.0f, acc = 0.0f;
	int i;

	for (i = 0; i < DFL_BINS; i++) {
		e[i] = expf(p[(size_t)i * stride]);
		sum += e[i];
	}
	for (i = 0; i < DFL_BINS; i++)
		acc += e[i] * (float)i;
	return acc / (sum + 1e-9f);
}

/* 判断输出结构是否为解耦头: >=3 组同尺寸的 4 维 NCHW 输出, 每组含 64ch reg 和 nc ch cls */
static bool rknn_is_decoupled_head(uint32_t n_out) {
	uint32_t i = 0;
	int groups = 0;

	if (n_out < 6 || n_out > 16)
		return false;
	while (i < n_out) {
		uint32_t h = g_rknn_output_attrs[i].dims[2];
		uint32_t w = g_rknn_output_attrs[i].dims[3];
		bool has_reg = false, has_cls = false;
		uint32_t j = i;

		if (g_rknn_output_attrs[i].n_dims != 4 ||
			g_rknn_output_attrs[i].dims[0] != 1 ||
			g_rknn_output_attrs[i].fmt != RKNN_TENSOR_NCHW)
			return false;
		while (j < n_out && g_rknn_output_attrs[j].n_dims == 4 &&
			   g_rknn_output_attrs[j].fmt == RKNN_TENSOR_NCHW &&
			   g_rknn_output_attrs[j].dims[2] == h &&
			   g_rknn_output_attrs[j].dims[3] == w) {
			uint32_t c = g_rknn_output_attrs[j].dims[1];

			if (c == DFL_BINS * 4)
				has_reg = true;
			else if (c != 1)
				has_cls = true;
			j++;
		}
		if (!has_reg || !has_cls || j == i)
			return false;
		groups++;
		i = j;
	}
	return groups >= 3;
}

/* 解耦头解码: 按空间尺寸分组, 组内按通道数区分 reg(64)/cls(nc)/obj(1) */
static int yolo_decoupled_decode(const rknn_output *outputs, uint32_t n_out,
								 int box_cnt) {
	uint32_t i = 0;

	while (i < n_out) {
		uint32_t h = g_rknn_output_attrs[i].dims[2];
		uint32_t w = g_rknn_output_attrs[i].dims[3];
		uint32_t j = i, k, nc = 0;
		const float *reg = NULL, *cls = NULL, *obj = NULL;
		float stride_w, stride_h;

		while (j < n_out && g_rknn_output_attrs[j].dims[2] == h &&
			   g_rknn_output_attrs[j].dims[3] == w) {
			uint32_t c = g_rknn_output_attrs[j].dims[1];

			if (c == DFL_BINS * 4) {
				reg = (const float *)outputs[j].buf;
			} else if (c == 1) {
				if (DECOUPLED_USE_OBJ)
					obj = (const float *)outputs[j].buf;
			} else {
				cls = (const float *)outputs[j].buf;
				nc = c;
			}
			j++;
		}
		if (!reg || !cls)
			return box_cnt;
		stride_w = w ? (float)g_rknn_input_width / w : 0.0f;
		stride_h = h ? (float)g_rknn_input_height / h : 0.0f;
		if (stride_w < 1.0f || stride_h < 1.0f)
			return box_cnt;
		for (uint32_t y = 0; y < h && box_cnt < DET_MAX_BOXES; y++) {
			for (uint32_t x = 0; x < w && box_cnt < DET_MAX_BOXES; x++) {
				size_t off = (size_t)y * w + x;
				size_t plane = (size_t)h * w;
				float best = 0.0f;
				int best_cls = 0;

				/* score_sum 预筛(官方 postprocess.cc 语义): 分数和低于阈值整格跳过 */
				if (obj && obj[off] <= DET_CONF_THRESHOLD)
					continue;
				for (k = 0; k < nc; k++) {
					/* cls 输出已是 sigmoid 概率(量化 zp=-128, 值域[0,0.77]), 直接用 */
					float score = cls[(size_t)k * plane + off];

					if (score > best) {
						best = score;
						best_cls = (int)k;
					}
				}
				/* conf = max_cls; 勿乘 score_sum(它是各类分之和, 相乘会压掉中等置信度框) */
				if (best <= DET_CONF_THRESHOLD)
					continue;
				{
					float l = dfl_expect(reg + off, plane);
					float t = dfl_expect(reg + DFL_BINS * plane + off, plane);
					float r = dfl_expect(reg + 2 * DFL_BINS * plane + off, plane);
					float b = dfl_expect(reg + 3 * DFL_BINS * plane + off, plane);

					g_det_boxes[box_cnt].x1 = ((float)x + 0.5f - l) * stride_w;
					g_det_boxes[box_cnt].y1 = ((float)y + 0.5f - t) * stride_h;
					g_det_boxes[box_cnt].x2 = ((float)x + 0.5f + r) * stride_w;
					g_det_boxes[box_cnt].y2 = ((float)y + 0.5f + b) * stride_h;
					g_det_boxes[box_cnt].conf = best;
					g_det_boxes[box_cnt].cls = best_cls;
					box_cnt++;
				}
			}
		}
		i = j;
	}
	return box_cnt;
}

/* 解析全部输出 -> g_det_boxes, 返回框个数 */
static int rknn_postprocess_outputs(const rknn_output *outputs, uint32_t n_out) {
	int mode = CVR_RKNN_POSTPROC;
	int n = 0, i;

	if (mode == 0)
		mode = rknn_is_decoupled_head(n_out) ? 4 :
			   (n_out >= 3 ? 1 : 3);

	switch (mode) {
	case 1:
		if (n_out >= 3) {
			for (i = 0; i < 3; i++)
				n = yolov5_branch_decode((const float *)outputs[i].buf,
										 &g_rknn_output_attrs[i], n);
		} else {
			printf("RKNN postproc: yolov5 3-branch needs 3 outputs, got %u\n", n_out);
		}
		break;
	case 2:
	case 3:
		if (n_out >= 1)
			n = yolo_single_decode((const float *)outputs[0].buf,
								   &g_rknn_output_attrs[0], n, mode == 3);
		break;
	case 4:
		if (rknn_is_decoupled_head(n_out))
			n = yolo_decoupled_decode(outputs, n_out, 0);
		else
			printf("RKNN postproc: decoupled head structure mismatch, got %u outputs\n",
				   n_out);
		break;
	}

	for (i = 0; i < n; i++) {
		g_det_boxes[i].x1 = clampf(g_det_boxes[i].x1, 0.0f, (float)g_rknn_input_width);
		g_det_boxes[i].y1 = clampf(g_det_boxes[i].y1, 0.0f, (float)g_rknn_input_height);
		g_det_boxes[i].x2 = clampf(g_det_boxes[i].x2, 0.0f, (float)g_rknn_input_width);
		g_det_boxes[i].y2 = clampf(g_det_boxes[i].y2, 0.0f, (float)g_rknn_input_height);
	}
	if (n > 1)
		det_nms(g_det_boxes, &n);
	return n;
}

/* --------------------- DISP 流 RGN OSD 挂载与刷新 --------------------- */

static void disp_osd_get_chn(MPP_CHN_S *stChn) {
	memset(stChn, 0, sizeof(*stChn));
#if DISP_OSD_ATTACH_TARGET == 1
	stChn->enModId = RK_ID_VO;
	stChn->s32DevId = DISP_OSD_VO_DEV;
	stChn->s32ChnId = DISP_OSD_VO_CHN;
#else
	stChn->enModId = RK_ID_VPSS;
	stChn->s32DevId = DISP_OSD_VPSS_GRP;
	stChn->s32ChnId = DISP_OSD_VPSS_CHN;
#endif
}

/* 创建 OVERLAY 区域并挂到显示通道; 失败返回 -1(稍后重试) */
static int disp_osd_attach(void) {
	RGN_ATTR_S stRegion;
	MPP_CHN_S stChn;
	RGN_CHN_ATTR_S stChnAttr;
	int ret;

	if (!g_disp_osd_canvas) {
		g_disp_osd_canvas = malloc((size_t)DISP_OSD_WIDTH * DISP_OSD_HEIGHT * 4);
		if (!g_disp_osd_canvas)
			return -1;
		memset(g_disp_osd_canvas, 0, (size_t)DISP_OSD_WIDTH * DISP_OSD_HEIGHT * 4);
	}

	memset(&stRegion, 0, sizeof(stRegion));
	stRegion.enType = OVERLAY_RGN;
	stRegion.unAttr.stOverlay.enPixelFmt = RK_FMT_ARGB8888;
	stRegion.unAttr.stOverlay.u32CanvasNum = 2;
	stRegion.unAttr.stOverlay.stSize.u32Width = DISP_OSD_WIDTH;
	stRegion.unAttr.stOverlay.stSize.u32Height = DISP_OSD_HEIGHT;
	ret = RK_MPI_RGN_Create(DISP_OSD_RGN_HANDLE, &stRegion);
	if (ret != RK_SUCCESS) {
		printf("disp osd: RK_MPI_RGN_Create failed %#x\n", ret);
		return -1;
	}

	disp_osd_get_chn(&stChn);
	memset(&stChnAttr, 0, sizeof(stChnAttr));
	stChnAttr.bShow = RK_TRUE;
	stChnAttr.enType = OVERLAY_RGN;
	stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = 0;
	stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = 0;
	stChnAttr.unChnAttr.stOverlayChn.u32Layer = 0;
	ret = RK_MPI_RGN_AttachToChn(DISP_OSD_RGN_HANDLE, &stChn, &stChnAttr);
	if (ret != RK_SUCCESS) {
		printf("disp osd: attach to %s %d chn %d failed %#x\n",
#if DISP_OSD_ATTACH_TARGET == 1
			   "VO", DISP_OSD_VO_DEV, DISP_OSD_VO_CHN,
#else
			   "VPSS grp", DISP_OSD_VPSS_GRP, DISP_OSD_VPSS_CHN,
#endif
			   ret);
		RK_MPI_RGN_Destroy(DISP_OSD_RGN_HANDLE);
		return -1;
	}

	g_disp_osd_attached = true;
	printf("disp osd: OVERLAY_RGN %d attached, canvas %dx%d\n",
		   DISP_OSD_RGN_HANDLE, DISP_OSD_WIDTH, DISP_OSD_HEIGHT);
	return 0;
}

static void disp_osd_detach(void) {
	MPP_CHN_S stChn;

	if (!g_disp_osd_attached)
		return;
	disp_osd_get_chn(&stChn);
	RK_MPI_RGN_DetachFromChn(DISP_OSD_RGN_HANDLE, &stChn);
	RK_MPI_RGN_Destroy(DISP_OSD_RGN_HANDLE);
	g_disp_osd_attached = false;
}

/* 把检测框画到画布并刷新 RGN 位图(框坐标由模型分辨率映射到显示分辨率) */
static void disp_osd_show(const det_box_t *boxes, int n) {
	BITMAP_S stBitmap;
	float sx = (float)DISP_OSD_WIDTH / (float)g_rknn_input_width;
	float sy = (float)DISP_OSD_HEIGHT / (float)g_rknn_input_height;
	int ret;

	if (!g_disp_osd_attached) {
		int64_t now = osd_now_ms();

		if (now - g_disp_osd_last_try_ms < DISP_OSD_RETRY_MS)
			return;
		g_disp_osd_last_try_ms = now;
		if (disp_osd_attach() != 0)
			return;
	}

	memset(g_disp_osd_canvas, 0, (size_t)DISP_OSD_WIDTH * DISP_OSD_HEIGHT * 4);
	for (int i = 0; i < n; i++) {
		int x1 = (int)(boxes[i].x1 * sx);
		int y1 = (int)(boxes[i].y1 * sy);
		int x2 = (int)(boxes[i].x2 * sx);
		int y2 = (int)(boxes[i].y2 * sy);
		uint32_t color = g_osd_palette[boxes[i].cls & 7];

		if (x2 - x1 < 2 || y2 - y1 < 2)
			continue;
		osd_draw_box(x1, y1, x2, y2, color);
		osd_draw_class_label(x1 + DISP_OSD_LINE_THICK, y1 + DISP_OSD_LINE_THICK,
							 boxes[i].cls, color);
	}

	memset(&stBitmap, 0, sizeof(stBitmap));
	stBitmap.enPixelFormat = RK_FMT_ARGB8888;
	stBitmap.u32Width = DISP_OSD_WIDTH;
	stBitmap.u32Height = DISP_OSD_HEIGHT;
	stBitmap.pData = (RK_U8 *)g_disp_osd_canvas;
	ret = RK_MPI_RGN_SetBitMap(DISP_OSD_RGN_HANDLE, &stBitmap);
	if (ret != RK_SUCCESS) {
		printf("disp osd: RK_MPI_RGN_SetBitMap failed %#x, will re-attach\n", ret);
		disp_osd_detach();
	}
}


/* ============================ 测试 ============================ */
static int g_fails;

static int check(int ok, const char *name) {
    printf("%-50s %s\n", name, ok ? "PASS" : "FAIL");
    if (!ok) g_fails++;
    return !ok;
}

static void osd_reset(void) {
    if (g_disp_osd_attached) disp_osd_detach();
    free(g_disp_osd_canvas);
    g_disp_osd_canvas = NULL;
    g_disp_osd_attached = false;
    g_disp_osd_last_try_ms = 0;
}

int main(void) {
    int fails = 0;
    int n;

    g_rknn_input_width = 640;
    g_rknn_input_height = 640;

    /* ---- yolov5 3 分支解码 (NCHW 1x24x20x20, nc=3, stride 32) ---- */
    {
        static float d[24 * 400];
        rknn_tensor_attr a;
        size_t plane = 400, off = 10 * 20 + 10;

        memset(&a, 0, sizeof(a));
        a.n_dims = 4; a.fmt = RKNN_TENSOR_NCHW;
        a.dims[0] = 1; a.dims[1] = 24; a.dims[2] = 20; a.dims[3] = 20;
        memset(d, 0, sizeof(d));
        /* anchor a=1: 通道 8..15; obj=ch12, cls2=ch15 均置 3 */
        d[12 * plane + off] = 3.0f;
        d[15 * plane + off] = 3.0f;
        n = yolov5_branch_decode(d, &a, 0);
        fails += check(n == 1, "yolov5 branch: 1 box decoded");
        if (n == 1) {
            fails += check(g_det_boxes[0].cls == 2, "yolov5 branch: class idx");
            fails += check(g_det_boxes[0].conf > 0.9f && g_det_boxes[0].conf < 0.92f,
                           "yolov5 branch: confidence");
            fails += check(fabsf(g_det_boxes[0].x1 - 258.0f) < 0.5f &&
                           fabsf(g_det_boxes[0].x2 - 414.0f) < 0.5f &&
                           fabsf(g_det_boxes[0].y1 - 237.0f) < 0.5f &&
                           fabsf(g_det_boxes[0].y2 - 435.0f) < 0.5f,
                           "yolov5 branch: box coords/anchors");
        }
    }

    /* ---- auto: 3 输出走 yolov5 分支 ---- */
    {
        static float d0[24 * 6400], d1[24 * 1600], d2[24 * 400];
        rknn_output outs[3];
        size_t plane = 400, off = 10 * 20 + 10;

        memset(d0, 0, sizeof(d0)); memset(d1, 0, sizeof(d1)); memset(d2, 0, sizeof(d2));
        for (int i = 0; i < 3; i++) {
            int hw = i == 0 ? 80 : (i == 1 ? 40 : 20);
            g_rknn_output_attrs[i].n_dims = 4;
            g_rknn_output_attrs[i].fmt = RKNN_TENSOR_NCHW;
            g_rknn_output_attrs[i].dims[0] = 1;
            g_rknn_output_attrs[i].dims[1] = 24;
            g_rknn_output_attrs[i].dims[2] = hw;
            g_rknn_output_attrs[i].dims[3] = hw;
        }
        d2[12 * plane + off] = 3.0f;
        d2[15 * plane + off] = 3.0f;
        outs[0].buf = d0; outs[1].buf = d1; outs[2].buf = d2;
        n = rknn_postprocess_outputs(outs, 3);
        fails += check(n == 1, "postproc auto 3-out path works");
    }

    /* ---- yolov8 单输出 [1,24,8400] chn-major, nc=20 ---- */
    {
        static float d[24 * 8400];

        memset(d, 0, sizeof(d));
        g_rknn_output_attrs[0].n_dims = 3;
        g_rknn_output_attrs[0].fmt = RKNN_TENSOR_NHWC;
        g_rknn_output_attrs[0].dims[0] = 1;
        g_rknn_output_attrs[0].dims[1] = 24;
        g_rknn_output_attrs[0].dims[2] = 8400;
        d[0 * 8400 + 5] = 100; d[1 * 8400 + 5] = 50;
        d[2 * 8400 + 5] = 200; d[3 * 8400 + 5] = 100;
        d[6 * 8400 + 5] = 0.9f;
        n = yolo_single_decode(d, &g_rknn_output_attrs[0], 0, true);
        fails += check(n == 1, "yolov8 single: 1 box");
        if (n == 1) {
            fails += check(g_det_boxes[0].cls == 2, "yolov8 single: class idx");
            fails += check(fabsf(g_det_boxes[0].x1) < 0.01f && fabsf(g_det_boxes[0].y1) < 0.01f &&
                           fabsf(g_det_boxes[0].x2 - 200.0f) < 0.01f &&
                           fabsf(g_det_boxes[0].y2 - 100.0f) < 0.01f,
                           "yolov8 single: box coords");
        }
    }

    /* ---- yolov5 单输出 [1,25200,8] row-major, nc=3 ---- */
    {
        static float d[25200 * 8];

        memset(d, 0, sizeof(d));
        g_rknn_output_attrs[0].n_dims = 3;
        g_rknn_output_attrs[0].fmt = RKNN_TENSOR_NHWC;
        g_rknn_output_attrs[0].dims[0] = 1;
        g_rknn_output_attrs[0].dims[1] = 25200;
        g_rknn_output_attrs[0].dims[2] = 8;
        {
            float *row = d + 7 * 8;
            row[0] = 200; row[1] = 100; row[2] = 80; row[3] = 60;
            row[4] = 3.0f; row[6] = 4.0f;
        }
        n = yolo_single_decode(d, &g_rknn_output_attrs[0], 0, false);
        fails += check(n == 1, "yolov5 single(v5): 1 box");
        if (n == 1) {
            fails += check(g_det_boxes[0].cls == 1, "yolov5 single(v5): class idx");
            fails += check(fabsf(g_det_boxes[0].x1 - 160.0f) < 0.01f &&
                           fabsf(g_det_boxes[0].y1 - 70.0f) < 0.01f &&
                           fabsf(g_det_boxes[0].x2 - 240.0f) < 0.01f &&
                           fabsf(g_det_boxes[0].y2 - 130.0f) < 0.01f,
                           "yolov5 single(v5): box coords");
        }
    }

    /* ---- NMS ---- */
    {
        det_box_t bs[3] = {{0, 0, 100, 100, 0.9f, 0},
                           {10, 10, 110, 110, 0.8f, 1},
                           {200, 200, 300, 300, 0.7f, 2}};
        int cnt = 3;

        fails += check(box_iou(&bs[0], &bs[1]) > 0.45f, "NMS: iou(overlap) > thresh");
        det_nms(bs, &cnt);
        fails += check(cnt == 2, "NMS: overlapping box suppressed");
        fails += check(bs[0].conf == 0.9f && bs[1].conf == 0.7f, "NMS: sorted by confidence");
    }

    /* ---- 解耦头 9 输出: auto 识别 + DFL 解码 (设备实际模型) ---- */
    {
        rknn_output outs[9];
        float *bufs[9];
        const int chs[3] = {64, 80, 1};
        const int hw[3] = {80, 40, 20};

        for (int lv = 0; lv < 3; lv++) {
            for (int k = 0; k < 3; k++) {
                int idx = lv * 3 + k;
                size_t sz = (size_t)chs[k] * hw[lv] * hw[lv];

                bufs[idx] = calloc(sz, sizeof(float));
                outs[idx].buf = bufs[idx];
                g_rknn_output_attrs[idx].n_dims = 4;
                g_rknn_output_attrs[idx].fmt = RKNN_TENSOR_NCHW;
                g_rknn_output_attrs[idx].dims[0] = 1;
                g_rknn_output_attrs[idx].dims[1] = chs[k];
                g_rknn_output_attrs[idx].dims[2] = hw[lv];
                g_rknn_output_attrs[idx].dims[3] = hw[lv];
            }
        }
        fails += check(rknn_is_decoupled_head(9), "decoupled: 9-out structure detected");
        fails += check(!rknn_is_decoupled_head(3), "decoupled: 3-out rejected");
        fails += check(!rknn_is_decoupled_head(1), "decoupled: 1-out rejected");

        /* level0(80x80, s=8) cell(10,10): l=2,t=1,r=3,b=4; cls=5 概率; score_sum */
        {
            float *reg = bufs[0], *cls = bufs[1], *obj = bufs[2];
            size_t plane = 6400, off = 10 * 80 + 10;

            reg[2 * plane + off] = 8.0f;   /* l -> bin2 */
            reg[17 * plane + off] = 8.0f;  /* t -> bin1 */
            reg[35 * plane + off] = 8.0f;  /* r -> bin3 */
            reg[52 * plane + off] = 8.0f;  /* b -> bin4 */
            cls[5 * plane + off] = 0.92f;
            obj[off] = 0.85f;
        }
        /* level2(20x20, s=32) cell(5,5): l=t=1, r=b=2; cls=79 概率; score_sum */
        {
            float *reg = bufs[6], *cls = bufs[7], *obj = bufs[8];
            size_t plane = 400, off = 5 * 20 + 5;

            reg[1 * plane + off] = 8.0f;
            reg[17 * plane + off] = 8.0f;
            reg[34 * plane + off] = 8.0f;
            reg[50 * plane + off] = 8.0f;
            cls[79 * plane + off] = 0.90f;
            obj[off] = 0.80f;
        }
        /* level1(40x40, s=16) cell(30,30): ltrb=1; cls=0 概率 0.45, sum=0.42
         * 官方语义 conf=0.45>0.25 保留; 旧 cls×sum 乘法 0.189<0.25 会误杀 */
        {
            float *reg = bufs[3], *cls = bufs[4], *obj = bufs[5];
            size_t plane = 1600, off = 30 * 40 + 30;

            reg[1 * plane + off] = 8.0f;
            reg[17 * plane + off] = 8.0f;
            reg[33 * plane + off] = 8.0f;
            reg[49 * plane + off] = 8.0f;
            cls[0 * plane + off] = 0.45f;
            obj[off] = 0.42f;
        }
        g_rknn_output_count = 9;
        rknn_dump_output_attrs();
        n = rknn_postprocess_outputs(outs, 9);
        fails += check(n == 3, "decoupled: auto path decodes 3 boxes");
        if (n >= 3) {
            det_box_t *b0 = &g_det_boxes[0], *b1 = &g_det_boxes[1], *b2 = &g_det_boxes[2];

            fails += check(b0->cls == 5, "decoupled: level0 class idx");
            fails += check(fabsf(b0->conf - 0.92f) < 0.02f,
                           "decoupled: level0 conf = max_cls(官方语义, sum 仅预筛)");
            fails += check(fabsf(b0->x1 - 67.8f) < 1.5f && fabsf(b0->y1 - 75.7f) < 1.5f &&
                           fabsf(b0->x2 - 108.2f) < 1.5f && fabsf(b0->y2 - 116.2f) < 1.5f,
                           "decoupled: level0 box coords(DFL)");
            fails += check(b1->cls == 79, "decoupled: level2 class idx");
            fails += check(fabsf(b1->x1 - 142.9f) < 1.5f && fabsf(b1->y1 - 142.9f) < 1.5f &&
                           fabsf(b1->x2 - 240.9f) < 1.5f && fabsf(b1->y2 - 240.9f) < 1.5f,
                           "decoupled: level2 box coords(DFL)");
            fails += check(b2->cls == 0 && fabsf(b2->conf - 0.45f) < 0.02f,
                           "decoupled: 中等置信度框保留(旧 cls×sum 乘法会误杀)");
            /* OSD: 5 BUS / 79 TOOTHBRUSH / 0 PERSON */
            disp_osd_show(g_det_boxes, n);
            {
                uint32_t lit = 0;

                for (int i = 0; i < 480 * 270; i++)
                    if (g_disp_osd_canvas[i])
                        lit++;
                fails += check(lit > 0, "decoupled: osd pixels drawn");
            }
        }
        osd_reset();
        for (int i = 0; i < 9; i++)
            free(bufs[i]);
    }

    /* ---- 噪声抑制(设备实测场景): 背景原始分 ~0.09, 不得再过 0.25 阈值 ----
     * 旧代码对已是概率的 cls/sum 再 sigmoid: 0.09 -> 0.52, 0.52*0.52 ~ 0.28
     * 满屏噪声框; 修复后 score_sum 预筛 0.09 < 0.25 整格跳过, 全部滤除 */
    {
        rknn_output outs[9];
        float *bufs[9];
        const int chs[3] = {64, 80, 1};
        const int hw[3] = {80, 40, 20};

        for (int lv = 0; lv < 3; lv++) {
            for (int k = 0; k < 3; k++) {
                int idx = lv * 3 + k;
                size_t sz = (size_t)chs[k] * hw[lv] * hw[lv];

                bufs[idx] = malloc(sz * sizeof(float));
                outs[idx].buf = bufs[idx];
                g_rknn_output_attrs[idx].n_dims = 4;
                g_rknn_output_attrs[idx].fmt = RKNN_TENSOR_NCHW;
                g_rknn_output_attrs[idx].dims[0] = 1;
                g_rknn_output_attrs[idx].dims[1] = chs[k];
                g_rknn_output_attrs[idx].dims[2] = hw[lv];
                g_rknn_output_attrs[idx].dims[3] = hw[lv];
            }
        }
        /* 全图背景: cls 全部 0.09, sum 全部 0.09(设备日志反量化典型背景值) */
        for (size_t i = 0; i < (size_t)80 * 80 * 80; i++)
            ((float *)bufs[1])[i] = 0.09f;
        for (size_t i = 0; i < (size_t)80 * 80; i++)
            ((float *)bufs[2])[i] = 0.09f;
        for (size_t i = 0; i < (size_t)80 * 40 * 40; i++)
            ((float *)bufs[4])[i] = 0.09f;
        for (size_t i = 0; i < (size_t)40 * 40; i++)
            ((float *)bufs[5])[i] = 0.09f;
        for (size_t i = 0; i < (size_t)80 * 20 * 20; i++)
            ((float *)bufs[7])[i] = 0.09f;
        for (size_t i = 0; i < (size_t)20 * 20; i++)
            ((float *)bufs[8])[i] = 0.09f;
        n = rknn_postprocess_outputs(outs, 9);
        fails += check(n == 0, "noise: 背景 0.09 全滤除(旧双 sigmoid 会出满屏 0.28 框)");
        for (int i = 0; i < 9; i++)
            free(bufs[i]);
    }

    /* ---- 解耦头 6 输出(无 obj, yolov6 zoo 风格) ---- */
    {
        rknn_output outs[6];
        float *bufs[6];
        const int hw[3] = {80, 40, 20};

        for (int lv = 0; lv < 3; lv++) {
            for (int k = 0; k < 2; k++) {
                int idx = lv * 2 + k;
                int ch = k == 0 ? 64 : 80;
                size_t sz = (size_t)ch * hw[lv] * hw[lv];

                bufs[idx] = malloc(sz * sizeof(float));
                if (k == 0) {
                    memset(bufs[idx], 0, sz * sizeof(float));
                } else {
                    for (size_t i = 0; i < sz; i++)
                        ((float *)bufs[idx])[i] = -4.0f;
                }
                outs[idx].buf = bufs[idx];
                g_rknn_output_attrs[idx].n_dims = 4;
                g_rknn_output_attrs[idx].fmt = RKNN_TENSOR_NCHW;
                g_rknn_output_attrs[idx].dims[0] = 1;
                g_rknn_output_attrs[idx].dims[1] = ch;
                g_rknn_output_attrs[idx].dims[2] = hw[lv];
                g_rknn_output_attrs[idx].dims[3] = hw[lv];
            }
        }
        /* level1(40x40, s=16) cell(20,20): l=t=r=b=2; cls=10 */
        {
            float *reg = bufs[2], *cls = bufs[3];
            size_t plane = 1600, off = 20 * 40 + 20;

            reg[2 * plane + off] = 8.0f;
            reg[18 * plane + off] = 8.0f;
            reg[34 * plane + off] = 8.0f;
            reg[50 * plane + off] = 8.0f;
            cls[10 * plane + off] = 0.88f;
        }
        n = rknn_postprocess_outputs(outs, 6);
        fails += check(n == 1, "decoupled 6-out(no obj): 1 box");
        if (n == 1) {
            fails += check(g_det_boxes[0].cls == 10, "decoupled 6-out: class idx");
            fails += check(fabsf(g_det_boxes[0].x1 - 295.5f) < 1.5f &&
                           fabsf(g_det_boxes[0].x2 - 360.5f) < 1.5f,
                           "decoupled 6-out: box coords");
        }
        for (int i = 0; i < 6; i++)
            free(bufs[i]);
    }

    /* ---- OSD 基础绘制/裁剪 + 类别标签 ---- */
    {
        fails += check((int)(sizeof(g_coco_names) / sizeof(g_coco_names[0])) == 80,
                       "coco names: 80 entries");
        fails += check(strcmp(g_coco_names[0], "person") == 0 &&
                       strcmp(g_coco_names[9], "traffic light") == 0 &&
                       strcmp(g_coco_names[35], "baseball glove") == 0 &&
                       strcmp(g_coco_names[79], "toothbrush") == 0,
                       "coco names: spot check");
        {
            det_box_t bs[2] = {{10, 10, 600, 600, 0.9f, 0}, {620, 320, 640, 640, 0.8f, 9}};

            disp_osd_show(bs, 2);
            uint32_t lit = 0;
            for (int i = 0; i < 480 * 270; i++)
                if (g_disp_osd_canvas[i])
                    lit++;
            fails += check(lit > 0, "osd: pixels drawn (0 PERSON / 9 TRAFFIC LIGHT)");
        }
        {
            det_box_t bad[1] = {{-50, -50, 100, 100, 0.9f, 79}};

            disp_osd_show(bad, 1);
            fails += check(1, "osd: clip out-of-range box (no crash)");
        }
        memset(g_disp_osd_canvas, 0, (size_t)480 * 270 * 4);
        osd_draw_class_label(10, 10, 9, 0xFF00FF00);
        {
            uint32_t lit = 0;

            for (int i = 0; i < 480 * 270; i++)
                if (g_disp_osd_canvas[i] == 0xFF00FF00)
                    lit++;
            fails += check(lit > 400, "osd: text glyph pixels rendered");
        }
        osd_draw_class_label(10, 10, 99, 0xFF00FF00);
        fails += check(1, "osd: out-of-range cls fallback");
        osd_reset();
    }

    /* ---- rk_ui.c OSD 画布合成语义(UI 帧为底, 画布透明不遮挡) ---- */
    {
        /* 与 rk_ui.c rk_ui_osd_blend 相同的算法: 非透明覆盖, 透明保留底层 */
        static uint32_t ui[64 * 32], canvas[64 * 32];
        struct { uint32_t w, h; } dd = {64, 32}; /* 模拟 disp_dev */
        uint32_t *g_osd_canvas_t = canvas;
        uint32_t g_osd_w = 64, g_osd_h = 32;

        for (int i = 0; i < 64 * 32; i++) {
            ui[i] = 0x80112233;      /* 半透明 UI 底色 */
            canvas[i] = 0x00000000;  /* 全透明画布 */
        }
        canvas[0] = 0xFF00FF00;       /* 左上角一个不透明 OSD 像素 */
        canvas[64 * 31 + 63] = 0xFFFF0000; /* 右下角 */
#define BLEND_T(dst) do { \
        const uint32_t *osd = g_osd_canvas_t; \
        uint32_t rows = g_osd_h < dd.h ? g_osd_h : dd.h; \
        uint32_t cols = g_osd_w < dd.w ? g_osd_w : dd.w; \
        for (uint32_t y = 0; y < rows; y++) { \
            const uint32_t *src = osd + (size_t)y * g_osd_w; \
            uint32_t *d = (dst) + (size_t)y * dd.w; \
            for (uint32_t x = 0; x < cols; x++) \
                if (src[x] >> 24) \
                    d[x] = src[x]; \
        } \
    } while (0)
        BLEND_T(ui);
        fails += check(ui[0] == 0xFF00FF00, "ui blend: 不透明画布像素覆盖 UI");
        fails += check(ui[64 * 31 + 63] == 0xFFFF0000, "ui blend: 右下角覆盖");
        fails += check(ui[5] == 0x80112233 && ui[1000] == 0x80112233,
                       "ui blend: 透明画布像素保留 UI 底(不遮挡)");
        /* 画布比屏幕大: 只合成重叠区, 不越界 */
        {
            uint32_t big[70 * 40];

            for (int i = 0; i < 70 * 40; i++)
                big[i] = 0xFF0000FF;
            g_osd_canvas_t = big;
            g_osd_w = 70;
            g_osd_h = 40;
            for (int i = 0; i < 64 * 32; i++)
                ui[i] = 0x00000000;
            BLEND_T(ui);
            fails += check(ui[64 * 32 - 1] == 0xFF0000FF && ui[0] == 0xFF0000FF,
                           "ui blend: 画布大于屏幕时按屏幕范围裁剪");
        }
#undef BLEND_T
    }

    printf("%s\n", g_fails ? "== FAIL ==" : "== ALL PASS ==");
    return g_fails ? 1 : 0;
}
