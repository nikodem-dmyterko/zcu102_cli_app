/******************************************************************************
 * (c) Copyright 2012-2018 Xilinx, Inc. All rights reserved.
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

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "vgst_lib.h"
#include "filter.h"
#include "helper.h"

static struct filter_discovery_table disc[] =
{
  {SDX_FILTER2D_PLUGIN, filter2d_create_gst},
  {SDX_OPTICALFLOW_PLUGIN, optical_flow_create_gst},
};

/**
 * filter_type_register - register a filter with vlib
 * @ft:	Pointer to filter table
 * @fs: Pointer to filter struct to register
 *
 * Return: 0 on success, error code otherwise.
 */
int
filter_type_register (struct filter_tbl *ft, struct filter_s *fs)
{
  if (!ft || !fs)
    return VLIB_ERROR_INVALID_PARAM;

  if (!ft->filter_types) {
    ft->filter_types = g_ptr_array_new ();
    if (!ft->filter_types) {
      return VLIB_ERROR_OTHER;
    }
  }

  g_ptr_array_add (ft->filter_types, fs);

  GST_INFO ("Filter %s registered successfully!\n",
      filter_type_get_display_text (fs));

  ft->size = ft->filter_types->len;

  return 0;
}

int
filter_type_unregister (struct filter_tbl *ft, struct filter_s *fs)
{
  if (!ft || !fs)
    return -1;

  g_ptr_array_remove_fast (ft->filter_types, fs);
  ft->size = ft->filter_types->len;

  if (!ft->size) {
    g_ptr_array_free (ft->filter_types, TRUE);
  }

  GST_INFO ("Filter %s unregistered successfully!\n",
      filter_type_get_display_text (fs));

  return ft->size;
}

struct filter_s *
filter_type_get_obj (struct filter_tbl *ft, unsigned int i)
{
  if (ft && i < ft->size)
    return g_ptr_array_index (ft->filter_types, i);

  return NULL;
}

int
filter_type_match (struct filter_s *fs, const char *str)
{
  if (fs && !strcmp (filter_type_get_display_text (fs), str))
    return 1;
  else
    return 0;
}

int
filter_type_set_mode (struct filter_s *fs, size_t mode)
{
  if (!fs || mode >= filter_type_get_num_modes (fs)) {
    return VLIB_ERROR_INVALID_PARAM;
  }

  fs->mode = mode;

  return 0;
}

const char *
filter_type_get_display_text (const struct filter_s *fs)
{
  ASSERT2 (fs, " %s :: argument NULL\n", __func__);
  return fs->display_text;
}


void
filter_init(struct filter_tbl *ft)
{
  int i;
  GstPlugin *loaded_plugin;
  struct filter_s *fs = NULL;

  UNUSED(ft);
  gst_init (NULL, NULL);
  for (i = 0; i < ARRAY_SIZE(disc); i++) {
    loaded_plugin = gst_registry_find_plugin (gst_registry_get (),
           disc[i].name);
    if (loaded_plugin == NULL) {
      GST_DEBUG ("failed to find %s plugin ", disc[i].name);
      continue;
    }
    GST_DEBUG ("plugin %s refcount %d", disc[i].name,
          GST_OBJECT_REFCOUNT_VALUE (loaded_plugin));
    gst_object_unref (loaded_plugin);

    fs = (*(disc[i].create))();
    if (filter_type_register(ft, fs)) {
      GST_ERROR("fail to register filter %s", filter_type_get_display_text(fs));
    }
  }
}

int
is_sdx_plugin_present (const struct filter_tbl *ft, const char *str)
{
  for (size_t i = 0; i < ft->size; i++) {
    struct filter_s *fs = g_ptr_array_index(ft->filter_types, i);
    if (fs && (!strcmp (fs->dt_comp_string, str)))
      return 1;
  }

  return 0;
}
