// /*
//  * Copyright (c) 2022 Rockchip, Inc. All Rights Reserved.
//  *
//  *  Licensed under the Apache License, Version 2.0 (the "License");
//  *  you may not use this file except in compliance with the License.
//  *  You may obtain a copy of the License at
//  *
//  *     http://www.apache.org/licenses/LICENSE-2.0
//  *
//  *  Unless required by applicable law or agreed to in writing, software
//  *  distributed under the License is distributed on an "AS IS" BASIS,
//  *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  *  See the License for the specific language governing permissions and
//  *  limitations under the License.
//  */

// #include "utils/log.h"
// #include "core/key_event.h"
// #include "core/linux_netlink.h"
// #include "core/sys.h"

// #include "mpu/photo/mpu_photo.h"
// #include "mpu/record/mpu_record.h"
// #include "mpu/storage/mpu_storage.h"
// #include "mpu/rtsp/mpu_rtsp.h"
// #include "net/cvr_net.h"

// #include "app/remote_config.h"
// #include <stdatomic.h>
// #include <stdbool.h>
// #include <pthread.h>
// #include <fcntl.h>
// #include <stdio.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/time.h>
// #include <sys/prctl.h>          // 提供 prctl() 和 PR_SET_NAME
// #include "rk_mpi_ivs.h"         // 提供 IVS_RESULT_INFO_S 等类型
// #include "rk_mpi_vpss.h"
// #include "rk_mpi_sys.h"
// #include "rk_gpio.h"
// #include "rk_pwm.h"
// #include "rk_smart_ir_api.h"
// #include "rk_aiq_user_api2_camgroup.h"
// #include "rk_aiq_user_api2_sysctl.h"
// #include "mpu/isp/rv1126b/sample_isp.h"

// #ifndef FULL_COLOR_MODE
// #define FULL_COLOR_MODE 1
// #endif

// struct smart_ir_ctx {
//     bool thread_is_quit;
//     pthread_t thread_id;
//     rk_smart_ir_result_t result;
//     RK_SMART_IR_STATUS_t last_status;
//     rk_smart_ir_ctx_t *ir_ctx;
//     rk_aiq_sys_ctx_t *aiq_ctx;
//     int night_mode;
//     int ircut_on_gpio;
//     int ircut_off_gpio;
//     int irled_pwm_channel;
//     int irled_enable_gpio;
//     int visled_pwm_channel;
//     int visled_enable_gpio;
//     int cur_irled_value;
// };

// /**********************
//  *  STATIC PROTOTYPES
//  **********************/

// /**********************
//  *  STATIC VARIABLES
//  **********************/

// /**********************
//  *      MACROS
//  **********************/

// /**********************
//  *   STATIC FUNCTIONS
//  **********************/
// // 在 main_app.c 文件顶部添加头文件
// #include "ui_thumb.h"
// //#include "ui/common/ui_page_manager.h"

// // 添加外部声明
// //extern lv_obj_t *ui_get_current_page_obj(void);  // 假设有这个函数获取当前页面对象


// //extern int rk_isp_af_zoom_change(int cam_id, int change);
// // 修改 button_app_storage_format 函数
// static void button_app_storage_format1(void) {
//     //cvr_start_ap();
//     //user_request_stop(10);
//     //rk_isp_af_zoom_change(0,30);
//     // lv_obj_t *parent_obj = lv_scr_act();
//     //rk_isp_af_zoom_change(0,20);
// 	extern int rk_enable_usb_function(void);
// 	rk_enable_usb_function();
    
// }

// static void button_app_photo(void){
//     // mpu_photo_init();
//     // mpu_photo_burst_capture(0,20);
//     // sleep(2);
//     // mpu_photo_deinit();
//     // mpu_rtsp_init();
//     // mpu_rtsp_start(0);
//     //rk_isp_af_zoom_change(0,-30);
//     //rk_isp_af_zoom_change(0,-20);
// 	mpu_photo_deinit();
// 	//mpu_photo_set_resolution(0,RKADK_RES_4320P);
// 	RKADK_PARAM_PHOTO_CFG_S *pstPhotoCfg = RKADK_PARAM_GetPhotoCfg(0);
// 	pstPhotoCfg->jpeg_slice = TRUE;
// 	pstPhotoCfg->image_width = 8192;
//     pstPhotoCfg->image_height = 4608;
// 	pstPhotoCfg->slice_height = 4608;
// 	mpu_photo_init();
// 	// mpu_photo_capture(0);
// }

// static void button_app_set(void){
// 	//mpu_record_deinit();
// 	// mpu_record_set_fps(0,RKADK_STREAM_TYPE_VIDEO_MAIN,25);
// 	// mpu_record_set_gop(0,RKADK_STREAM_TYPE_VIDEO_MAIN,25);
// 	// mpu_record_reset(0);
// 	// mpu_record_init();
// 	//mpu_record_set_resolution(0,RKADK_RES_1440P);
// 	// mpu_record_reset(0);
// 	// mpu_record_init();
// 	mpu_photo_capture(0);
// }
// extern int CVR_SetSensorMode(RKADK_U32 cam_id,
//                       uint32_t width,
//                       uint32_t height,
//                       uint32_t mbus_code,
//                       rk_aiq_working_mode_t hdr_mode);

// static void button_app_menu(void){
// 	/* 挂起 cvr_net worker 的 RTSP 自动重建，防止 sensor/ISP 重配期间
// 	 * VI 通道被重新使能(MIPI 错误、VENC 超时) */
// 	cvr_net_suspend();
// 	mpu_rtsp_deinit();
// 	mpu_record_deinit();
// 	mpu_photo_deinit();
// 	mpu_disp_deinit();
// 	//mpu_storage_deinit();
// 	usleep(200);
// 	CVR_SetSensorMode(0,1944,1097, 0x3010,0);
// 	//CVR_SetSensorMode(0,1920,1080, 0x3010,0);
// 	//CVR_SetSensorMode(0,2560,1440, 0x300e,0);
// 	//CVR_SetSensorMode(0,3864,2192, 0x300e,RK_AIQ_WORKING_MODE_ISP_HDR2);
// 	//mpu_storage_init();
// 	mpu_disp_init(0);
// 	mpu_record_init();
// 	mpu_rtsp_init();
// 	mpu_rtsp_start(RTSP_ALL_CHN);
// 	cvr_net_resume();
// }

// static void button_app_urgtent_video(void) {
//     mpu_record_urgent_video(0);
// }
// static int cnt=-1;
// //static int g_video_run_ = 1;
// static void button_app_storage_format(void) {
//     if(true == mpu_record_check_active(0) ){//true == mpu_record_check_active(0)||
//         printf("******stop record*******\n");
//         mpu_record_stop(0);
//         //usleep(200000);
//         //g_video_run_=1;
//         //rkipc_ivs_deinit();
//         mpu_photo_init();
//         //mpu_photo_capture(0);

//     }else{
//         printf("******start record*******\n");
        
//         if(DISK_MOUNTED!=mpu_storage_get_status()){
//             mpu_storage_format(); 
//         }
//         mpu_record_init();
//         RKADK_PARAM_REC_CFG_S *pstRecCfg=RKADK_PARAM_GetRecCfg(0);
//         //SAMPLE_ISP_EnableLdch(0, true, 120);
//         //SAMPLE_COMM_SmartIr_Start(0);
//         //SAMPLE_ISP_EnableLdch(0, false, 0);
//         printf("pstRecCfg->pre_record_time=%d  pstRecCfg->pre_record_mode=%d\n",pstRecCfg->pre_record_time,pstRecCfg->pre_record_mode);
//         mpu_record_start(0);  
//         printf("get main venchn=%d ", RKADK_PARAM_GetVencChnId(0,RKADK_STREAM_TYPE_VIDEO_MAIN));
//         printf("get sub venchn=%d ", RKADK_PARAM_GetVencChnId(0,RKADK_STREAM_TYPE_VIDEO_SUB)); 
//         //mpu_photo_capture(0);
//         printf("start record end\n");
//     }

// }

// static int32_t button_app_init(void) {
//     key_event_cb_register(button_app_storage_format1, KEY_EVENT_VOLUMEUP);      /* button to take photo */
//     //key_event_cb_register(button_app_urgtent_video, KEY_EVENT_VOLUMEUP);
// 	key_event_cb_register(button_app_set, KEY_EVENT_SET);
// 	key_event_cb_register(button_app_menu, KEY_EVENT_MENU);
//     key_event_cb_register(button_app_photo, KEY_EVENT_SHUTTER);
//     key_event_cb_register(button_app_storage_format, KEY_EVENT_VOLUMEDOWN);
//     return 0;
// }

// static void button_app_deinit(void) {
//     key_event_cb_unregister(button_app_storage_format1);
//     key_event_cb_unregister(button_app_photo);
//     key_event_cb_unregister(button_app_urgtent_video);
//     key_event_cb_unregister(button_app_storage_format);
// }

// /*****************************************************************************************/

// static void hotplug_reboot_proc(const NETLINK_UEVENT_EVENT_S *event) {
//     if (NETLINK_UEVENT_MMCBLK_EVENT == event->event_type) {
//         if (true == event->mmcblk.detached)
//             sys_ctl(SYSTEM_CTRL_REBOOT);
//     }
// }

// static int32_t hotplug_reboot_init(void) {
//     netlink_uevent_cb_register(hotplug_reboot_proc, NETLINK_UEVENT_MMCBLK_EVENT);
//     return 0;
// }

// static void hotplug_reboot_deinit(void) {
//     netlink_uevent_cb_unregister(hotplug_reboot_proc);
// }

// /*****************************************************************************************/

// static void system_standby_proc(void) {
// #ifdef USE_RKAOV
//     sys_ctl(SYSTEM_CTRL_AOV);
// #else
//     sys_ctl(SYSTEM_CTRL_RESET);
// #endif
// }

// static int32_t system_standby_init(void) {
//     key_event_cb_register(system_standby_proc, KEY_EVENT_POWER);
//     return 0;
// }

// static void system_standby_deinit(void) {
//     key_event_cb_unregister(system_standby_proc);
// }



// // struct smart_ir_ctx g_smartIr_ctx;
// // static void rk_enable_ircut(bool on) {
// // 	if (!on) {
// // 		rk_gpio_set_value(g_smartIr_ctx.ircut_on_gpio, 1);
// // 		usleep(100 * 1000);
// // 		rk_gpio_set_value(g_smartIr_ctx.ircut_on_gpio, 0);
// // 	} else {
// // 		rk_gpio_set_value(g_smartIr_ctx.ircut_off_gpio, 1);
// // 		usleep(100 * 1000);
// // 		rk_gpio_set_value(g_smartIr_ctx.ircut_off_gpio, 0);
// // 	}
// // }

// // static int get_board_info(void) {
// // #ifdef AOV_FASTBOOT_ENABLE
// // 	struct sensor_init_cfg *sensor_init_param = SAMPLE_COMM_AOV_GetSensorInitParam();
// // 	g_smartIr_ctx.ircut_on_gpio = sensor_init_param->ircut_a.gpio_index;
// // 	g_smartIr_ctx.ircut_off_gpio = sensor_init_param->ircut_b.gpio_index;
// // 	g_smartIr_ctx.irled_pwm_channel = sensor_init_param->led_ir.pwm_channel;
// // 	g_smartIr_ctx.irled_enable_gpio = sensor_init_param->led_ir_enable.gpio_index;
// // 	g_smartIr_ctx.visled_pwm_channel = sensor_init_param->led_white.pwm_channel;
// // 	g_smartIr_ctx.visled_enable_gpio = sensor_init_param->led_white_enable.gpio_index;
// // #else
// // 	g_smartIr_ctx.ircut_on_gpio = get_int32_from_env("ircut_on_gpio", 74);
// // 	g_smartIr_ctx.ircut_off_gpio = get_int32_from_env("ircut_off_gpio", 75);
// // 	g_smartIr_ctx.irled_pwm_channel = get_int32_from_env("irled_pwm_channel", 3);
// // 	g_smartIr_ctx.irled_enable_gpio = get_int32_from_env("irled_enable_gpio", 72);
// // 	g_smartIr_ctx.visled_pwm_channel = get_int32_from_env("visled_pwm_channel", -1);
// // 	g_smartIr_ctx.visled_enable_gpio = get_int32_from_env("visled_enable_gpio", -1);
// // #endif
// // 	printf("%s: ircut on %d, ircut off %d, irled pwm %d, irled enable %d, wled pwm %d, "
// // 	       "wled enable %d\n",
// // 	       __func__, g_smartIr_ctx.ircut_on_gpio, g_smartIr_ctx.ircut_off_gpio,
// // 	       g_smartIr_ctx.irled_pwm_channel, g_smartIr_ctx.irled_enable_gpio,
// // 	       g_smartIr_ctx.visled_pwm_channel, g_smartIr_ctx.visled_enable_gpio);
// // 	return RK_SUCCESS;
// // }

// // static void *smart_ir_thread(void *args) {
// // 	int ret;
// // 	RK_SMART_IR_STATUS_t last_status;
// // 	rk_smart_ir_attr_t init_attr;

// // 	printf("%s: thread start!\n", __func__);
// // 	while (!g_smartIr_ctx.thread_is_quit) {
// // 		ret = rk_smart_ir_run(g_smartIr_ctx.ir_ctx, false, &g_smartIr_ctx.result);
// // 		if (ret != RK_SUCCESS) {
// // 			printf("%s: rk_smart_ir_run failed %#X\n", __func__, ret);
// // 			continue;
// // 		}
// // 	}
// // 	printf("%s: thread exit!\n", __func__);

// // 	return NULL;
// // }

// // RK_S32 SAMPLE_COMM_SmartIr_Start(int cam_id) {
// // 	int ret = RK_SUCCESS;
// // #if defined(AOV_FASTBOOT_ENABLE)
// // 	struct app_param_info *app_param = SAMPLE_COMM_AOV_GetAppParam();
// // 	int rk_night_mode = app_param->night_mode;
// // 	int rk_led_value = app_param->led_value;
// // 	int rk_color_mode = app_param->color_mode;
// // #else
// // 	int rk_night_mode = (int)get_int32_from_env("rk_night_mode", 2);
// // 	int rk_led_value = (int)get_int32_from_env("rk_led_value", 20);
// // 	int rk_color_mode = (int)get_int32_from_env("rk_color_mode", FULL_COLOR_MODE);
// // #endif
// // 	int irled_pwm_period = 10000, irled_pwm_duty = 0;
// // 	int visled_pwm_period = 10000, visled_pwm_duty = 0;
// // 	rk_smart_ir_attr_t attr;

// // 	printf("%s: night mode %d, led value %d, color mode %d\n", __func__, rk_night_mode,
// // 	       rk_led_value, rk_color_mode);
// // 	g_smartIr_ctx.night_mode = rk_night_mode;
// // 	g_smartIr_ctx.aiq_ctx = g_aiq_ctx[cam_id];
// // 	g_smartIr_ctx.ir_ctx = rk_smart_ir_init(g_smartIr_ctx.aiq_ctx);
// // 	if (!g_smartIr_ctx.ir_ctx) {
// // 		printf("%s: rk_smart_ir_init failed!\n", __func__);
// // 		return RK_FAILURE;
// // 	}

// // 	get_board_info();
// // 	// memset(&attr, 0, sizeof(attr));
// // 	rk_smart_ir_getAttr(g_smartIr_ctx.ir_ctx, &attr);

// // 	if (rk_night_mode == 2) {
// // 		// load configs: auto switch, manual irled
// // 		attr.init_status = (rk_color_mode != FULL_COLOR_MODE) ? RK_SMART_IR_STATUS_NIGHT
// // 		                                                      : RK_SMART_IR_STATUS_DAY;
// // 		attr.switch_mode = RK_SMART_IR_SWITCH_MODE_AUTO;
// // 		attr.light_mode = RK_SMART_IR_LIGHT_MODE_MANUAL;
// // 		attr.light_type = RK_SMART_IR_LIGHT_TYPE_IR;
// // 		attr.light_value = rk_led_value;
// // 		attr.params.d2n_envL_th = 0.04f;
// // 		attr.params.n2d_envL_th = 0.20f;
// // 		attr.params.rggain_base = 1.00f;
// // 		attr.params.bggain_base = 1.00f;
// // 		attr.params.awbgain_rad = 0.10f;
// // 		attr.params.awbgain_dis = 0.20f;
// // 		attr.params.switch_cnts_th = 50;
// // 		ret = rk_smart_ir_setAttr(g_smartIr_ctx.ir_ctx, &attr);
// // 		if (ret != RK_SUCCESS) {
// // 			printf("%s: rk_smart_ir_setAttr failed\n", __func__);
// // 			return ret;
// // 		}

// // 		ret |=
// // 		    rk_gpio_export_direction(g_smartIr_ctx.ircut_on_gpio, GPIO_DIRECTION_OUTPUT);
// // 		ret |=
// // 		    rk_gpio_export_direction(g_smartIr_ctx.ircut_off_gpio, GPIO_DIRECTION_OUTPUT);
// // 		ret |= rk_gpio_export_direction(g_smartIr_ctx.irled_enable_gpio,
// // 		                                GPIO_DIRECTION_OUTPUT);
// // 		irled_pwm_duty = irled_pwm_period * MIN(rk_led_value, 100) / 100;
// // 		g_smartIr_ctx.cur_irled_value = rk_led_value;
// // #ifndef ROCKIT_LIGHT_CTL
// // 		ret = rk_pwm_init(g_smartIr_ctx.irled_pwm_channel, irled_pwm_period,
// // 		                  irled_pwm_duty, PWM_POLARITY_NORMAL);
// // 		if (ret) {
// // 			printf("%s: rk_pwm_init error ret [%d]\n", __func__, ret);
// // 		}
// // #endif
// // 		// In fastboot sdk, init_status set by rtt. For normal sdk, init_status is day by
// // 		// default
// // 		g_smartIr_ctx.last_status = attr.init_status;
// // 		if (g_smartIr_ctx.last_status == RK_SMART_IR_STATUS_NIGHT) {
// // 			rk_gpio_set_value(g_smartIr_ctx.irled_enable_gpio, 1);
// // 			rk_enable_ircut(false);
// // 			printf("%s: smart ir init status is night\n", __func__);
// // 		} else {
// // 			rk_gpio_set_value(g_smartIr_ctx.irled_enable_gpio, 0);
// // 			rk_enable_ircut(true);
// // 			printf("%s: smart ir init status is day\n", __func__);
// // 		}
// // 	} else {
// // 		printf("%s: not support night mode %d!\n", __func__, rk_night_mode);
// // 	}
// // 	// create thread
// // 	g_smartIr_ctx.thread_is_quit = false;
// // 	pthread_create(&g_smartIr_ctx.thread_id, NULL, smart_ir_thread, NULL);
// // 	printf("%s: smart ir init done\n", __func__);
// // 	return RK_SUCCESS;
// // }

// // RK_S32 SAMPLE_COMM_SmartIr_Stop() {
// // #ifndef ROCKIT_LIGHT_CTL
// // 	rk_pwm_deinit(g_smartIr_ctx.irled_pwm_channel);
// // #endif
// // 	g_smartIr_ctx.thread_is_quit = true;
// // 	pthread_join(g_smartIr_ctx.thread_id, NULL);

// // 	if (g_smartIr_ctx.ir_ctx) {
// // 		rk_smart_ir_deInit(g_smartIr_ctx.ir_ctx);
// // 		g_smartIr_ctx.ir_ctx = NULL;
// // 	}
// // 	return RK_SUCCESS;
// // }

// struct smart_ir_ctx g_smartIr_ctx;
// static int rkipc_aiq_use_group = 0;
// static rk_aiq_sys_ctx_t *g_aiq_ctx[8];
// static rk_aiq_camgroup_ctx_t *g_camera_group_ctx[8];
// rk_aiq_sys_ctx_t *rkipc_aiq_get_ctx(int cam_id) {
// 	if (rkipc_aiq_use_group)
// 		return (rk_aiq_sys_ctx_t *)g_camera_group_ctx[cam_id];

// 	return g_aiq_ctx[cam_id];
// }
// // 【改动】函数保留，但内部不再执行任何GPIO动作，空实现
// static void rk_enable_ircut(bool on) {
// #if 0   // 屏蔽原有硬件操作
// 	if (!on) {
// 		rk_gpio_set_value(g_smartIr_ctx.ircut_on_gpio, 1);
// 		usleep(100 * 1000);
// 		rk_gpio_set_value(g_smartIr_ctx.ircut_on_gpio, 0);
// 	} else {
// 		rk_gpio_set_value(g_smartIr_ctx.ircut_off_gpio, 1);
// 		usleep(100 * 1000);
// 		rk_gpio_set_value(g_smartIr_ctx.ircut_off_gpio, 0);
// 	}
// #endif
// 	printf("%s: software-only smart ir mode, skip ircut gpio control, on=%d\n", __func__, on);
// }

// static void smart_ir_switch_scene(RK_SMART_IR_STATUS_t status) {
// 	int ret;

// 	if (!g_smartIr_ctx.aiq_ctx) {
// 		printf("%s: aiq ctx not ready, skip software scene switch\n", __func__);
// 		return;
// 	}

// 	if (status == RK_SMART_IR_STATUS_NIGHT) {
// 		ret = rk_aiq_uapi2_sysctl_switch_scene(g_smartIr_ctx.aiq_ctx, "normal", "night");
// 		if (ret != RK_SUCCESS) {
// 			printf("%s: switch_scene night fail ret:%#x\n", __func__, ret);
// 		} else {
// 			printf("switch scene to normal/night OK\n");
// 		}
// 	} else if (status == RK_SMART_IR_STATUS_DAY) {
// 		ret = rk_aiq_uapi2_sysctl_switch_scene(g_smartIr_ctx.aiq_ctx, "normal", "day");
// 		if (ret != RK_SUCCESS) {
// 			printf("%s: switch_scene day fail ret:%#x\n", __func__, ret);
// 		} else {
// 			printf("switch scene to normal/day OK\n");
// 		}
// 	}
// }

// static int get_board_info(void) {
// #ifdef AOV_FASTBOOT_ENABLE
// 	struct sensor_init_cfg *sensor_init_param = SAMPLE_COMM_AOV_GetSensorInitParam();
// 	g_smartIr_ctx.ircut_on_gpio = sensor_init_param->ircut_a.gpio_index;
// 	g_smartIr_ctx.ircut_off_gpio = sensor_init_param->ircut_b.gpio_index;
// 	g_smartIr_ctx.irled_pwm_channel = sensor_init_param->led_ir.pwm_channel;
// 	g_smartIr_ctx.irled_enable_gpio = sensor_init_param->led_ir_enable.gpio_index;
// 	g_smartIr_ctx.visled_pwm_channel = sensor_init_param->led_white.pwm_channel;
// 	g_smartIr_ctx.visled_enable_gpio = sensor_init_param->led_white_enable.gpio_index;
// #else
// 	// g_smartIr_ctx.ircut_on_gpio =  74;
// 	// g_smartIr_ctx.ircut_off_gpio =  75;
// 	// g_smartIr_ctx.irled_pwm_channel = 3;
// 	// g_smartIr_ctx.irled_enable_gpio = 72;
//     g_smartIr_ctx.ircut_on_gpio =  -1;
// 	g_smartIr_ctx.ircut_off_gpio =  -1;
// 	g_smartIr_ctx.irled_pwm_channel = -1;
// 	g_smartIr_ctx.irled_enable_gpio = -1;
// 	g_smartIr_ctx.visled_pwm_channel = -1;
// 	g_smartIr_ctx.visled_enable_gpio = -1;
// #endif
// 	printf("%s: ircut on %d, ircut off %d, irled pwm %d, irled enable %d, wled pwm %d, "
// 	       "wled enable %d\n",
// 	       __func__, g_smartIr_ctx.ircut_on_gpio, g_smartIr_ctx.ircut_off_gpio,
// 	       g_smartIr_ctx.irled_pwm_channel, g_smartIr_ctx.irled_enable_gpio,
// 	       g_smartIr_ctx.visled_pwm_channel, g_smartIr_ctx.visled_enable_gpio);
// 	return RK_SUCCESS;
// }

// static void *smart_ir_thread(void *args) {
// 	int ret;
// 	RK_SMART_IR_STATUS_t last_status = RK_SMART_IR_STATUS_DAY;

// 	printf("%s: thread start!\n", __func__);
// 	while (!g_smartIr_ctx.thread_is_quit) {
// 		ret = rk_smart_ir_run(g_smartIr_ctx.ir_ctx, false, &g_smartIr_ctx.result);
// 		if (ret != RK_SUCCESS) {
// 			printf("%s: rk_smart_ir_run failed %#X\n", __func__, ret);
// 			usleep(50000);
// 			continue;
// 		}

// 		// =========新增：状态变化时切换isp night/day子场景=========
// 		if (g_smartIr_ctx.result.status != last_status) {
// 			printf("smartir status change: %d -> %d\n", last_status, g_smartIr_ctx.result.status);
// 			smart_ir_switch_scene(g_smartIr_ctx.result.status);
// 			last_status = g_smartIr_ctx.result.status;
// 		}

//         usleep(200000);
// 	}
// 	printf("%s: thread exit!\n", __func__);
// 	return NULL;
// }
// // static void *smart_ir_thread(void *args) {
// // 	int ret;
// // 	printf("%s: thread start!\n", __func__);
// // 	while (!g_smartIr_ctx.thread_is_quit) {
// // 		ret = rk_smart_ir_run(g_smartIr_ctx.ir_ctx, false, &g_smartIr_ctx.result);
// // 		if (ret != RK_SUCCESS) {
// // 			printf("%s: rk_smart_ir_run failed %#X\n", __func__, ret);
// // 			continue;
// // 		}
// // 		// 【可选扩展】如果你后续需要在这里监听 result 状态变化打印昼夜切换事件
// // 		// if(g_smartIr_ctx.result.status != g_smartIr_ctx.last_status) {
// // 		//     printf("DAY/NIGHT switch detect: %d -> %d\n", g_smartIr_ctx.last_status, g_smartIr_ctx.result.status);
// // 		//     g_smartIr_ctx.last_status = g_smartIr_ctx.result.status;
// // 		// }
// //         usleep(200000);
// // 	}
// // 	printf("%s: thread exit!\n", __func__);
// // 	return NULL;
// // }

// RK_S32 SAMPLE_COMM_SmartIr_Start(int cam_id) {
// 	int ret = RK_SUCCESS;
// #if defined(AOV_FASTBOOT_ENABLE)
// 	struct app_param_info *app_param = SAMPLE_COMM_AOV_GetAppParam();
// 	int rk_night_mode = app_param->night_mode;
// 	int rk_led_value = app_param->led_value;
// 	int rk_color_mode = app_param->color_mode;
// #else
// 	int rk_night_mode =  2;
// 	int rk_led_value = 20;
// 	int rk_color_mode = 0;//FULL_COLOR_MODE;
// #endif
// 	rk_smart_ir_attr_t attr;

// 	printf("%s: night mode %d, led value %d, color mode %d\n", __func__, rk_night_mode,
// 	       rk_led_value, rk_color_mode);
// 	g_smartIr_ctx.night_mode = rk_night_mode;
// 	RKADK_MW_PTR aiq_ctx_ptr = NULL;
// 	if (SAMPLE_ISP_Get_AiqHandle(0, &aiq_ctx_ptr) == 0 && aiq_ctx_ptr) {
// 		g_smartIr_ctx.aiq_ctx = (rk_aiq_sys_ctx_t *)aiq_ctx_ptr;
// 	} else {
// 		printf("%s: aiq ctx is not ready, skip smart ir init now\n", __func__);
// 		return RK_SUCCESS;
// 	}
// 	g_smartIr_ctx.ir_ctx = rk_smart_ir_init(g_smartIr_ctx.aiq_ctx);
// 	if (!g_smartIr_ctx.ir_ctx) {
// 		printf("%s: rk_smart_ir_init failed!\n", __func__);
// 		return RK_FAILURE;
// 	}

// 	get_board_info();
// 	rk_smart_ir_getAttr(g_smartIr_ctx.ir_ctx, &attr);

// 	if (rk_night_mode == 2) {
// 		attr.init_status = RK_SMART_IR_STATUS_NIGHT;
		                                             
// 		attr.switch_mode = RK_SMART_IR_SWITCH_MODE_NIGHT;
// 		attr.light_mode = RK_SMART_IR_LIGHT_MODE_INVALID;//RK_SMART_IR_LIGHT_MODE_MANUAL;
// 		attr.light_type = RK_SMART_IR_LIGHT_TYPE_INVALID;//RK_SMART_IR_LIGHT_TYPE_IR;
// 		attr.light_value = rk_led_value;
// 		attr.params.d2n_envL_th = 0.04f;
// 		attr.params.n2d_envL_th = 0.20f;
// 		attr.params.rggain_base = 1.00f;
// 		attr.params.bggain_base = 1.00f;
// 		attr.params.awbgain_rad = 0.10f;
// 		attr.params.awbgain_dis = 0.20f;
// 		attr.params.switch_cnts_th = 50;
// 		ret = rk_smart_ir_setAttr(g_smartIr_ctx.ir_ctx, &attr);
// 		if (ret != RK_SUCCESS) {
// 			printf("%s: rk_smart_ir_setAttr failed\n", __func__);
// 			return ret;
// 		}
// 		// =========【核心改动开始】屏蔽所有GPIO导出、PWM初始化、硬件引脚操作 =========
// #if 0
// 		ret |= rk_gpio_export_direction(g_smartIr_ctx.ircut_on_gpio, GPIO_DIRECTION_OUTPUT);
// 		ret |= rk_gpio_export_direction(g_smartIr_ctx.ircut_off_gpio, GPIO_DIRECTION_OUTPUT);
// 		ret |= rk_gpio_export_direction(g_smartIr_ctx.irled_enable_gpio, GPIO_DIRECTION_OUTPUT);

// 		int irled_pwm_period = 10000, irled_pwm_duty = 0;
// 		irled_pwm_duty = irled_pwm_period * MIN(rk_led_value, 100) / 100;
// 		g_smartIr_ctx.cur_irled_value = rk_led_value;
// #ifndef ROCKIT_LIGHT_CTL
// 		ret = rk_pwm_init(g_smartIr_ctx.irled_pwm_channel, irled_pwm_period,
// 		                  irled_pwm_duty, PWM_POLARITY_NORMAL);
// 		if (ret) {
// 			printf("%s: rk_pwm_init error ret [%d]\n", __func__, ret);
// 		}
// #endif
// #endif
// 		// 初始状态：不再控制IRcut、IR灯GPIO，仅保存状态变量
// 		g_smartIr_ctx.last_status = attr.init_status;
// 		printf("%s: smart ir init status = %d, software-only mode enabled, hardware gpio/pwm control disabled\n",
// 		       __func__, g_smartIr_ctx.last_status);
// 		// =========【核心改动结束】 =========
// 	} else {
// 		printf("%s: not support night mode %d!\n", __func__, rk_night_mode);
// 	}
// 	// 创建检测线程保持运行，持续执行rk_smart_ir_run计算昼夜状态
// 	g_smartIr_ctx.thread_is_quit = false;
// 	pthread_create(&g_smartIr_ctx.thread_id, NULL, smart_ir_thread, NULL);
// 	printf("%s: smart ir init done, only DAY/NIGHT algorithm enabled, hardware control disabled\n", __func__);
// 	return RK_SUCCESS;
// }

// RK_S32 SAMPLE_COMM_SmartIr_Stop() {
// 	// 屏蔽PWM反初始化
// #if 0
// #ifndef ROCKIT_LIGHT_CTL
// 	rk_pwm_deinit(g_smartIr_ctx.irled_pwm_channel);
// #endif
// #endif
// 	g_smartIr_ctx.thread_is_quit = true;
// 	pthread_join(g_smartIr_ctx.thread_id, NULL);

// 	if (g_smartIr_ctx.ir_ctx) {
// 		rk_smart_ir_deInit(g_smartIr_ctx.ir_ctx);
// 		g_smartIr_ctx.ir_ctx = NULL;
// 	}
// 	return RK_SUCCESS;
// }
// /*****************************************************************************************/

// static int32_t app_init(void) {
//     printf("************app_init start**********************\n");
    
// #ifdef REMOTE_CONFIG
//     remote_cfg_init();
// #endif
    
//     button_app_init();
//     hotplug_reboot_init();
//     system_standby_init();

//     //mpu_photo_init();
//     //mpu_record_init();

//     //mpu_record_set_fps(0, RKADK_STREAM_TYPE_VIDEO_MAIN,25);
//     //mpu_record_set_gop(0, RKADK_STREAM_TYPE_VIDEO_MAIN,25);
//     mpu_record_reset(0);
//     mpu_record_init();
//     //rkipc_ivs_init();
    
	
//     // rk_gpio_export_direction(GPIO(RK_GPIO0, RK_PA3), GPIO_DIRECTION_INPUT);
//     // rk_gpio_set_value(GPIO(RK_GPIO0, RK_PA3), 0);
//     return 0;
// }

// static void app_deinit(void) {
// #ifdef REMOTE_CONFIG
//     remote_cfg_deinit();
// #endif

//     system_standby_deinit();

//     hotplug_reboot_deinit();

// #ifndef USE_LVGL
//     button_app_deinit();
// #endif
// }



// static int32_t app_sleep(void) {
//     return 0;
// }

// static int32_t app_wakeup(void) {
//     return 0;
// }

// /**********************
//  *   GLOBAL FUNCTIONS
//  **********************/

// static module_node_t app_module = {
//     .priority = MODULE_LEVEL_APP,
//     .ops = {
//         .init = app_init,
//         .deinit = app_deinit,
//         .sleep = app_sleep,
//         .wakeup = app_wakeup,
//     }
// };

// SYS_MODULE_REGISTER(app_module)

/*
 * Copyright (c) 2022 Rockchip, Inc. All Rights Reserved.
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

#include "utils/log.h"
#include "core/key_event.h"
#include "core/linux_netlink.h"
#include "core/sys.h"
#include "cvr_conf.h"

#if CVR_RKNN_STREAM_DETECT
#include "rknn_api.h"
#include "rk_mpi_mb.h"
#include "rk_mpi_vi.h"
#include "rk_mpi_rgn.h"
#include <math.h>
#include <time.h>
#endif

#include "mpu/photo/mpu_photo.h"
#include "mpu/record/mpu_record.h"
#include "mpu/storage/mpu_storage.h"
#include "mpu/rtsp/mpu_rtsp.h"
#include "net/cvr_net.h"

#include "app/remote_config.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/prctl.h>          // 提供 prctl() 和 PR_SET_NAME
#include "rk_mpi_ivs.h"         // 提供 IVS_RESULT_INFO_S 等类型
#include "rk_mpi_vpss.h"
#include "rk_mpi_sys.h"
#include "rk_gpio.h"
#include "rk_pwm.h"
#include "rk_smart_ir_api.h"
#include "rk_aiq_user_api2_camgroup.h"
#include "rk_aiq_user_api2_sysctl.h"
#include "mpu/isp/rv1126b/sample_isp.h"

#ifndef FULL_COLOR_MODE
#define FULL_COLOR_MODE 1
#endif

struct smart_ir_ctx {
    bool thread_is_quit;
    pthread_t thread_id;
    rk_smart_ir_result_t result;
    RK_SMART_IR_STATUS_t last_status;
    rk_smart_ir_ctx_t *ir_ctx;
    rk_aiq_sys_ctx_t *aiq_ctx;
    int night_mode;
    int ircut_on_gpio;
    int ircut_off_gpio;
    int irled_pwm_channel;
    int irled_enable_gpio;
    int visled_pwm_channel;
    int visled_enable_gpio;
    int cur_irled_value;
};

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/
// 在 main_app.c 文件顶部添加头文件
#include "ui_thumb.h"
//#include "ui/common/ui_page_manager.h"

// 添加外部声明
//extern lv_obj_t *ui_get_current_page_obj(void);  // 假设有这个函数获取当前页面对象


//extern int rk_isp_af_zoom_change(int cam_id, int change);
// 修改 button_app_storage_format 函数
static void button_app_storage_format1(void) {
    //cvr_start_ap();
    //user_request_stop(10);
    //rk_isp_af_zoom_change(0,30);
    // lv_obj_t *parent_obj = lv_scr_act();
    //rk_isp_af_zoom_change(0,20);
	extern int rk_enable_usb_function(void);
	rk_enable_usb_function();
    
}

static void button_app_photo(void){
    // mpu_photo_init();
    // mpu_photo_burst_capture(0,20);
    // sleep(2);
    // mpu_photo_deinit();
    // mpu_rtsp_init();
    // mpu_rtsp_start(0);
    //rk_isp_af_zoom_change(0,-30);
    //rk_isp_af_zoom_change(0,-20);
	mpu_photo_deinit();
	//mpu_photo_set_resolution(0,RKADK_RES_4320P);
	RKADK_PARAM_PHOTO_CFG_S *pstPhotoCfg = RKADK_PARAM_GetPhotoCfg(0);
	pstPhotoCfg->jpeg_slice = TRUE;
	pstPhotoCfg->image_width = 8192;
    pstPhotoCfg->image_height = 4608;
	pstPhotoCfg->slice_height = 4608;
	mpu_photo_init();
	// mpu_photo_capture(0);
}

static void button_app_set(void){
	//mpu_record_deinit();
	// mpu_record_set_fps(0,RKADK_STREAM_TYPE_VIDEO_MAIN,25);
	// mpu_record_set_gop(0,RKADK_STREAM_TYPE_VIDEO_MAIN,25);
	// mpu_record_reset(0);
	// mpu_record_init();
	//mpu_record_set_resolution(0,RKADK_RES_1440P);
	// mpu_record_reset(0);
	// mpu_record_init();
	mpu_photo_capture(0);
}
extern int CVR_SetSensorMode(RKADK_U32 cam_id,
                      uint32_t width,
                      uint32_t height,
                      uint32_t mbus_code,
                      rk_aiq_working_mode_t hdr_mode);

static void button_app_menu(void){
	/* 挂起 cvr_net worker 的 RTSP 自动重建，防止 sensor/ISP 重配期间
	 * VI 通道被重新使能(MIPI 错误、VENC 超时) */
	cvr_net_suspend();
	mpu_rtsp_deinit();
	mpu_record_deinit();
	mpu_photo_deinit();
	mpu_disp_deinit();
	//mpu_storage_deinit();
	usleep(200);
	CVR_SetSensorMode(0,1944,1097, 0x3010,0);
	//CVR_SetSensorMode(0,1920,1080, 0x3010,0);
	//CVR_SetSensorMode(0,2560,1440, 0x300e,0);
	//CVR_SetSensorMode(0,3864,2192, 0x300e,RK_AIQ_WORKING_MODE_ISP_HDR2);
	//mpu_storage_init();
	mpu_disp_init(0);
	mpu_record_init();
	mpu_rtsp_init();
	mpu_rtsp_start(RTSP_ALL_CHN);
	cvr_net_resume();
}

static void button_app_urgtent_video(void) {
    mpu_record_urgent_video(0);
}
static int cnt=-1;
//static int g_video_run_ = 1;
static void button_app_storage_format(void) {
    if(true == mpu_record_check_active(0) ){//true == mpu_record_check_active(0)||
        printf("******stop record*******\n");
        mpu_record_stop(0);
        //usleep(200000);
        //g_video_run_=1;
        //rkipc_ivs_deinit();
        mpu_photo_init();
        //mpu_photo_capture(0);

    }else{
        printf("******start record*******\n");
        
        if(DISK_MOUNTED!=mpu_storage_get_status()){
            mpu_storage_format(); 
        }
        mpu_record_init();
        RKADK_PARAM_REC_CFG_S *pstRecCfg=RKADK_PARAM_GetRecCfg(0);
        //SAMPLE_ISP_EnableLdch(0, true, 120);
        //SAMPLE_COMM_SmartIr_Start(0);
        //SAMPLE_ISP_EnableLdch(0, false, 0);
        printf("pstRecCfg->pre_record_time=%d  pstRecCfg->pre_record_mode=%d\n",pstRecCfg->pre_record_time,pstRecCfg->pre_record_mode);
        mpu_record_start(0);  
        printf("get main venchn=%d ", RKADK_PARAM_GetVencChnId(0,RKADK_STREAM_TYPE_VIDEO_MAIN));
        printf("get sub venchn=%d ", RKADK_PARAM_GetVencChnId(0,RKADK_STREAM_TYPE_VIDEO_SUB)); 
        //mpu_photo_capture(0);
        printf("start record end\n");
    }

}

static int32_t button_app_init(void) {
    key_event_cb_register(button_app_storage_format1, KEY_EVENT_VOLUMEUP);      /* button to take photo */
    //key_event_cb_register(button_app_urgtent_video, KEY_EVENT_VOLUMEUP);
	key_event_cb_register(button_app_set, KEY_EVENT_SET);
	key_event_cb_register(button_app_menu, KEY_EVENT_MENU);
    key_event_cb_register(button_app_photo, KEY_EVENT_SHUTTER);
    key_event_cb_register(button_app_storage_format, KEY_EVENT_VOLUMEDOWN);
    return 0;
}

static void button_app_deinit(void) {
    key_event_cb_unregister(button_app_storage_format1);
    key_event_cb_unregister(button_app_photo);
    key_event_cb_unregister(button_app_urgtent_video);
    key_event_cb_unregister(button_app_storage_format);
}

static int read_model_file(const char *model_path, void **model, uint32_t *model_len) {
	FILE *file;
	long file_size;
	void *buffer;
	int retry_count;

	if (!model_path || !model || !model_len)
		return -1;

	file = NULL;
	for (retry_count = 0; retry_count < 100; retry_count++) {
		file = fopen(model_path, "rb");
		if (file)
			break;
		if (errno != ENOENT && errno != ENODEV)
			break;
		usleep(100000);
	}
	if (!file)
		return -1;

	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return -1;
	}

	file_size = ftell(file);
	if (file_size <= 0 || (unsigned long)file_size > UINT32_MAX ||
		fseek(file, 0, SEEK_SET) != 0) {
		fclose(file);
		return -1;
	}

	buffer = malloc((size_t)file_size);
	if (!buffer || fread(buffer, 1, (size_t)file_size, file) != (size_t)file_size) {
		free(buffer);
		fclose(file);
		return -1;
	}

	fclose(file);
	*model = buffer;
	*model_len = (uint32_t)file_size;
	return 0;
}

#if CVR_RKNN_STREAM_DETECT
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
 *   0: 自动 (>=3 个输出 -> yolov5 3 分支, 1 个输出 -> yolov8 单输出)
 *   1: yolov5, 3 个分支输出(原始输出, 需要用 anchors 解码)
 *   2: yolov5, 单个已解码输出([1,N,5+nc], obj/cls 需 sigmoid)
 *   3: yolov8, 单个输出([1,4+nc,N], 分数已解码, 无需 sigmoid)
 * 模型输出不匹配时按 rknn_dump_output_attrs() 打印的 dims 调整。 */
#define CVR_RKNN_POSTPROC        0
#define DET_MAX_BOXES            32
#define DET_CONF_THRESHOLD       0.25f
#define DET_NMS_THRESHOLD        0.45f

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

/* 解析全部输出 -> g_det_boxes, 返回框个数 */
static int rknn_postprocess_outputs(const rknn_output *outputs, uint32_t n_out) {
	int mode = CVR_RKNN_POSTPROC;
	int n = 0, i;

	if (mode == 0)
		mode = (n_out >= 3) ? 1 : 3;

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

static int nv12_to_rgb(const uint8_t *src, uint32_t src_width, uint32_t src_height,
					   uint32_t src_stride, uint8_t *dst, uint32_t dst_width,
					   uint32_t dst_height) {
	uint32_t y;

	if (!src || !dst || !src_width || !src_height || !src_stride ||
		!dst_width || !dst_height)
		return -1;

	for (y = 0; y < dst_height; y++) {
		uint32_t src_y = y * src_height / dst_height;
		uint32_t x;
		for (x = 0; x < dst_width; x++) {
			uint32_t src_x = x * src_width / dst_width;
			int y_value = src[src_y * src_stride + src_x];
			uint32_t uv_offset = src_stride * src_height + (src_y / 2) * src_stride;
			int u_value = src[uv_offset + (src_x & ~1)] - 128;
			int v_value = src[uv_offset + (src_x & ~1) + 1] - 128;
			int red = y_value + (int)(1.402f * v_value);
			int green = y_value - (int)(0.344f * u_value + 0.714f * v_value);
			int blue = y_value + (int)(1.772f * u_value);
			uint8_t *pixel = dst + (y * dst_width + x) * 3;

			pixel[0] = (uint8_t)(red < 0 ? 0 : red > 255 ? 255 : red);
			pixel[1] = (uint8_t)(green < 0 ? 0 : green > 255 ? 255 : green);
			pixel[2] = (uint8_t)(blue < 0 ? 0 : blue > 255 ? 255 : blue);
		}
	}
	return 0;
}

static void rgb_nhwc_to_nchw(const uint8_t *src, uint8_t *dst,
							 uint32_t width, uint32_t height) {
	uint32_t pixel_count = width * height;
	uint32_t i;

	for (i = 0; i < pixel_count; i++) {
		dst[i] = src[i * 3];
		dst[pixel_count + i] = src[i * 3 + 1];
		dst[pixel_count * 2 + i] = src[i * 3 + 2];
	}
}

static void *rknn_detect_thread(void *arg) {
	(void)arg;
	uint8_t *input_rgb = malloc(g_rknn_input_width * g_rknn_input_height * 3);
	uint8_t *input_data = malloc(g_rknn_input_width * g_rknn_input_height * 3);

	if (!input_rgb || !input_data) {
		free(input_rgb);
		free(input_data);
		return NULL;
	}

	while (g_rknn_detect_running) {
		VIDEO_FRAME_INFO_S frame = {0};
		int ret = RK_MPI_VI_GetChnFrame(0, 3, &frame, 1000);
		if (ret != RK_SUCCESS)
			continue;

		uint8_t *frame_data = RK_MPI_MB_Handle2VirAddr(frame.stVFrame.pMbBlk);
		uint32_t frame_size = RK_MPI_MB_GetSize(frame.stVFrame.pMbBlk);
		uint32_t frame_stride = frame.stVFrame.u32VirWidth;
		rknn_input input = {0};
		rknn_output outputs[16] = {0};
		rknn_input_output_num io_num = {0};

		if (frame_data && frame_size >= frame_stride * frame.stVFrame.u32VirHeight &&
			nv12_to_rgb(frame_data, frame.stVFrame.u32Width, frame.stVFrame.u32Height,
						frame_stride, input_rgb, g_rknn_input_width,
						g_rknn_input_height) == 0) {
			input.index = 0;
			input.type = RKNN_TENSOR_UINT8;
			input.size = g_rknn_input_width * g_rknn_input_height * 3;
			input.fmt = g_rknn_input_format;
			if (g_rknn_input_format == RKNN_TENSOR_NCHW)
				rgb_nhwc_to_nchw(input_rgb, input_data, g_rknn_input_width,
								 g_rknn_input_height);
			else
				memcpy(input_data, input_rgb, input.size);
			input.buf = input_data;

			ret = rknn_inputs_set(g_rknn_detect_ctx, 1, &input);
			if (ret == RKNN_SUCC)
				ret = rknn_run(g_rknn_detect_ctx, NULL);
			if (ret == RKNN_SUCC && rknn_query(g_rknn_detect_ctx,
											   RKNN_QUERY_IN_OUT_NUM, &io_num,
											   sizeof(io_num)) == RKNN_SUCC) {
				for (uint32_t i = 0; i < io_num.n_output && i < 16; i++)
					outputs[i].want_float = 1;
				if (rknn_outputs_get(g_rknn_detect_ctx, io_num.n_output,
								 outputs, NULL) == RKNN_SUCC) {
					int box_cnt = rknn_postprocess_outputs(outputs, io_num.n_output);

					for (int i = 0; i < box_cnt; i++) {
						int c = g_det_boxes[i].cls;

						printf("det[%d/%d]: cls=%d(%s) conf=%.2f box=(%.0f,%.0f)-(%.0f,%.0f)\n",
							   i + 1, box_cnt, c,
							   (c >= 0 && c < 80) ? g_coco_names[c] : "?",
							   g_det_boxes[i].conf,
							   g_det_boxes[i].x1, g_det_boxes[i].y1,
							   g_det_boxes[i].x2, g_det_boxes[i].y2);
					}
					/* 把检测框 + 类别 index/名称 叠加到 DISP 流 */
					disp_osd_show(g_det_boxes, box_cnt);
					rknn_outputs_release(g_rknn_detect_ctx, io_num.n_output, outputs);
				}
			}
		}
		RK_MPI_VI_ReleaseChnFrame(0, 3, &frame);
	}

	free(input_rgb);
	free(input_data);
	return NULL;
}

static int rknn_stream_detect_start(const char *model_path) {
	void *model = NULL;
	uint32_t model_len = 0;
	rknn_tensor_attr input_attr = {0};
	int ret;

	if (read_model_file(model_path, &model, &model_len) != 0)
		return -1;

	ret = rknn_init(&g_rknn_detect_ctx, model, model_len, 0, NULL);
	free(model);
	if (ret != RKNN_SUCC)
		return ret;

	input_attr.index = 0;
	ret = rknn_query(g_rknn_detect_ctx, RKNN_QUERY_INPUT_ATTR, &input_attr,
					 sizeof(input_attr));
	if (ret != RKNN_SUCC || input_attr.n_dims < 4)
		goto fail;

	g_rknn_input_format = input_attr.fmt;
	if (input_attr.fmt == RKNN_TENSOR_NCHW) {
		g_rknn_input_height = input_attr.dims[2];
		g_rknn_input_width = input_attr.dims[3];
	} else {
		g_rknn_input_height = input_attr.dims[1];
		g_rknn_input_width = input_attr.dims[2];
	}
	if (!g_rknn_input_width || !g_rknn_input_height)
		goto fail;

	{
		rknn_input_output_num io_num = {0};
		ret = rknn_query(g_rknn_detect_ctx, RKNN_QUERY_IN_OUT_NUM, &io_num,
					 sizeof(io_num));
		if (ret != RKNN_SUCC || io_num.n_output > 16)
			goto fail;
		g_rknn_output_count = io_num.n_output;
		for (uint32_t i = 0; i < g_rknn_output_count; i++) {
			g_rknn_output_attrs[i].index = i;
			ret = rknn_query(g_rknn_detect_ctx, RKNN_QUERY_OUTPUT_ATTR,
						 &g_rknn_output_attrs[i], sizeof(g_rknn_output_attrs[i]));
			if (ret != RKNN_SUCC)
				goto fail;
		}
		rknn_dump_output_attrs();
		printf("RKNN output attributes ready, postproc=%d(auto), boxes+index will be drawn on DISP osd\n",
			   CVR_RKNN_POSTPROC);
	}

	g_rknn_detect_running = true;
	if (pthread_create(&g_rknn_detect_thread, NULL, rknn_detect_thread, NULL) != 0)
		goto fail_thread;
	printf("RKNN stream detection started, input=%ux%u\n",
		   g_rknn_input_width, g_rknn_input_height);
	return 0;

fail_thread:
	g_rknn_detect_running = false;
fail:
	rknn_destroy(g_rknn_detect_ctx);
	g_rknn_detect_ctx = 0;
	return -1;
}

static void rknn_stream_detect_stop(void) {
	if (g_rknn_detect_running) {
		g_rknn_detect_running = false;
		pthread_join(g_rknn_detect_thread, NULL);
	}
	/* 卸载 DISP 流上的检测 OSD 并释放画布 */
	disp_osd_detach();
	free(g_disp_osd_canvas);
	g_disp_osd_canvas = NULL;
	if (g_rknn_detect_ctx) {
		rknn_destroy(g_rknn_detect_ctx);
		g_rknn_detect_ctx = 0;
	}
}
#endif

/*****************************************************************************************/

static void hotplug_reboot_proc(const NETLINK_UEVENT_EVENT_S *event) {
    if (NETLINK_UEVENT_MMCBLK_EVENT == event->event_type) {
        if (true == event->mmcblk.detached)
            sys_ctl(SYSTEM_CTRL_REBOOT);
    }
}

static int32_t hotplug_reboot_init(void) {
    netlink_uevent_cb_register(hotplug_reboot_proc, NETLINK_UEVENT_MMCBLK_EVENT);
    return 0;
}

static void hotplug_reboot_deinit(void) {
    netlink_uevent_cb_unregister(hotplug_reboot_proc);
}

/*****************************************************************************************/

static void system_standby_proc(void) {
#ifdef USE_RKAOV
    sys_ctl(SYSTEM_CTRL_AOV);
#else
    sys_ctl(SYSTEM_CTRL_RESET);
#endif
}

static int32_t system_standby_init(void) {
    key_event_cb_register(system_standby_proc, KEY_EVENT_POWER);
    return 0;
}

static void system_standby_deinit(void) {
    key_event_cb_unregister(system_standby_proc);
}



// struct smart_ir_ctx g_smartIr_ctx;
// static void rk_enable_ircut(bool on) {
// 	if (!on) {
// 		rk_gpio_set_value(g_smartIr_ctx.ircut_on_gpio, 1);
// 		usleep(100 * 1000);
// 		rk_gpio_set_value(g_smartIr_ctx.ircut_on_gpio, 0);
// 	} else {
// 		rk_gpio_set_value(g_smartIr_ctx.ircut_off_gpio, 1);
// 		usleep(100 * 1000);
// 		rk_gpio_set_value(g_smartIr_ctx.ircut_off_gpio, 0);
// 	}
// }

// static int get_board_info(void) {
// #ifdef AOV_FASTBOOT_ENABLE
// 	struct sensor_init_cfg *sensor_init_param = SAMPLE_COMM_AOV_GetSensorInitParam();
// 	g_smartIr_ctx.ircut_on_gpio = sensor_init_param->ircut_a.gpio_index;
// 	g_smartIr_ctx.ircut_off_gpio = sensor_init_param->ircut_b.gpio_index;
// 	g_smartIr_ctx.irled_pwm_channel = sensor_init_param->led_ir.pwm_channel;
// 	g_smartIr_ctx.irled_enable_gpio = sensor_init_param->led_ir_enable.gpio_index;
// 	g_smartIr_ctx.visled_pwm_channel = sensor_init_param->led_white.pwm_channel;
// 	g_smartIr_ctx.visled_enable_gpio = sensor_init_param->led_white_enable.gpio_index;
// #else
// 	g_smartIr_ctx.ircut_on_gpio = get_int32_from_env("ircut_on_gpio", 74);
// 	g_smartIr_ctx.ircut_off_gpio = get_int32_from_env("ircut_off_gpio", 75);
// 	g_smartIr_ctx.irled_pwm_channel = get_int32_from_env("irled_pwm_channel", 3);
// 	g_smartIr_ctx.irled_enable_gpio = get_int32_from_env("irled_enable_gpio", 72);
// 	g_smartIr_ctx.visled_pwm_channel = get_int32_from_env("visled_pwm_channel", -1);
// 	g_smartIr_ctx.visled_enable_gpio = get_int32_from_env("visled_enable_gpio", -1);
// #endif
// 	printf("%s: ircut on %d, ircut off %d, irled pwm %d, irled enable %d, wled pwm %d, "
// 	       "wled enable %d\n",
// 	       __func__, g_smartIr_ctx.ircut_on_gpio, g_smartIr_ctx.ircut_off_gpio,
// 	       g_smartIr_ctx.irled_pwm_channel, g_smartIr_ctx.irled_enable_gpio,
// 	       g_smartIr_ctx.visled_pwm_channel, g_smartIr_ctx.visled_enable_gpio);
// 	return RK_SUCCESS;
// }

// static void *smart_ir_thread(void *args) {
// 	int ret;
// 	RK_SMART_IR_STATUS_t last_status;
// 	rk_smart_ir_attr_t init_attr;

// 	printf("%s: thread start!\n", __func__);
// 	while (!g_smartIr_ctx.thread_is_quit) {
// 		ret = rk_smart_ir_run(g_smartIr_ctx.ir_ctx, false, &g_smartIr_ctx.result);
// 		if (ret != RK_SUCCESS) {
// 			printf("%s: rk_smart_ir_run failed %#X\n", __func__, ret);
// 			continue;
// 		}
// 	}
// 	printf("%s: thread exit!\n", __func__);

// 	return NULL;
// }

// RK_S32 SAMPLE_COMM_SmartIr_Start(int cam_id) {
// 	int ret = RK_SUCCESS;
// #if defined(AOV_FASTBOOT_ENABLE)
// 	struct app_param_info *app_param = SAMPLE_COMM_AOV_GetAppParam();
// 	int rk_night_mode = app_param->night_mode;
// 	int rk_led_value = app_param->led_value;
// 	int rk_color_mode = app_param->color_mode;
// #else
// 	int rk_night_mode = (int)get_int32_from_env("rk_night_mode", 2);
// 	int rk_led_value = (int)get_int32_from_env("rk_led_value", 20);
// 	int rk_color_mode = (int)get_int32_from_env("rk_color_mode", FULL_COLOR_MODE);
// #endif
// 	int irled_pwm_period = 10000, irled_pwm_duty = 0;
// 	int visled_pwm_period = 10000, visled_pwm_duty = 0;
// 	rk_smart_ir_attr_t attr;

// 	printf("%s: night mode %d, led value %d, color mode %d\n", __func__, rk_night_mode,
// 	       rk_led_value, rk_color_mode);
// 	g_smartIr_ctx.night_mode = rk_night_mode;
// 	g_smartIr_ctx.aiq_ctx = g_aiq_ctx[cam_id];
// 	g_smartIr_ctx.ir_ctx = rk_smart_ir_init(g_smartIr_ctx.aiq_ctx);
// 	if (!g_smartIr_ctx.ir_ctx) {
// 		printf("%s: rk_smart_ir_init failed!\n", __func__);
// 		return RK_FAILURE;
// 	}

// 	get_board_info();
// 	// memset(&attr, 0, sizeof(attr));
// 	rk_smart_ir_getAttr(g_smartIr_ctx.ir_ctx, &attr);

// 	if (rk_night_mode == 2) {
// 		// load configs: auto switch, manual irled
// 		attr.init_status = (rk_color_mode != FULL_COLOR_MODE) ? RK_SMART_IR_STATUS_NIGHT
// 		                                                      : RK_SMART_IR_STATUS_DAY;
// 		attr.switch_mode = RK_SMART_IR_SWITCH_MODE_AUTO;
// 		attr.light_mode = RK_SMART_IR_LIGHT_MODE_MANUAL;
// 		attr.light_type = RK_SMART_IR_LIGHT_TYPE_IR;
// 		attr.light_value = rk_led_value;
// 		attr.params.d2n_envL_th = 0.04f;
// 		attr.params.n2d_envL_th = 0.20f;
// 		attr.params.rggain_base = 1.00f;
// 		attr.params.bggain_base = 1.00f;
// 		attr.params.awbgain_rad = 0.10f;
// 		attr.params.awbgain_dis = 0.20f;
// 		attr.params.switch_cnts_th = 50;
// 		ret = rk_smart_ir_setAttr(g_smartIr_ctx.ir_ctx, &attr);
// 		if (ret != RK_SUCCESS) {
// 			printf("%s: rk_smart_ir_setAttr failed\n", __func__);
// 			return ret;
// 		}

// 		ret |=
// 		    rk_gpio_export_direction(g_smartIr_ctx.ircut_on_gpio, GPIO_DIRECTION_OUTPUT);
// 		ret |=
// 		    rk_gpio_export_direction(g_smartIr_ctx.ircut_off_gpio, GPIO_DIRECTION_OUTPUT);
// 		ret |= rk_gpio_export_direction(g_smartIr_ctx.irled_enable_gpio,
// 		                                GPIO_DIRECTION_OUTPUT);
// 		irled_pwm_duty = irled_pwm_period * MIN(rk_led_value, 100) / 100;
// 		g_smartIr_ctx.cur_irled_value = rk_led_value;
// #ifndef ROCKIT_LIGHT_CTL
// 		ret = rk_pwm_init(g_smartIr_ctx.irled_pwm_channel, irled_pwm_period,
// 		                  irled_pwm_duty, PWM_POLARITY_NORMAL);
// 		if (ret) {
// 			printf("%s: rk_pwm_init error ret [%d]\n", __func__, ret);
// 		}
// #endif
// 		// In fastboot sdk, init_status set by rtt. For normal sdk, init_status is day by
// 		// default
// 		g_smartIr_ctx.last_status = attr.init_status;
// 		if (g_smartIr_ctx.last_status == RK_SMART_IR_STATUS_NIGHT) {
// 			rk_gpio_set_value(g_smartIr_ctx.irled_enable_gpio, 1);
// 			rk_enable_ircut(false);
// 			printf("%s: smart ir init status is night\n", __func__);
// 		} else {
// 			rk_gpio_set_value(g_smartIr_ctx.irled_enable_gpio, 0);
// 			rk_enable_ircut(true);
// 			printf("%s: smart ir init status is day\n", __func__);
// 		}
// 	} else {
// 		printf("%s: not support night mode %d!\n", __func__, rk_night_mode);
// 	}
// 	// create thread
// 	g_smartIr_ctx.thread_is_quit = false;
// 	pthread_create(&g_smartIr_ctx.thread_id, NULL, smart_ir_thread, NULL);
// 	printf("%s: smart ir init done\n", __func__);
// 	return RK_SUCCESS;
// }

// RK_S32 SAMPLE_COMM_SmartIr_Stop() {
// #ifndef ROCKIT_LIGHT_CTL
// 	rk_pwm_deinit(g_smartIr_ctx.irled_pwm_channel);
// #endif
// 	g_smartIr_ctx.thread_is_quit = true;
// 	pthread_join(g_smartIr_ctx.thread_id, NULL);

// 	if (g_smartIr_ctx.ir_ctx) {
// 		rk_smart_ir_deInit(g_smartIr_ctx.ir_ctx);
// 		g_smartIr_ctx.ir_ctx = NULL;
// 	}
// 	return RK_SUCCESS;
// }

struct smart_ir_ctx g_smartIr_ctx;
static int rkipc_aiq_use_group = 0;
static rk_aiq_sys_ctx_t *g_aiq_ctx[8];
static rk_aiq_camgroup_ctx_t *g_camera_group_ctx[8];
rk_aiq_sys_ctx_t *rkipc_aiq_get_ctx(int cam_id) {
	if (rkipc_aiq_use_group)
		return (rk_aiq_sys_ctx_t *)g_camera_group_ctx[cam_id];

	return g_aiq_ctx[cam_id];
}
// 【改动】函数保留，但内部不再执行任何GPIO动作，空实现
static void rk_enable_ircut(bool on) {
#if 0   // 屏蔽原有硬件操作
	if (!on) {
		rk_gpio_set_value(g_smartIr_ctx.ircut_on_gpio, 1);
		usleep(100 * 1000);
		rk_gpio_set_value(g_smartIr_ctx.ircut_on_gpio, 0);
	} else {
		rk_gpio_set_value(g_smartIr_ctx.ircut_off_gpio, 1);
		usleep(100 * 1000);
		rk_gpio_set_value(g_smartIr_ctx.ircut_off_gpio, 0);
	}
#endif
	printf("%s: software-only smart ir mode, skip ircut gpio control, on=%d\n", __func__, on);
}

static void smart_ir_switch_scene(RK_SMART_IR_STATUS_t status) {
	int ret;

	if (!g_smartIr_ctx.aiq_ctx) {
		printf("%s: aiq ctx not ready, skip software scene switch\n", __func__);
		return;
	}

	if (status == RK_SMART_IR_STATUS_NIGHT) {
		ret = rk_aiq_uapi2_sysctl_switch_scene(g_smartIr_ctx.aiq_ctx, "normal", "night");
		if (ret != RK_SUCCESS) {
			printf("%s: switch_scene night fail ret:%#x\n", __func__, ret);
		} else {
			printf("switch scene to normal/night OK\n");
		}
	} else if (status == RK_SMART_IR_STATUS_DAY) {
		ret = rk_aiq_uapi2_sysctl_switch_scene(g_smartIr_ctx.aiq_ctx, "normal", "day");
		if (ret != RK_SUCCESS) {
			printf("%s: switch_scene day fail ret:%#x\n", __func__, ret);
		} else {
			printf("switch scene to normal/day OK\n");
		}
	}
}

static int get_board_info(void) {
#ifdef AOV_FASTBOOT_ENABLE
	struct sensor_init_cfg *sensor_init_param = SAMPLE_COMM_AOV_GetSensorInitParam();
	g_smartIr_ctx.ircut_on_gpio = sensor_init_param->ircut_a.gpio_index;
	g_smartIr_ctx.ircut_off_gpio = sensor_init_param->ircut_b.gpio_index;
	g_smartIr_ctx.irled_pwm_channel = sensor_init_param->led_ir.pwm_channel;
	g_smartIr_ctx.irled_enable_gpio = sensor_init_param->led_ir_enable.gpio_index;
	g_smartIr_ctx.visled_pwm_channel = sensor_init_param->led_white.pwm_channel;
	g_smartIr_ctx.visled_enable_gpio = sensor_init_param->led_white_enable.gpio_index;
#else
	// g_smartIr_ctx.ircut_on_gpio =  74;
	// g_smartIr_ctx.ircut_off_gpio =  75;
	// g_smartIr_ctx.irled_pwm_channel = 3;
	// g_smartIr_ctx.irled_enable_gpio = 72;
    g_smartIr_ctx.ircut_on_gpio =  -1;
	g_smartIr_ctx.ircut_off_gpio =  -1;
	g_smartIr_ctx.irled_pwm_channel = -1;
	g_smartIr_ctx.irled_enable_gpio = -1;
	g_smartIr_ctx.visled_pwm_channel = -1;
	g_smartIr_ctx.visled_enable_gpio = -1;
#endif
	printf("%s: ircut on %d, ircut off %d, irled pwm %d, irled enable %d, wled pwm %d, "
	       "wled enable %d\n",
	       __func__, g_smartIr_ctx.ircut_on_gpio, g_smartIr_ctx.ircut_off_gpio,
	       g_smartIr_ctx.irled_pwm_channel, g_smartIr_ctx.irled_enable_gpio,
	       g_smartIr_ctx.visled_pwm_channel, g_smartIr_ctx.visled_enable_gpio);
	return RK_SUCCESS;
}

static void *smart_ir_thread(void *args) {
	int ret;
	RK_SMART_IR_STATUS_t last_status = RK_SMART_IR_STATUS_DAY;

	printf("%s: thread start!\n", __func__);
	while (!g_smartIr_ctx.thread_is_quit) {
		ret = rk_smart_ir_run(g_smartIr_ctx.ir_ctx, false, &g_smartIr_ctx.result);
		if (ret != RK_SUCCESS) {
			printf("%s: rk_smart_ir_run failed %#X\n", __func__, ret);
			usleep(50000);
			continue;
		}

		// =========新增：状态变化时切换isp night/day子场景=========
		if (g_smartIr_ctx.result.status != last_status) {
			printf("smartir status change: %d -> %d\n", last_status, g_smartIr_ctx.result.status);
			smart_ir_switch_scene(g_smartIr_ctx.result.status);
			last_status = g_smartIr_ctx.result.status;
		}

        usleep(200000);
	}
	printf("%s: thread exit!\n", __func__);
	return NULL;
}
// static void *smart_ir_thread(void *args) {
// 	int ret;
// 	printf("%s: thread start!\n", __func__);
// 	while (!g_smartIr_ctx.thread_is_quit) {
// 		ret = rk_smart_ir_run(g_smartIr_ctx.ir_ctx, false, &g_smartIr_ctx.result);
// 		if (ret != RK_SUCCESS) {
// 			printf("%s: rk_smart_ir_run failed %#X\n", __func__, ret);
// 			continue;
// 		}
// 		// 【可选扩展】如果你后续需要在这里监听 result 状态变化打印昼夜切换事件
// 		// if(g_smartIr_ctx.result.status != g_smartIr_ctx.last_status) {
// 		//     printf("DAY/NIGHT switch detect: %d -> %d\n", g_smartIr_ctx.last_status, g_smartIr_ctx.result.status);
// 		//     g_smartIr_ctx.last_status = g_smartIr_ctx.result.status;
// 		// }
//         usleep(200000);
// 	}
// 	printf("%s: thread exit!\n", __func__);
// 	return NULL;
// }

RK_S32 SAMPLE_COMM_SmartIr_Start(int cam_id) {
	int ret = RK_SUCCESS;
#if defined(AOV_FASTBOOT_ENABLE)
	struct app_param_info *app_param = SAMPLE_COMM_AOV_GetAppParam();
	int rk_night_mode = app_param->night_mode;
	int rk_led_value = app_param->led_value;
	int rk_color_mode = app_param->color_mode;
#else
	int rk_night_mode =  2;
	int rk_led_value = 20;
	int rk_color_mode = 0;//FULL_COLOR_MODE;
#endif
	rk_smart_ir_attr_t attr;

	printf("%s: night mode %d, led value %d, color mode %d\n", __func__, rk_night_mode,
	       rk_led_value, rk_color_mode);
	g_smartIr_ctx.night_mode = rk_night_mode;
	RKADK_MW_PTR aiq_ctx_ptr = NULL;
	if (SAMPLE_ISP_Get_AiqHandle(0, &aiq_ctx_ptr) == 0 && aiq_ctx_ptr) {
		g_smartIr_ctx.aiq_ctx = (rk_aiq_sys_ctx_t *)aiq_ctx_ptr;
	} else {
		printf("%s: aiq ctx is not ready, skip smart ir init now\n", __func__);
		return RK_SUCCESS;
	}
	g_smartIr_ctx.ir_ctx = rk_smart_ir_init(g_smartIr_ctx.aiq_ctx);
	if (!g_smartIr_ctx.ir_ctx) {
		printf("%s: rk_smart_ir_init failed!\n", __func__);
		return RK_FAILURE;
	}

	get_board_info();
	rk_smart_ir_getAttr(g_smartIr_ctx.ir_ctx, &attr);

	if (rk_night_mode == 2) {
		attr.init_status = RK_SMART_IR_STATUS_NIGHT;
		                                             
		attr.switch_mode = RK_SMART_IR_SWITCH_MODE_NIGHT;
		attr.light_mode = RK_SMART_IR_LIGHT_MODE_INVALID;//RK_SMART_IR_LIGHT_MODE_MANUAL;
		attr.light_type = RK_SMART_IR_LIGHT_TYPE_INVALID;//RK_SMART_IR_LIGHT_TYPE_IR;
		attr.light_value = rk_led_value;
		attr.params.d2n_envL_th = 0.04f;
		attr.params.n2d_envL_th = 0.20f;
		attr.params.rggain_base = 1.00f;
		attr.params.bggain_base = 1.00f;
		attr.params.awbgain_rad = 0.10f;
		attr.params.awbgain_dis = 0.20f;
		attr.params.switch_cnts_th = 50;
		ret = rk_smart_ir_setAttr(g_smartIr_ctx.ir_ctx, &attr);
		if (ret != RK_SUCCESS) {
			printf("%s: rk_smart_ir_setAttr failed\n", __func__);
			return ret;
		}
		// =========【核心改动开始】屏蔽所有GPIO导出、PWM初始化、硬件引脚操作 =========
#if 0
		ret |= rk_gpio_export_direction(g_smartIr_ctx.ircut_on_gpio, GPIO_DIRECTION_OUTPUT);
		ret |= rk_gpio_export_direction(g_smartIr_ctx.ircut_off_gpio, GPIO_DIRECTION_OUTPUT);
		ret |= rk_gpio_export_direction(g_smartIr_ctx.irled_enable_gpio, GPIO_DIRECTION_OUTPUT);

		int irled_pwm_period = 10000, irled_pwm_duty = 0;
		irled_pwm_duty = irled_pwm_period * MIN(rk_led_value, 100) / 100;
		g_smartIr_ctx.cur_irled_value = rk_led_value;
#ifndef ROCKIT_LIGHT_CTL
		ret = rk_pwm_init(g_smartIr_ctx.irled_pwm_channel, irled_pwm_period,
		                  irled_pwm_duty, PWM_POLARITY_NORMAL);
		if (ret) {
			printf("%s: rk_pwm_init error ret [%d]\n", __func__, ret);
		}
#endif
#endif
		// 初始状态：不再控制IRcut、IR灯GPIO，仅保存状态变量
		g_smartIr_ctx.last_status = attr.init_status;
		printf("%s: smart ir init status = %d, software-only mode enabled, hardware gpio/pwm control disabled\n",
		       __func__, g_smartIr_ctx.last_status);
		// =========【核心改动结束】 =========
	} else {
		printf("%s: not support night mode %d!\n", __func__, rk_night_mode);
	}
	// 创建检测线程保持运行，持续执行rk_smart_ir_run计算昼夜状态
	g_smartIr_ctx.thread_is_quit = false;
	pthread_create(&g_smartIr_ctx.thread_id, NULL, smart_ir_thread, NULL);
	printf("%s: smart ir init done, only DAY/NIGHT algorithm enabled, hardware control disabled\n", __func__);
	return RK_SUCCESS;
}

RK_S32 SAMPLE_COMM_SmartIr_Stop() {
	// 屏蔽PWM反初始化
#if 0
#ifndef ROCKIT_LIGHT_CTL
	rk_pwm_deinit(g_smartIr_ctx.irled_pwm_channel);
#endif
#endif
	g_smartIr_ctx.thread_is_quit = true;
	pthread_join(g_smartIr_ctx.thread_id, NULL);

	if (g_smartIr_ctx.ir_ctx) {
		rk_smart_ir_deInit(g_smartIr_ctx.ir_ctx);
		g_smartIr_ctx.ir_ctx = NULL;
	}
	return RK_SUCCESS;
}
/*****************************************************************************************/

static int32_t app_init(void) {
    printf("************app_init start**********************\n");
    
#ifdef REMOTE_CONFIG
    remote_cfg_init();
#endif
    
    button_app_init();
    hotplug_reboot_init();
    system_standby_init();

    //mpu_photo_init();
    //mpu_record_init();

    //mpu_record_set_fps(0, RKADK_STREAM_TYPE_VIDEO_MAIN,25);
    //mpu_record_set_gop(0, RKADK_STREAM_TYPE_VIDEO_MAIN,25);
    mpu_record_reset(0);
    mpu_record_init();

#if CVR_RKNN_STREAM_DETECT
	if (rknn_stream_detect_start(CVR_RKNN_MODEL_PATH) != 0)
		printf("RKNN stream detection start failed\n");
#endif
    // rk_gpio_export_direction(GPIO(RK_GPIO0, RK_PA3), GPIO_DIRECTION_INPUT);
    // rk_gpio_set_value(GPIO(RK_GPIO0, RK_PA3), 0);
    return 0;
}

static void app_deinit(void) {
#if CVR_RKNN_STREAM_DETECT
	rknn_stream_detect_stop();
#endif
#ifdef REMOTE_CONFIG
    remote_cfg_deinit();
#endif

    system_standby_deinit();

    hotplug_reboot_deinit();

#ifndef USE_LVGL
    button_app_deinit();
#endif
}



static int32_t app_sleep(void) {
    return 0;
}

static int32_t app_wakeup(void) {
    return 0;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static module_node_t app_module = {
    .priority = MODULE_LEVEL_APP,
    .ops = {
        .init = app_init,
        .deinit = app_deinit,
        .sleep = app_sleep,
        .wakeup = app_wakeup,
    }
};

SYS_MODULE_REGISTER(app_module)