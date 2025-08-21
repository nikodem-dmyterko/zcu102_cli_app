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


#include <math.h>
#include <string.h>
#include "vgst_pipeline.h"
GST_DEBUG_CATEGORY_EXTERN (vgst_lib);
#define GST_CAT_DEFAULT vgst_lib


VGST_ERROR_LOG
create_pipeline (vgst_ip_params *ip_param, vgst_enc_params *enc_param, vgst_playback *play_ptr, guint sink_type, gchar *uri, vgst_sdx_filter_params *filter_param) {
	printf("create_pipeline: 1\n"); fflush(stdout);
	play_ptr->pipeline        = gst_pipeline_new ("vcu-trd");
    if (ip_param->src_type == FILE_SRC || STREAMING_SRC == ip_param->src_type)
      play_ptr->ip_src          = gst_element_factory_make (FILE_SRC_NAME,    NULL);
    else if (ip_param->src_type == LIVE_SRC)
      play_ptr->ip_src          = gst_element_factory_make (V4L2_SRC_NAME,    NULL);
    printf("create_pipeline: 2\n"); fflush(stdout);
    play_ptr->srccapsfilter   = gst_element_factory_make ("capsfilter",     NULL);
    printf("create_pipeline: 3\n"); fflush(stdout);
    play_ptr->queue           = gst_element_factory_make ("queue",          NULL);
    play_ptr->enc_queue       = gst_element_factory_make ("queue",          NULL);
    play_ptr->enccapsfilter   = gst_element_factory_make ("capsfilter",     NULL);
    if ((ip_param->filter_type == SDX_FILTER) && (ip_param->src_type == FILE_SRC))
      play_ptr->srccapsfilter  = gst_element_factory_make ("videoparse",  NULL);
    if (sink_type == DISPLAY) {
      play_ptr->videosink       = gst_element_factory_make (KMS_SINK_NAME,       NULL);
      play_ptr->fpsdisplaysink  = gst_element_factory_make ("fpsdisplaysink",NULL);
    } else if (sink_type == RECORD) {
      play_ptr->videosink       = gst_element_factory_make ("filesink",     NULL);
      if (strstr(uri, MP4_MUX_TYPE) != NULL) {
        GST_DEBUG ("MP4 mux type\n");
        play_ptr->mux           = gst_element_factory_make ("qtmux",        NULL);
      } else if (strstr(uri, MKV_MUX_TYPE) != NULL) {
        GST_DEBUG ("MKV mux type\n");
        play_ptr->mux           = gst_element_factory_make ("matroskamux",        NULL);
      } else if (strstr(uri, TS_MUX_TYPE) != NULL) {
        GST_DEBUG ("TS mux type\n");
        play_ptr->mux           = gst_element_factory_make ("mpegtsmux",        NULL);
      } else {
        GST_DEBUG ("MP4 mux type\n");
        play_ptr->mux           = gst_element_factory_make ("qtmux",        NULL);
      }

    } else if (sink_type == STREAM) {
      play_ptr->stream_sink     = gst_element_factory_make ("udpsink",      NULL);
      play_ptr->tee             = gst_element_factory_make ("tee",          NULL);
    }

    if (!play_ptr->pipeline || !play_ptr->ip_src || !play_ptr->srccapsfilter || !play_ptr->queue || !play_ptr->enc_queue || !play_ptr->enccapsfilter) {
      GST_ERROR ("FAILED to create common element");
      return VGST_ERROR_PIPELINE_CREATE_FAIL;
    } else {
      GST_DEBUG ("All common elements are created");
    }
    gst_bin_add_many (GST_BIN(play_ptr->pipeline), play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->queue, play_ptr->enc_queue,
                      play_ptr->enccapsfilter, NULL);

    if (sink_type == STREAM) {
      if (!play_ptr->tee || !play_ptr->stream_sink) {
        GST_ERROR ("FAILED to create stream-out elements");
        return VGST_ERROR_PIPELINE_CREATE_FAIL;
      } else {
        GST_DEBUG ("All stream-out elements are created");
      }
      gst_bin_add_many (GST_BIN(play_ptr->pipeline), play_ptr->tee, play_ptr->stream_sink, NULL);
    } else if (sink_type == RECORD) {
      if (!play_ptr->videosink || !play_ptr->mux) {
        GST_ERROR ("FAILED to create record elements");
        return VGST_ERROR_PIPELINE_CREATE_FAIL;
      } else {
        GST_DEBUG ("All record elements are created");
      }
      gst_bin_add_many (GST_BIN(play_ptr->pipeline), play_ptr->videosink, play_ptr->mux, NULL);
    } else if (sink_type == DISPLAY) {
      if (!play_ptr->videosink || !play_ptr->fpsdisplaysink) {
        GST_ERROR ("FAILED to create display elements");
        return VGST_ERROR_PIPELINE_CREATE_FAIL;
      } else {
        GST_DEBUG ("All display elements are created");
      }
      gst_bin_add_many (GST_BIN(play_ptr->pipeline), play_ptr->fpsdisplaysink, NULL);
    }

    if (!ip_param->raw && (ip_param->filter_type == SDX_FILTER)) {
       play_ptr->videofilter = gst_element_factory_make (filter_param->filter_name, NULL);
		if ( !strcmp (filter_param->filter_name, "sdxfilter2d")) {
			g_object_set (G_OBJECT (play_ptr->videofilter), "filter-kernel", "filter2d_pl_accel", NULL );
		}
      if (!play_ptr->videofilter) {
        GST_ERROR ("FAILED to create videofilter elements");
        return VGST_ERROR_PIPELINE_CREATE_FAIL;
      }
      gst_bin_add_many(GST_BIN(play_ptr->pipeline), play_ptr->videofilter, NULL);
    }
    if ((ip_param->filter_type != SDX_FILTER) && !ip_param->raw && LIVE_SRC == ip_param->src_type) {
      if (enc_param->enc_type == AVC) {
          play_ptr->videoenc    = gst_element_factory_make (H264_ENC_NAME, NULL);
          play_ptr->videodec    = gst_element_factory_make (H264_DEC_NAME,          NULL);
          play_ptr->videoparser = gst_element_factory_make (H264_PARSER_NAME,       NULL);
      } else if (enc_param->enc_type == HEVC) {
        play_ptr->videoenc    = gst_element_factory_make (H265_ENC_NAME, NULL);
        play_ptr->videodec    = gst_element_factory_make (H265_DEC_NAME,          NULL);
        play_ptr->videoparser = gst_element_factory_make (H265_PARSER_NAME,       NULL);
      }
      play_ptr->rtppay = gst_element_factory_make (MPEG_TS_RTP_PAYLOAD_NAME,  NULL);
      play_ptr->mpegtsmux      = gst_element_factory_make (MPEG_TS_MUX_NAME,  NULL);
      if (!play_ptr->videoenc || !play_ptr->videodec || !play_ptr->videoparser || !play_ptr->rtppay || !play_ptr->mpegtsmux) {
        GST_ERROR ("FAILED to create enc-dec-parser elements");
        return VGST_ERROR_PIPELINE_CREATE_FAIL;
      }
      if (!play_ptr->rtppay || !play_ptr->mpegtsmux) {
        GST_ERROR ("FAILED to create streaming elements");
        return VGST_ERROR_PIPELINE_CREATE_FAIL;
      }
      gst_bin_add_many(GST_BIN(play_ptr->pipeline), play_ptr->videoenc, play_ptr->videodec, play_ptr->videoparser, play_ptr->rtppay, play_ptr->mpegtsmux, NULL);
    }
    return VGST_SUCCESS;
}

void
fetch_tag (const GstTagList * list, const gchar * tag, gpointer user_data) {
    uint br =0;
    vgst_playback *play_ptr = (vgst_playback *)user_data;
    if (!play_ptr->file_br && gst_tag_list_get_uint (list, "bitrate", &br)) {
      play_ptr->file_br = br;
      GST_DEBUG ("Average bit rate value : %u", play_ptr->file_br);
    }
}




void
on_fps_measurement (GstElement *fps,
        gdouble fps_value,
        gdouble drop_rate,
        gdouble avg_rate,
        gpointer   data) {
    guint *fps_num = (guint *)data;
    *fps_num = round(fps_value);
}


void
set_property (vgst_application *app, gint index) {
    vgst_ip_params *ip_param = &app->ip_params[index];
    vgst_playback *play_ptr = &app->playback[index];
    vgst_enc_params *enc_param = &app->enc_params[index];
    vgst_cmn_params *cmn_param = app->cmn_params;
    vgst_op_params *op_param = &app->op_params[index];
    vgst_sdx_filter_params *filter_param = &app->filter_params[index];
    guint dec_buffer_cnt = DEFAULT_DEC_BUFFER_CNT/cmn_param->num_src;
    dec_buffer_cnt = dec_buffer_cnt < MIN_DEC_BUFFER_CNT ? MIN_DEC_BUFFER_CNT : dec_buffer_cnt;
    GstCaps *srcCaps = NULL, *encCaps = NULL;
    if (STREAMING_SRC == ip_param->src_type) {
      GST_DEBUG ("buffer-size is %d", enc_param->bitrate * 1000);
      g_object_set (G_OBJECT(play_ptr->ip_src), "buffer-size", enc_param->bitrate * 1000, NULL);
      g_object_set (G_OBJECT(play_ptr->ip_src), "use-buffering", TRUE, NULL);
    }
    if (LIVE_SRC == ip_param->src_type) {
      g_object_set (G_OBJECT(play_ptr->ip_src), "io-mode", VGST_V4L2_IO_MODE_DMABUF_EXPORT, NULL);
      if (!ip_param->raw && (ip_param->filter_type == SDX_FILTER))
        g_object_set (G_OBJECT(play_ptr->ip_src), "io-mode", ip_param->io_mode, NULL);
      if (!g_strcmp0 (V4L2_SRC_NAME, XLNX_V4L2_SRC_NAME))
        gst_util_set_object_arg (G_OBJECT(play_ptr->ip_src), "src-type", vgst_get_srctype(ip_param->device_type));
      else
        g_object_set (G_OBJECT(play_ptr->ip_src), "device", vlib_get_devname(ip_param->device_type), NULL);
      if ((ip_param->filter_type == SDX_FILTER) && !ip_param->raw) {
        g_object_set (G_OBJECT (play_ptr->videofilter),  "filter-mode",       filter_param->filter_mode, NULL );
      }
      if ((ip_param->filter_type != SDX_FILTER) && !ip_param->raw) {
        g_object_set (G_OBJECT (play_ptr->videoenc),  "gop-length",       enc_param->gop_len,       NULL);
        g_object_set (G_OBJECT (play_ptr->videoenc),  "gop-mode",         enc_param->gop_mode,      NULL);
        g_object_set (G_OBJECT (play_ptr->videoenc),  "low-bandwidth",    enc_param->low_bandwidth, NULL);
        g_object_set (G_OBJECT (play_ptr->videoenc),  "target-bitrate",   enc_param->bitrate,       NULL);
        g_object_set (G_OBJECT (play_ptr->videoenc),  "b-frames",         enc_param->b_frame,       NULL);
        g_object_set (G_OBJECT (play_ptr->videoenc),  "num-slices",       enc_param->slice,         NULL);
        g_object_set (G_OBJECT (play_ptr->videoenc),  "control-rate",     enc_param->rc_mode,       NULL);
        g_object_set (G_OBJECT (play_ptr->videoenc),  "qp-mode",          enc_param->qp_mode,       NULL);
        g_object_set (G_OBJECT (play_ptr->videoenc),  "periodicity-idr",  enc_param->gop_len,       NULL );

        if (enc_param->enable_l2Cache) {
          g_object_set (G_OBJECT (play_ptr->videoenc),  "prefetch-buffer",  enc_param->enable_l2Cache, NULL);
        }
        if (enc_param->rc_mode == CBR)
          g_object_set (G_OBJECT (play_ptr->videoenc),  "filler-data",      enc_param->filler_data,   NULL);
        g_object_set (G_OBJECT (play_ptr->videoenc),  "latency-mode",      enc_param->latency_mode,   NULL);
        if (SUB_FRAME_LATENCY == enc_param->latency_mode) {
          g_object_set (G_OBJECT (play_ptr->videodec),  "latency-mode",     DEC_SUB_FRAME_LATENCY_MODE,   NULL);
        }
        GST_DEBUG ("Setting internal-entropy-buffers to [%d]", dec_buffer_cnt);
        g_object_set (G_OBJECT (play_ptr->videodec),  "internal-entropy-buffers",  dec_buffer_cnt,  NULL );

        gchar * profile;
        profile = enc_param->profile == BASELINE_PROFILE ? "constrained-baseline" : enc_param->profile == MAIN_PROFILE ? "main" : "high";
        if (AVC == enc_param->enc_type) {
          encCaps  = gst_caps_new_simple ("video/x-h264",
                                          "profile",   G_TYPE_STRING,     profile,
                                          NULL);
        } else if (HEVC == enc_param->enc_type) {
          encCaps  = gst_caps_new_simple ("video/x-h265",
                                          "profile",   G_TYPE_STRING,     profile,
                                          NULL);
        }
        GST_DEBUG ("new Caps for enc capsfilter %" GST_PTR_FORMAT, encCaps);
        g_object_set (G_OBJECT (play_ptr->enccapsfilter),  "caps",  encCaps, NULL);
        gst_caps_unref (encCaps);
      }
    }
    if (ip_param->filter_type == SDX_FILTER) {
	srcCaps = gst_caps_new_full (
		gst_structure_new ("video/x-raw",
                           "width",     G_TYPE_INT,        ip_param->width,
                           "height",    G_TYPE_INT,        ip_param->height,
                           "format",    G_TYPE_STRING,     ip_param->format_str,
                           "framerate", GST_TYPE_FRACTION, cmn_param->frame_rate, MAX_FRAME_RATE_DENOM,
                           NULL),
                gst_structure_new ("video/x-raw",
			   "width",     G_TYPE_INT,        ip_param->width,
			   "height",    G_TYPE_INT,        ip_param->height,
			   "format",    G_TYPE_STRING,     ip_param->format_str,
			   NULL),
		NULL);
    }
    if (ip_param->filter_type != SDX_FILTER) {
      gchar * format;
      format = ip_param->format == NV16 ? "NV16" : "NV12";
      srcCaps  = gst_caps_new_simple ("video/x-raw",
                           "width",     G_TYPE_INT,        ip_param->width,
                           "height",    G_TYPE_INT,        ip_param->height,
                           "format",    G_TYPE_STRING,     format,
                           "framerate", GST_TYPE_FRACTION, cmn_param->frame_rate, MAX_FRAME_RATE_DENOM,
                           NULL);
    }
    if (!g_strcmp0 (ip_param->src, FILE_SRC_NAME) && ip_param->filter_type == SDX_FILTER) {
      g_object_set (G_OBJECT (play_ptr->srccapsfilter),  "format",  FILE_SRC_FORMAT, NULL);
      g_object_set (G_OBJECT (play_ptr->srccapsfilter),  "width",   ip_param->width, NULL);
      g_object_set (G_OBJECT (play_ptr->srccapsfilter),  "height",  ip_param->height, NULL);
      gst_caps_unref (srcCaps);
    } else {
      GST_DEBUG ("new Caps for src capsfilter %" GST_PTR_FORMAT, srcCaps);
      g_object_set (G_OBJECT (play_ptr->srccapsfilter),  "caps",  srcCaps, NULL);
      gst_caps_unref (srcCaps);
    }

    if ((ip_param->filter_type == SDX_FILTER) && !g_strcmp0 (ip_param->src, FILE_SRC_NAME)) {
      g_object_set (G_OBJECT(play_ptr->ip_src), "location", ip_param->uri, NULL);
      if (!ip_param->raw) {
        g_object_set (G_OBJECT (play_ptr->videofilter),  "filter-mode", filter_param->filter_mode, NULL );
      }
    }

    if (cmn_param->sink_type == RECORD) {
      g_object_set (G_OBJECT (play_ptr->videosink), "location",    op_param->file_out, NULL);
      g_object_set (G_OBJECT (play_ptr->ip_src),    "num-buffers", op_param->duration*cmn_param->frame_rate*GST_MINUTE, NULL);
    } else if (cmn_param->sink_type == DISPLAY) {
      g_object_set (G_OBJECT (play_ptr->fpsdisplaysink), "fps-update-interval",     FPS_UPDATE_INTERVAL, NULL);
      g_object_set (G_OBJECT (play_ptr->fpsdisplaysink), "signal-fps-measurements", TRUE, NULL);
      g_object_set (G_OBJECT (play_ptr->fpsdisplaysink), "text-overlay",            FALSE, NULL);
      if (ip_param->filter_type == SDX_FILTER)
        g_object_set (G_OBJECT (play_ptr->fpsdisplaysink), "sync",                    FALSE, NULL);
      if (!g_strcmp0 (KMS_SINK_NAME, XLNX_KMS_SINK_NAME)) {
        gst_util_set_object_arg (G_OBJECT(play_ptr->videosink), "sink-type", vgst_get_sinkname(cmn_param->driver_type));
        g_object_set (G_OBJECT (play_ptr->videosink), "fullscreen-overlay", FALSE, NULL);
      } else if (cmn_param->driver_type == DP) {
        g_object_set (G_OBJECT (play_ptr->videosink), "bus-id",                DP_BUS_ID, NULL);
      } else if (cmn_param->driver_type == HDMI_Tx || cmn_param->driver_type == SDI_Tx) {
        g_object_set (G_OBJECT (play_ptr->videosink), "bus-id",                MIXER_BUS_ID, NULL);
      }
      g_object_set (G_OBJECT (play_ptr->fpsdisplaysink), "video-sink",         play_ptr->videosink, NULL);
      g_signal_connect (play_ptr->fpsdisplaysink,        "fps-measurements",   G_CALLBACK (on_fps_measurement), &play_ptr->fps_num[0]);
      cmn_param->plane_id++;
    } else if (cmn_param->sink_type == STREAM) {
      g_object_set (G_OBJECT (play_ptr->mpegtsmux), "alignment",     PKT_NUMBER_PER_BUFFER, NULL);
      g_object_set (G_OBJECT (play_ptr->stream_sink), "host",        op_param->host_ip, NULL);
      /* bitrate conversion from Kbps to bps needs multiplication of 1000 */
      g_object_set (G_OBJECT (play_ptr->stream_sink), "max-bitrate", enc_param->bitrate * 1000, NULL);
      g_object_set (G_OBJECT (play_ptr->stream_sink), "port",        op_param->port_num, NULL);
      g_object_set (G_OBJECT (play_ptr->stream_sink), "qos-dscp",    QOS_DSCP_VALUE, NULL);
      g_object_set (G_OBJECT (play_ptr->stream_sink), "async",       FALSE, NULL);
      g_object_set (G_OBJECT (play_ptr->stream_sink), "buffer-size", 20000000, NULL);
      g_object_set (G_OBJECT (play_ptr->stream_sink), "max-lateness",-1, NULL);
    }
    if (play_ptr->queue) {
      g_object_set (G_OBJECT (play_ptr->queue), "max-size-bytes", 0, NULL);
    }
}




void
on_pad_added (GstElement *element,
              GstPad     *pad,
              gpointer   data) {
    gchar *str;
    vgst_playback *play_ptr = (vgst_playback *)data;
    GstCaps *caps = gst_pad_get_current_caps (pad);
    str = gst_caps_to_string(caps);
    if (g_str_has_prefix (str, "video/")) {
      if (!gst_element_link_many(play_ptr->ip_src, play_ptr->queue, play_ptr->fpsdisplaysink, NULL)) {
        GST_ERROR ("Error linking for ip_src --> queue --> fpsdisplaysink");
      } else {
        GST_DEBUG ("Linked for ip_src --> queue --> fpsdisplaysink successfully");
      }
    }
    g_free (str);
    gst_caps_unref (caps);
}


VGST_ERROR_LOG
link_streaming_pipeline (vgst_playback *play_ptr) {
    gint ret = VGST_SUCCESS;

    if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->videoenc, play_ptr->enccapsfilter, \
                                play_ptr->mpegtsmux, play_ptr->rtppay, play_ptr->enc_queue, play_ptr->stream_sink, NULL)) {
      GST_ERROR ("Error linking for ip_src --> srccapsfilter --> videoenc --> enccapsfilter --> mpegtsmux --> rtppay --> enc_queue --> stream_sink");
      ret = VGST_ERROR_PIPELINE_LINKING_FAIL;
    } else {
      GST_DEBUG ("Linked for ip_src --> srccapsfilter --> videoenc --> enccapsfilter --> mpegtsmux --> rtppay --> enc_queue --> stream_sink successfully");
    }

    return ret;
}


VGST_ERROR_LOG
link_elements (vgst_ip_params *ip_param, vgst_playback *play_ptr, gint sink_type, gint latency_mode) {
    if ((ip_param->filter_type != SDX_FILTER) && (FILE_SRC == ip_param->src_type || STREAMING_SRC == ip_param->src_type)) {
      g_object_set (G_OBJECT(play_ptr->ip_src), "uri", ip_param->uri, NULL);
      g_signal_connect (play_ptr->ip_src, "pad-added", G_CALLBACK (on_pad_added), play_ptr);
      return VGST_SUCCESS;
    }

    if (ip_param->raw == FALSE && (ip_param->filter_type == SDX_FILTER)) {
      if (sink_type == DISPLAY) {
        if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->videofilter, play_ptr->fpsdisplaysink, NULL)) {
          GST_ERROR ("Error linking for ip_src --> capsfilter --> videofilter --> fpsdisplaysink");
          return VGST_ERROR_PIPELINE_LINKING_FAIL;
        } else {
          GST_DEBUG ("Linked for ip_src --> capsfilter --> videofilter --> queue --> fpsdisplaysink successfully");
        }
      }
    } else if (ip_param->raw == FALSE) {
      if (sink_type == DISPLAY) {
        {
          if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->videoenc, play_ptr->enccapsfilter, play_ptr->enc_queue,
                                      play_ptr->videodec, play_ptr->queue, play_ptr->fpsdisplaysink, NULL)) {
            GST_ERROR ("Error linking for ip_src --> capsfilter --> videoenc --> enccapsfilter --> queue --> videodec --> queue --> fpsdisplaysink");
            return VGST_ERROR_PIPELINE_LINKING_FAIL;
          } else {
            GST_DEBUG ("Linked for ip_src --> capsfilter --> videoenc --> enccapsfilter --> queue --> videodec --> queue --> fpsdisplaysink successfully");
          }
        }
      } else if (sink_type == STREAM) {
        if ( VGST_SUCCESS != link_streaming_pipeline (play_ptr) )
          return VGST_ERROR_PIPELINE_LINKING_FAIL;
      } else if (sink_type == RECORD) {
        if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->videoenc, play_ptr->enc_queue, play_ptr->enccapsfilter,
                                    play_ptr->videoparser, play_ptr->mux, play_ptr->videosink, NULL)) {
          GST_ERROR ("Error linking for ip_src --> capsfilter --> videoenc --> queue --> capsfilter --> videoparser --> mux --> videosink");
          return VGST_ERROR_PIPELINE_LINKING_FAIL;
        } else {
          GST_DEBUG ("Linked for ip_src --> capsfilter --> videoenc --> queue --> capsfilter --> videoparser --> mux --> videosink successfully");
        }
      }
    } else {
      // It Comes here means need to use raw path means src --> sink path
      if (!gst_element_link_many (play_ptr->ip_src, play_ptr->srccapsfilter, play_ptr->fpsdisplaysink, NULL)) {
        GST_ERROR ("Error linking for testsrc --> capsfilter --> fpsdisplaysink");
        return VGST_ERROR_PIPELINE_LINKING_FAIL;
      } else {
        GST_DEBUG ("Linked for ip_src --> capsfilter --> fpsdisplaysink successfully");
      }
    }
    return VGST_SUCCESS;
}


void
get_coordinates (guint *x, guint *y, guint cnt) {
    if (cnt == 0) {
      *x = 0;
      *y = 0;
    } else if (cnt == 1) {
      *x = 1920;
      *y = 0;
    } else if (cnt == 2) {
      *x = 0;
      *y = 1080;
    } else if (cnt == 3) {
      *x = 1920;
      *y = 1080;
    }
}
