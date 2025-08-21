/******************************************************************************
 * (c) Copyright 2017-2018 Xilinx, Inc. All rights reserved.
 *
 * This file contains confidential and proprietary information of Xilinx, Inc.
 * and is protected under U.S. and international copyright and other
 * intellectual property laws.
 *
 * DISCLAIMER
 * This disclaimer is not a license and does not grant any rights to the
 * materials distributed herewith. Except as otherwise provided in a valid
 * license issued to you by Xilinx, and to the maximum extent permitted by
 * applicable law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL
 * FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS,
 * IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF
 * MERCHANTABILITY, NON-INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE;
 * and (2) Xilinx shall not be liable (whether in contract or tort, including
 * negligence, or under any other theory of liability) for any loss or damage
 * of any kind or nature related to, arising under or in connection with these
 * materials, including for any direct, or any indirect, special, incidental,
 * or consequential loss or damage (including loss of data, profits, goodwill,
 * or any type of loss or damage suffered as a result of any action brought by
 * a third party) even if such damage or loss was reasonably foreseeable or
 * Xilinx had been advised of the possibility of the same.
 *
 * CRITICAL APPLICATIONS
 * Xilinx products are not designed or intended to be fail-safe, or for use in
 * any application requiring fail-safe performance, such as life-support or
 * safety devices or systems, Class III medical devices, nuclear facilities,
 * applications related to the deployment of airbags, or any other applications
 * that could lead to death, personal injury, or severe property or
 * environmental damage (individually and collectively, "Critical
 * Applications"). Customer assumes the sole risk and liability of any use of
 * Xilinx products in Critical Applications, subject only to applicable laws
 * and regulations governing limitations on product liability.
 *
 * THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE
 * AT ALL TIMES.
 *******************************************************************************/


#ifndef INCLUDE_VGST_LIB_H_
#define INCLUDE_VGST_LIB_H_

#include <sys/stat.h>
#include <string.h>
#include "vgst_err.h"
#include <gst/gst.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define FILE_SRC_FORMAT		4
#define GST_FILTER_MODE_HW	1
#define GST_FILTER_MODE_SW	0
#define INPUT_PIX_FMT  v4l2_fourcc('Y','U','Y','V')
#define SDX_FILTER2D_PLUGIN "sdxfilter2d"
#define SDX_OPTICALFLOW_PLUGIN "sdxopticalflow"

#ifdef BASE_TRD
  /*
   * Adding below enum/macros for compilation purpose only. Will remove it
   * once we use same vlib library for VCU and SDX filters
   */
#include "video.h"
#include "filter.h"
#include "vgst_sdxfilter2d.h"
#define vlib_src_init()		0
#define vlib_src_uninit		vlib_uninit_gst
#define vlib_src_config(x, y)	0
#define vlib_get_devname	vlib_video_src_get_vdev_from_id
#define vlib_error	vcu_vlib_error
  typedef enum
  {
    TPG_1 = 1,
    TPG_2,
    HDMI_1,
    HDMI_2,
    HDMI_3,
    CSI,
    SDI,
  } vlib_dev_type;

  typedef enum
  {
    VLIB_ERROR_INIT = -50,
    VLIB_ERROR_DEINIT = -51,
    VLIB_ERROR_SRC_CONFIG = -52,
    VLIB_ERROR_HDMIRX_INVALID_STATE = -53,
    VLIB_ERROR_HDMIRX_INVALID_RES = -54,
    VLIB_ERROR_HDMIRX_INVALID_FPS = -55,
    VLIB_ERROR_HDMIRX_INVALID_SEQUENCE = -56,
    VLIB_ERROR_SET_FPS = -57,
    VLIB_ERROR_MIPI_CONFIG_FAILED = -58,
    VLIB_ERROR_INVALID_STATE = -59,
    VLIB_ERROR_MIPI_NOT_CONNECTED = -60,
    VLIB_ERROR_INVALID_RESOLUTION = -61,
    VLIB_ERROR_SET_FORMAT_FAILED = -62,
    VLIB_ERROR_GET_FORMAT_FAILED = -63,
  } vlib_error;
#endif

typedef struct
_vgst_sdx_filter_params {
  gchar      *filter_name;
  gint       filter_mode;
} vgst_sdx_filter_params;

typedef struct
_vgst_enc_params {
    gboolean   enable_l2Cache;
    gboolean   low_bandwidth;
    gboolean   filler_data;
    guint      bitrate;
    guint      gop_len;
    guint      b_frame;
    guint      slice;
    guint      qp_mode;
    guint      rc_mode;
    guint      profile;
    guint      enc_type;
    guint      gop_mode;
    guint      latency_mode;
} vgst_enc_params;


typedef struct
_vgst_input_param {
    gchar      *src;
    gchar      *uri;
    gchar      *format_str;
    gboolean   raw;
    guint      width;
    guint      height;
    guint      src_type;
    guint      device_type;
    gint       filter_type;
    gint       io_mode;
    guint      format;
} vgst_ip_params;


typedef struct
_vgst_output_param {
    gchar      *file_out;
    gchar      *host_ip;
    guint      duration;
    guint      port_num;
} vgst_op_params;

typedef struct
_vgst_cmn_param {
    guint      num_src;
    guint      sink_type;
    guint      driver_type;
    guint      plane_id;
    guint      frame_rate;
} vgst_cmn_params;


typedef enum {
    STREAM,
    RECORD,
    DISPLAY,
    SPLIT_SCREEN,
} VGST_SINK_TYPE;


typedef enum {
    UNIFORM,
    ROI,
    AUTO,
} VGST_QP_MODE;


typedef enum {
    CONST_QP,
    VBR,
    CBR,
    LOW_LATENCY = 0x7F000001,
} VGST_RC_MODE;

typedef enum {
    BASIC,
    LOW_DELAY_P = 3,
    LOW_DELAY_B,
} VGST_GOP_MODE;


typedef enum {
    BASELINE_PROFILE,
    MAIN_PROFILE,
    HIGH_PROFILE,
} VGST_PROFILE_MODE;

typedef enum {
    DP,
    HDMI_Tx,
    SDI_Tx,
} VGST_DRIVER_TYPE;

typedef enum {
    LIVE_SRC,
    FILE_SRC,
    STREAMING_SRC,
} VGST_SRC_TYPE;

typedef enum {
    AVC,
    HEVC,
} VGST_CODEC_TYPE;

typedef enum {
    NV12,
    NV16,
} VGST_FORMAT_TYPE;


typedef enum {
    NORMAL_LATENCY,
    SUB_FRAME_LATENCY,
} VGST_LOW_LATENCY_MODE;

typedef enum {
    EVENT_NONE,
    EVENT_EOS,
    EVENT_ERROR,
} VGST_EVENT_TYPE;

typedef enum {
  VCU,
  SDX_FILTER,
} VGST_FILTER_TYPE;

/* This API is to initialize the library */
gint vgst_init(void);

/* This API is to initialize the options to initiate the pipeline */
gint vgst_config_options (vgst_enc_params *enc_param, vgst_ip_params *ip_param, vgst_op_params *op_param, vgst_cmn_params *cmn_param, vgst_sdx_filter_params *filter_param);

/* This API is to start the pipeline */
gint vgst_start_pipeline (void);

/* This API is interface to stop the single/multi-stream pipeline */
gint vgst_stop_pipeline (void);

/* This API is to convert error number to string */
const gchar * vgst_error_to_string (VGST_ERROR_LOG error_code, gint index);

/* This API is to get fps of the pipeline */
void vgst_get_fps (guint index, guint *fps);

/* This API is to get bitrate for file playback */
guint vgst_get_bitrate (int index);

/* This API is to poll events */
gint vgst_poll_event (int *arg, int index);

/* This API is to un-initialize the library */
gint vgst_uninit(void);

#ifdef BASE_TRD
gint vgst_init_base (struct vlib_config_data *cfg,
    vgst_enc_params * enc_param, vgst_ip_params * input_param,
    vgst_op_params * output_param, vgst_cmn_params * cmn_param,
    vgst_sdx_filter_params * filter_param, struct filter_tbl *ft);
gint vgst_video_src_init (struct vlib_config_data *cfg);
gint vgst_stop_pipeline_base (void);
gint vgst_change_mode (struct vlib_config *config, unsigned int flags,
    vgst_enc_params * enc_param, vgst_ip_params * input_param,
    vgst_op_params * output_param, vgst_cmn_params * cmn_param,
    vgst_sdx_filter_params * filter_param);
int vgst_set_event_log (int state);
int vgst_get_active_height (void);
int vgst_get_active_width (void);
float vgst_get_event_cnt (pipeline_event event);
const char* vgst_error_name (vlib_error error_code);
#endif

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_VGST_LIB_H_ */
