/*
 * Copyright (c) 2023 Rockchip, Inc. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#if defined(UI_RKADK_ENGINE)

#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <lvgl.h>
#include "disp.h"

#include <rkadk_media_comm.h>
#include <rkadk_ui.h>

#if defined(UI_USE_RGA_CPP) || defined(UI_USE_RGA)
#include "lvgl/common/lv_rga.h"
#endif

/**********************
 *      MACROS
 **********************/
#define USE_DOUBLE_BUF

#define DRM_CARD          "/dev/dri/card0"
#define DRM_CONNECTOR_ID  -1	/* -1 for the first connected one */

#define DBG_TAG "drm"

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

#define print(msg, ...)    fprintf(stderr, msg, ##__VA_ARGS__);
#define err(msg, ...)  print("error: " msg "\n", ##__VA_ARGS__)
#define info(msg, ...) print(msg "\n", ##__VA_ARGS__)
#define dbg(msg, ...)  {} //print(DBG_TAG ": " msg "\n", ##__VA_ARGS__)

/**********************
 *  GLOBAL PROTOTYPES
 **********************/

/**********************
 *  GLOBAL VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
struct disp_buffer {
    RKADK_UI_FRAME_INFO frame_info;
    unsigned long int size;
    int32_t fd;
    void *map;
};

struct disp_dev {
    RKADK_MW_PTR ui_ptr;
    lv_disp_rot_t rot;
#ifdef USE_DOUBLE_BUF
    struct disp_buffer *cur_bufs[2]; /* double buffering handling */
    struct disp_buffer bufs[2];
#else
    struct disp_buffer bufs[1];
#endif
    uint32_t width, height;
    uint32_t mm_width, mm_height;
} disp_dev;

/**********************
 *  DETECT OSD 合成送显 (供 main_app 调用)
 *
 * 显示链路为 RKADK_UI(SPLICE_MODE_RGA): UI 帧由 RGA 拼到视频上送显,
 * RGN OVERLAY 挂 VPSS 通道在该链路上不生效(disp 画面出不了框)。
 * 因此检测框改走本路径: main_app 每次推理后调 rk_ui_osd_commit() 交付
 * 整幅 ARGB8888 画布(透明=不遮挡), 每次 RKADK_UI_Update 送显前把画布
 * 合成进 UI 帧; LVGL 自身缓冲不写入 OSD 像素, 不会产生移动框残影。
 **********************/
static pthread_mutex_t g_osd_lock = PTHREAD_MUTEX_INITIALIZER;
static const uint32_t *g_osd_canvas;
static uint32_t g_osd_w, g_osd_h;
static struct disp_buffer g_osd_present;

/* 画布非透明像素覆盖 dst(UI 帧), 透明处保留 dst 内容 */
static void rk_ui_osd_blend(uint32_t *dst) {
    const uint32_t *osd = g_osd_canvas;
    uint32_t rows = g_osd_h < disp_dev.height ? g_osd_h : disp_dev.height;
    uint32_t cols = g_osd_w < disp_dev.width ? g_osd_w : disp_dev.width;
    uint32_t x, y;

    if (!osd || !dst)
        return;
    for (y = 0; y < rows; y++) {
        const uint32_t *src = osd + (size_t)y * g_osd_w;
        uint32_t *d = dst + (size_t)y * disp_dev.width;

        for (x = 0; x < cols; x++)
            if (src[x] >> 24)
                d[x] = src[x];
    }
}

/* 把 base(UI 帧) 拷入送显缓冲, 合成 OSD 后送显; base 为空则只送 OSD */
static int rk_ui_present(struct disp_buffer *base) {
    int ret;

    if (!disp_dev.ui_ptr)
        return -1;
    if (!g_osd_present.map) {
        RKADK_FORMAT_E format = -1;
        void *blk = NULL;
        uint32_t size = disp_dev.width * disp_dev.height * (LV_COLOR_SIZE / 8);

        if (LV_COLOR_DEPTH == 32)
            format = RKADK_FMT_BGRA8888;
        else {
            err("osd present: unsupported color depth\n");
            return -1;
        }
        if (0 != RK_MPI_MMZ_Alloc(&blk, size, RK_MMZ_ALLOC_CACHEABLE)) {
            err("osd present: alloc failed\n");
            return -1;
        }
        g_osd_present.frame_info.Format = format;
        g_osd_present.frame_info.pMblk = blk;
        g_osd_present.frame_info.u32Width = disp_dev.width;
        g_osd_present.frame_info.u32Height = disp_dev.height;
        g_osd_present.size = size;
        g_osd_present.map = RK_MPI_MMZ_Handle2VirAddr(blk);
        g_osd_present.fd = RK_MPI_MMZ_Handle2Fd(blk);
        memset(g_osd_present.map, 0x00, size);
    }

    pthread_mutex_lock(&g_osd_lock);
    if (base && base->map)
        memcpy(g_osd_present.map, base->map, g_osd_present.size);
    else
        memset(g_osd_present.map, 0x00, g_osd_present.size);
    rk_ui_osd_blend((uint32_t *)g_osd_present.map);
    RK_MPI_SYS_MmzFlushCache(g_osd_present.frame_info.pMblk, RK_FALSE);
    ret = RKADK_UI_Update(disp_dev.ui_ptr, &(g_osd_present.frame_info));
    pthread_mutex_unlock(&g_osd_lock);
    return ret;
}

/* main_app 每次推理后调用: 交付新画布并立即送显一帧(UI+OSD) */
int rk_ui_osd_commit(const void *canvas, uint32_t width, uint32_t height) {
    struct disp_buffer *base = NULL;

    if (!disp_dev.ui_ptr || !canvas)
        return -1;
    pthread_mutex_lock(&g_osd_lock);
    g_osd_canvas = (const uint32_t *)canvas;
    g_osd_w = width;
    g_osd_h = height;
    pthread_mutex_unlock(&g_osd_lock);

    /* 取最近一次 LVGL 送出的 UI 帧做底(该帧刷新间隙不被 LVGL 写) */
#ifdef USE_DOUBLE_BUF
    base = disp_dev.cur_bufs[0];
#else
    base = &disp_dev.bufs[0];
#endif
    return rk_ui_present(base);
}

/**********************
 *  STATIC VARIABLES
 **********************/


/**********************
 *   STATIC FUNCTIONS
 **********************/
static int32_t drm_open(const char *path) {
    int32_t fd, flags;
    uint64_t has_dumb;
    int32_t ret;

    fd = open(path, O_RDWR);
    if (fd < 0) {
        err("cannot open \"%s\"", path);
        return -1;
    }

    /* set FD_CLOEXEC flag */
    if ((flags = fcntl(fd, F_GETFD)) < 0 ||
         fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0) {
        err("fcntl FD_CLOEXEC failed");
        goto err;
    }

    /* check capability */
    ret = drmGetCap(fd, DRM_CAP_DUMB_BUFFER, &has_dumb);
    if (ret < 0 || has_dumb == 0) {
        err("drmGetCap DRM_CAP_DUMB_BUFFER failed or \"%s\" doesn't have dumb "
            "buffer", path);
        goto err;
    }

    return fd;
err:
    close(fd);
    return -1;
}

static int drm_find_connector(int32_t dev_fd) {
    drmModeConnector *conn = NULL;
    drmModeRes *res = NULL;
    int32_t i;

    if ((res = drmModeGetResources(dev_fd)) == NULL) {
        err("drmModeGetResources() failed");
        return -1;
    }

    if (res->count_crtcs <= 0) {
        err("no Crtcs");
        goto free_res;
    }

    /* find all available connectors */
    for (i = 0; i < res->count_connectors; i++) {
        conn = drmModeGetConnector(dev_fd, res->connectors[i]);
        if (!conn)
            continue;

#if DRM_CONNECTOR_ID >= 0
        if (conn->connector_id != DRM_CONNECTOR_ID) {
            drmModeFreeConnector(conn);
            continue;
        }
#endif

        if (conn->connection == DRM_MODE_CONNECTED) {
            dbg("drm: connector %d: connected", conn->connector_id);
        } else if (conn->connection == DRM_MODE_DISCONNECTED) {
            dbg("drm: connector %d: disconnected", conn->connector_id);
        } else if (conn->connection == DRM_MODE_UNKNOWNCONNECTION) {
            dbg("drm: connector %d: unknownconnection", conn->connector_id);
        } else {
            dbg("drm: connector %d: unknown", conn->connector_id);
        }

        if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0)
            break;

        drmModeFreeConnector(conn);
        conn = NULL;
    };

    if (!conn) {
        err("suitable connector not found");
        goto free_res;
    }

    disp_dev.width = conn->modes[0].hdisplay;
    disp_dev.height = conn->modes[0].vdisplay;
    disp_dev.mm_width = conn->mmWidth;
    disp_dev.mm_height = conn->mmHeight;

    return 0;

free_res:
    drmModeFreeResources(res);

    return -1;
}

static int32_t get_disp_info(void) {
    int32_t dev_fd, ret;
    const char *device_path = NULL;

    device_path = getenv("DRM_CARD");
    if (!device_path)
        device_path = DRM_CARD;

    dev_fd = drm_open(device_path);
    if (dev_fd < 0)
        return -1;

    ret = drm_find_connector(dev_fd);
    if (ret) {
        err("available drm devices not found");
        close(dev_fd);
        return -1;
    }

    close(dev_fd);

    info("rk_disp: %dx%d (%dmm X% dmm)",
         disp_dev.width, disp_dev.height, disp_dev.mm_width, disp_dev.mm_height);

    return 0;
}

static int32_t rk_disp_setup(void) {
    int32_t ret;
    RKADK_UI_ATTR_S ui_attr;

    memset(&ui_attr, 0, sizeof(ui_attr));

    ui_attr.u32DispFrmRt = 30;
    ui_attr.u32DispWidth = disp_dev.width;
    ui_attr.u32DispHeight = disp_dev.height;
    ui_attr.u32ImgWidth = disp_dev.width;
    ui_attr.u32ImgHeight = disp_dev.height;
    ui_attr.enUiVoFormat = VO_FORMAT_RGB888;
    ui_attr.enVoSpliceMode = SPLICE_MODE_RGA;
    //ui_attr.enUiVoFormat = VO_FORMAT_BGRA8888; 
    //ui_attr.enUiVoFormat = VO_FORMAT_ARGB8888;
    //ui_attr.enVoSpliceMode = SPLICE_MODE_BYPASS;
#if defined(UI_VO_INTF_MIPI)
    ui_attr.enUiVoIntfTye = DISPLAY_TYPE_MIPI;
#else
    ui_attr.enUiVoIntfTye = DISPLAY_TYPE_DEFAULT;
#endif
    ui_attr.u32VoDev = 0;
    ui_attr.u32VoLay = 0;
    ui_attr.u32VoChn = 1;

    ret = RKADK_UI_Create(&ui_attr, &disp_dev.ui_ptr);
    if (0 != ret) {
        err("RKADK_DISP_Init failed(%d)", ret);
        return -1;
    }

    info("rk_disp: ui created successfullyl.");

    return 0;
}

static void rk_disp_teardown(void) {
    if (NULL == disp_dev.ui_ptr)
        return;

    int32_t ret = RKADK_UI_Destroy(disp_dev.ui_ptr);
    if (0 != ret)
        err("RKADK_DISP_Init failed(%d)", ret);

    disp_dev.ui_ptr = NULL;

    info("rk_disp: ui destroyed successfully.");
}

static int32_t rk_disp_setup_buffers(void) {
    int32_t ret;
    uint32_t i, size;
    void *blk = NULL;
    RKADK_FORMAT_E format;

    if (LV_COLOR_DEPTH == 32) {
        format = RKADK_FMT_BGRA8888;
    }else {
        format = -1;
        err("drm_flush rga not supported format\n");
        return -1;
    }
    
    size = disp_dev.width * disp_dev.height * (LV_COLOR_SIZE / 8);
    for (i = 0; i < sizeof(disp_dev.bufs) / sizeof(disp_dev.bufs[0]); i++) {
        ret = RK_MPI_MMZ_Alloc(&blk, size, RK_MMZ_ALLOC_CACHEABLE);
        if (0 != ret) {
            err("alloc failed!");
            break;
        }

        disp_dev.bufs[i].frame_info.Format = format;
        disp_dev.bufs[i].frame_info.pMblk = blk;
        disp_dev.bufs[i].frame_info.u32Width = disp_dev.width;
        disp_dev.bufs[i].frame_info.u32Height = disp_dev.height;
        disp_dev.bufs[i].size = size;
        disp_dev.bufs[i].map = RK_MPI_MMZ_Handle2VirAddr(blk);
        disp_dev.bufs[i].fd = RK_MPI_MMZ_Handle2Fd(blk);

        memset(disp_dev.bufs[i].map, 0x00, size);

        info("rk_disp: ui bufs[%u] application is successful.", i);
    }

#ifdef USE_DOUBLE_BUF
    /* Set buffering handling */
    disp_dev.cur_bufs[0] = NULL;
    disp_dev.cur_bufs[1] = &disp_dev.bufs[0];
#endif

    return 0;
}

static void rk_disp_teardown_buffers(void) {
    for (uint32_t i = 0; i < sizeof(disp_dev.bufs) / sizeof(disp_dev.bufs[0]); i++) {
        if (NULL != disp_dev.bufs[i].frame_info.pMblk) {
            RK_MPI_MMZ_Free(disp_dev.bufs[i].frame_info.pMblk);
            memset(&disp_dev.bufs[i], 0, sizeof(disp_dev.bufs[i]));
            info("rk_disp: ui bufs[%d] released successfully.", i);
        }
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int32_t rk_disp_init(lv_disp_rot_t rotate_disp) {
    int32_t ret;

    ret = get_disp_info();
    if (0 != ret) {
        err("get display info failed");
        return -1;
    }

    ret = rk_disp_setup();
    if (0 != ret) {
        err("rk_disp_setup failed");
        return -1;
    }

    ret = rk_disp_setup_buffers();
    if (0 != ret) {
        err("Allocating display buffer failed");
        goto err;
    }

    disp_dev.rot = rotate_disp;

    return 0;
err:
    rk_disp_teardown();

    return -1;
}

void rk_disp_exit(void) {
    rk_disp_teardown_buffers();
    pthread_mutex_lock(&g_osd_lock);
    g_osd_canvas = NULL;
    g_osd_w = g_osd_h = 0;
    pthread_mutex_unlock(&g_osd_lock);
    if (g_osd_present.frame_info.pMblk) {
        RK_MPI_MMZ_Free(g_osd_present.frame_info.pMblk);
        memset(&g_osd_present, 0, sizeof(g_osd_present));
    }
    rk_disp_teardown();
}

static void draw_buf_rotate_90(lv_color_t * color_p, const lv_area_t *area,
                               lv_color_t * dst_buf, lv_coord_t canvas_w, lv_coord_t canvas_h) {
    lv_coord_t area_w = (area->x2 - area->x1 + 1);
    lv_coord_t area_h = (area->y2 - area->y1 + 1);
    uint32_t   initial_i = area->x1 * canvas_w + (canvas_w - area->y1 - 1);
    for (lv_coord_t y = 0; y < area_h; y++) {
        uint32_t i = initial_i - y;
        for (lv_coord_t x = 0; x < area_w; x++) {
            dst_buf[i] = *(color_p++);
            i += canvas_w;
        }
    }
}

#ifdef USE_DOUBLE_BUF
#if defined(UI_USE_RGA_CPP) || defined(UI_USE_RGA)
void rk_disp_flush_by_rga(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
    int32_t format = 0;
    bool partial_update = false;
    struct disp_buffer *fbuf = disp_dev.cur_bufs[1];
    lv_coord_t w = (area->x2 - area->x1 + 1);
    lv_coord_t h = (area->y2 - area->y1 + 1);
    RKADK_UI_FRAME_INFO ui_frame_info;

    memset(&ui_frame_info, 0, sizeof(ui_frame_info));

    dbg("x %d:%d y %d:%d w %d h %d", area->x1, area->x2, area->y1, area->y2, w, h);

    if (disp_dev.rot == LV_DISP_ROT_90 || disp_dev.rot == LV_DISP_ROT_270) {
        if ((w != disp_dev.height || h != disp_dev.width) && disp_dev.cur_bufs[0])
            partial_update = true;
    } else {
        if ((w != disp_dev.width || h != disp_dev.height) && disp_dev.cur_bufs[0])
            partial_update = true;
    }

    if (LV_COLOR_DEPTH == 16) {
        format = RK_FORMAT_RGB_565;
    }else if (LV_COLOR_DEPTH == 32) {
        format = RK_FORMAT_BGRA_8888;
    }else {
        format = -1;
        printf("drm_flush rga not supported format\n");
        return;
    }

    lv_rga_info_t src;
    lv_rga_info_t dst;

    /* Partial update */
    if (true == partial_update) {
        memset(&src, 0, sizeof(src));
        memset(&dst, 0, sizeof(dst));

        src.virAddr = disp_dev.cur_bufs[0]->map;
        src.fd = disp_dev.cur_bufs[0]->fd;
        src.rect.xoffset = 0;
        src.rect.yoffset = 0;
        src.rect.width = disp_dev.width;
        src.rect.height = disp_dev.height;
        src.rect.wstride = disp_dev.width;
        src.rect.hstride = disp_dev.height;
        src.rect.format = format;

        dst.virAddr = fbuf->map;
        dst.fd = fbuf->fd;
        dst.rect.xoffset = 0;
        dst.rect.yoffset = 0;
        dst.rect.width = disp_dev.width;
        dst.rect.height = disp_dev.height;
        dst.rect.wstride = disp_dev.width;
        dst.rect.hstride = disp_dev.height;
        dst.rect.format = format;
        if (lv_rga_copy(&src, &dst, LV_RGA_TRANSFORM_ROT_NONE))
            printf("lv_rga_copy error");
    }

    memset(&src, 0, sizeof(src));
    memset(&dst, 0, sizeof(dst));

    if (disp_dev.rot == LV_DISP_ROT_90 || disp_dev.rot == LV_DISP_ROT_270) {
        if (w < 2 || h < 2) {
            draw_buf_rotate_90(color_p, area, fbuf->map, disp_dev.width, disp_dev.height);
        } else {
            src.virAddr = color_p;
            src.rect.xoffset = 0;
            src.rect.yoffset = 0;
            src.rect.width = w;
            src.rect.height = h;
            src.rect.wstride = w;
            src.rect.hstride = h;
            src.rect.format = format;

            dst.virAddr = fbuf->map;
            dst.fd = fbuf->fd;
            dst.rect.xoffset = (disp_dev.width - (area->y1 + h));
            dst.rect.yoffset = area->x1;
            dst.rect.width = h;
            dst.rect.height = w;
            dst.rect.wstride = disp_dev.width;
            dst.rect.hstride = disp_dev.height;
            dst.rect.format = format;

            if (lv_rga_copy(&src, &dst, LV_RGA_TRANSFORM_ROT_90))
                printf("lv_rga_copy error");
        }
    } else {
        if (w < 2 || h < 2) {
            for (uint32_t y = 0, i = area->y1 ; i <= area->y2 ; ++i, ++y) {
                memcpy((uint8_t *)fbuf->map + (area->x1 + disp_dev.width * i) * (LV_COLOR_SIZE/8),
                       (uint8_t *)color_p + (w * (LV_COLOR_SIZE/8) * y),
                       w * (LV_COLOR_SIZE/8));
            }
        } else {
            src.virAddr = color_p;
            src.rect.xoffset = 0;
            src.rect.yoffset = 0;
            src.rect.width = w;
            src.rect.height = h;
            src.rect.wstride = w;
            src.rect.hstride = h;
            src.rect.format = format;

            dst.virAddr = fbuf->map;
            dst.fd = fbuf->fd;
            dst.rect.xoffset = area->x1;
            dst.rect.yoffset = area->y1;
            dst.rect.width = w;
            dst.rect.height = h;
            dst.rect.wstride = disp_dev.width;
            dst.rect.hstride = disp_dev.height;
            dst.rect.format = format;

            if (lv_rga_copy(&src, &dst, LV_RGA_TRANSFORM_ROT_NONE))
                printf("lv_rga_copy error");
        }
    }

    /* show fbuf plane */
    RK_MPI_SYS_MmzFlushCache(fbuf->frame_info.pMblk, RK_FALSE);
    /* LVGL 帧 + 检测 OSD 画布一并合成送显 */
    rk_ui_present(fbuf);

    if (!disp_dev.cur_bufs[0])
        disp_dev.cur_bufs[1] = &disp_dev.bufs[1];
    else
        disp_dev.cur_bufs[1] = disp_dev.cur_bufs[0];

    disp_dev.cur_bufs[0] = fbuf;

    lv_disp_flush_ready(disp_drv);
}
#endif

void rk_disp_flush_by_cpu(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
    int32_t format = 0;
    bool partial_update = false;
    struct disp_buffer *fbuf = disp_dev.cur_bufs[1];
    lv_coord_t w = (area->x2 - area->x1 + 1);
    lv_coord_t h = (area->y2 - area->y1 + 1);
    RKADK_UI_FRAME_INFO ui_frame_info;

    memset(&ui_frame_info, 0, sizeof(ui_frame_info));

    dbg("x %d:%d y %d:%d w %d h %d", area->x1, area->x2, area->y1, area->y2, w, h);

    if (disp_dev.rot == LV_DISP_ROT_90 || disp_dev.rot == LV_DISP_ROT_270) {
        if ((w != disp_dev.height || h != disp_dev.width) && disp_dev.cur_bufs[0])
            partial_update = true;
    } else {
        if ((w != disp_dev.width || h != disp_dev.height) && disp_dev.cur_bufs[0])
            partial_update = true;
    }
    /* Partial update */
    if (true == partial_update)
        memcpy(fbuf->map, disp_dev.cur_bufs[0]->map, disp_dev.width * disp_dev.height * (LV_COLOR_SIZE/8));

    if (disp_dev.rot == LV_DISP_ROT_90 || disp_dev.rot == LV_DISP_ROT_270) {
        draw_buf_rotate_90(color_p, area, fbuf->map, disp_dev.width, disp_dev.height);
    } else {
        for (uint32_t y = 0, i = area->y1 ; i <= area->y2 ; ++i, ++y) {
            memcpy((uint8_t *)fbuf->map + (area->x1 + disp_dev.width * i) * (LV_COLOR_SIZE/8),
                    (uint8_t *)color_p + (w * (LV_COLOR_SIZE/8) * y),
                    w * (LV_COLOR_SIZE/8));
        }
    }

    /* show fbuf plane */
    RK_MPI_SYS_MmzFlushCache(fbuf->frame_info.pMblk, RK_FALSE);
    /* LVGL 帧 + 检测 OSD 画布一并合成送显 */
    rk_ui_present(fbuf);

    if (!disp_dev.cur_bufs[0])
        disp_dev.cur_bufs[1] = &disp_dev.bufs[1];
    else
        disp_dev.cur_bufs[1] = disp_dev.cur_bufs[0];

    disp_dev.cur_bufs[0] = fbuf;

    lv_disp_flush_ready(disp_drv);
}
#else

#if defined(UI_USE_RGA_CPP) || defined(UI_USE_RGA)
void rk_disp_flush_by_rga(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
    int32_t format = 0;
    struct disp_buffer *fbuf = &disp_dev.bufs[0];
    lv_coord_t w = (area->x2 - area->x1 + 1);
    lv_coord_t h = (area->y2 - area->y1 + 1);
    RKADK_UI_FRAME_INFO ui_frame_info;

    memset(&ui_frame_info, 0, sizeof(ui_frame_info));

    dbg("x %d:%d y %d:%d w %d h %d", area->x1, area->x2, area->y1, area->y2, w, h);

    if (LV_COLOR_DEPTH == 16) {
        format = RK_FORMAT_RGB_565;
    }else if (LV_COLOR_DEPTH == 32) {
        format = RK_FORMAT_BGRA_8888;
    }else {
        format = -1;
        printf("drm_flush rga not supported format\n");
        return;
    }

    lv_rga_info_t src = {0};
    lv_rga_info_t dst = {0};

    /* Partial update */
    if (disp_dev.rot == LV_DISP_ROT_90 || disp_dev.rot == LV_DISP_ROT_270) {
        if (w < 2 || h < 2) {
            draw_buf_rotate_90(color_p, area, fbuf->map, disp_dev.width, disp_dev.height);
        } else {
            src.virAddr = color_p;
            src.rect.xoffset = 0;
            src.rect.yoffset = 0;
            src.rect.width = w;
            src.rect.height = h;
            src.rect.wstride = w;
            src.rect.hstride = h;
            src.rect.format = format;

            dst.virAddr = fbuf->map;
            dst.fd = fbuf->fd;
            dst.rect.xoffset = (disp_dev.width - (area->y1 + h));
            dst.rect.yoffset = area->x1;
            dst.rect.width = h;
            dst.rect.height = w;
            dst.rect.wstride = disp_dev.width;
            dst.rect.hstride = disp_dev.height;
            dst.rect.format = format;

            if (lv_rga_copy(&src, &dst, LV_RGA_TRANSFORM_ROT_90))
                printf("lv_rga_copy error");
        }
    } else {
        if (w < 2 || h < 2) {
            for (uint32_t y = 0, i = area->y1 ; i <= area->y2 ; ++i, ++y) {
                memcpy((uint8_t *)fbuf->map + (area->x1 + disp_dev.width * i) * (LV_COLOR_SIZE/8),
                       (uint8_t *)color_p + (w * (LV_COLOR_SIZE/8) * y),
                       w * (LV_COLOR_SIZE/8));
            }
        } else {
            src.virAddr = color_p;
            src.rect.xoffset = 0;
            src.rect.yoffset = 0;
            src.rect.width = w;
            src.rect.height = h;
            src.rect.wstride = w;
            src.rect.hstride = h;
            src.rect.format = format;

            dst.virAddr = fbuf->map;
            dst.fd = fbuf->fd;
            dst.rect.xoffset = area->x1;
            dst.rect.yoffset = area->y1;
            dst.rect.width = w;
            dst.rect.height = h;
            dst.rect.wstride = disp_dev.width;
            dst.rect.hstride = disp_dev.height;
            dst.rect.format = format;
            
            if (lv_rga_copy(&src, &dst, LV_RGA_TRANSFORM_ROT_NONE))
                printf("lv_rga_copy error");
        }
    }
    
    /* show fbuf plane */
    RK_MPI_SYS_MmzFlushCache(disp_dev.bufs[0].frame_info.pMblk, RK_FALSE);
    /* LVGL 帧 + 检测 OSD 画布一并合成送显 */
    rk_ui_present(&disp_dev.bufs[0]);

    lv_disp_flush_ready(disp_drv);
}
#endif

void rk_disp_flush_by_cpu(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
    struct disp_buffer *fbuf = &disp_dev.bufs[0];
    lv_coord_t w = (area->x2 - area->x1 + 1);
    RKADK_UI_FRAME_INFO ui_frame_info;

    memset(&ui_frame_info, 0, sizeof(ui_frame_info));

    /* Partial update */
    if (disp_dev.rot == LV_DISP_ROT_90 || disp_dev.rot == LV_DISP_ROT_270) {
        draw_buf_rotate_90(color_p, area, fbuf->map, disp_dev.width, disp_dev.height);
    } else {
        for (uint32_t y = 0, i = area->y1 ; i <= area->y2 ; ++i, ++y) {
            memcpy((uint8_t *)fbuf->map + (area->x1 + disp_dev.width * i) * (LV_COLOR_SIZE/8),
                    (uint8_t *)color_p + (w * (LV_COLOR_SIZE/8) * y),
                    w * (LV_COLOR_SIZE/8));
        }
    }

    /* show fbuf plane */
    RK_MPI_SYS_MmzFlushCache(disp_dev.bufs[0].frame_info.pMblk, RK_FALSE);
    /* LVGL 帧 + 检测 OSD 画布一并合成送显 */
    rk_ui_present(&disp_dev.bufs[0]);

    lv_disp_flush_ready(disp_drv);
}
#endif

void rk_disp_get_sizes(lv_coord_t *width, lv_coord_t *height, uint32_t *dpi) {
    if (width)
        *width = disp_dev.width;

    if (height)
        *height = disp_dev.height;

    if (dpi && disp_dev.mm_width)
        *dpi = DIV_ROUND_UP(disp_dev.width * 25400, disp_dev.mm_width * 1000);

    return;
}

disp_ops_t rk_ui_ops = {
    rk_disp_init,
    rk_disp_exit,
    rk_disp_get_sizes,
#if defined(UI_USE_RGA_CPP) || defined(UI_USE_RGA)
    rk_disp_flush_by_rga,
#else
    rk_disp_flush_by_cpu,
#endif
};

#endif // UI_RKADK_ENGINE