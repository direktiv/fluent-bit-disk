/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/**
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2020 vorteil.io Pty Ltd
 */

/*  Fluent Bit
 *  ==========
 *  Copyright (C) 2019      The Fluent Bit Authors
 *  Copyright (C) 2015-2018 Treasure Data Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <fluent-bit/flb_config.h>
#include <fluent-bit/flb_info.h>
#include <fluent-bit/flb_input.h>
#include <fluent-bit/flb_pack.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <msgpack.h>

#include <sys/statvfs.h>

struct flb_input_plugin in_vdisk_plugin;

#define DEFAULT_INTERVAL_SEC 1
#define DEFAULT_INTERVAL_NSEC 0

int in_vdisk_collect(struct flb_input_instance *i_ins,
                     struct flb_config *config, void *in_context);
static int in_vdisk_init(struct flb_input_instance *in,
                         struct flb_config *config, void *data);

struct flb_in_vdisk_config {
  char partition[128];
  int coll_fd;
  int interval_sec;  /* interval collection time (Second) */
  int interval_nsec; /* interval collection time (Nanosecond) */
  struct flb_input_instance *i_ins;
};

static int in_vdisk_init(struct flb_input_instance *in,
                         struct flb_config *config, void *data) {

  int ret;
  struct flb_in_vdisk_config *ctx;
  const char *pval = NULL;

  /* Allocate space for the configuration */
  ctx = flb_calloc(1, sizeof(struct flb_in_vdisk_config));
  if (!ctx) {
    perror("calloc");
    return -1;
  }

  ctx->i_ins = in;

  pval = flb_input_get_property("partition", in);
  if (pval != NULL) {
    strcpy(ctx->partition, pval);
  } else {
    strcpy(ctx->partition, "/");
  }

  pval = flb_input_get_property("interval_nsec", in);
  if (pval != NULL && atoi(pval) >= 0) {
    ctx->interval_nsec = atoi(pval);
  } else {
    ctx->interval_nsec = DEFAULT_INTERVAL_NSEC;
  }

  pval = flb_input_get_property("interval_sec", in);
  if (pval != NULL && atoi(pval) >= 0) {
    ctx->interval_sec = atoi(pval);
  } else {
    ctx->interval_sec = DEFAULT_INTERVAL_SEC;
  }

  if (ctx->interval_sec <= 0 && ctx->interval_nsec <= 0) {
    ctx->interval_sec = DEFAULT_INTERVAL_SEC;
    ctx->interval_nsec = DEFAULT_INTERVAL_NSEC;
  }

  flb_input_set_context(in, ctx);

  ret = flb_input_set_collector_time(in, in_vdisk_collect, ctx->interval_sec,
                                     ctx->interval_nsec, config);
  if (ret == -1) {
    flb_error("[in_vdisk] Could not set collector for vdisk input plugin");
    return -1;
  }
  ctx->coll_fd = ret;

  return 0;
}

int in_vdisk_collect(struct flb_input_instance *i_ins,
                     struct flb_config *config, void *in_context) {

  struct statvfs sts;
  int ret;
  struct flb_in_vdisk_config *ctx = in_context;
  msgpack_packer mp_pck;
  msgpack_sbuffer mp_sbuf;
  double freeb, availb, freei, availi;

  ret = statvfs(ctx->partition, &sts);
  if (ret < 0) {
    return ret;
  }

  msgpack_sbuffer_init(&mp_sbuf);
  msgpack_packer_init(&mp_pck, &mp_sbuf, msgpack_sbuffer_write);

  /* Pack data */
  msgpack_pack_array(&mp_pck, 2);
  flb_pack_time_now(&mp_pck);
  msgpack_pack_map(&mp_pck, 6);

  freeb = sts.f_bfree * sts.f_bsize;
  availb = sts.f_blocks * sts.f_bsize;

  freei = sts.f_ffree;
  availi = sts.f_files;

  msgpack_pack_str(&mp_pck, 11);
  msgpack_pack_str_body(&mp_pck, "bytes_total", 11);
  msgpack_pack_long(&mp_pck, availb);

  msgpack_pack_str(&mp_pck, 10);
  msgpack_pack_str_body(&mp_pck, "bytes_free", 10);
  msgpack_pack_long(&mp_pck, freeb);

  msgpack_pack_str(&mp_pck, 9);
  msgpack_pack_str_body(&mp_pck, "bytes_pct", 9);
  msgpack_pack_double(&mp_pck, 100 - ((freeb / availb) * 100));

  msgpack_pack_str(&mp_pck, 12);
  msgpack_pack_str_body(&mp_pck, "inodes_total", 12);
  msgpack_pack_long(&mp_pck, freei);

  msgpack_pack_str(&mp_pck, 11);
  msgpack_pack_str_body(&mp_pck, "inodes_free", 11);
  msgpack_pack_long(&mp_pck, availi);

  msgpack_pack_str(&mp_pck, 10);
  msgpack_pack_str_body(&mp_pck, "inodes_pct", 10);
  msgpack_pack_double(&mp_pck, 100 - ((freei / availi) * 100));

  flb_input_chunk_append_raw(i_ins, FLB_INPUT_LOGS, 0, i_ins->tag, i_ins->tag_len, mp_sbuf.data, mp_sbuf.size);
  msgpack_sbuffer_destroy(&mp_sbuf);

  return 0;
}

static void in_vdisk_pause(void *data, struct flb_config *config) {
  struct flb_in_vdisk_config *ctx = data;
  flb_input_collector_pause(ctx->coll_fd, ctx->i_ins);
}

static void in_vdisk_resume(void *data, struct flb_config *config) {
  struct flb_in_vdisk_config *ctx = data;
  flb_input_collector_resume(ctx->coll_fd, ctx->i_ins);
}

static int in_vdisk_exit(void *data, struct flb_config *config) {
  (void) *config;
  struct flb_in_vdisk_config *ctx = data;
  flb_free(ctx);

  return 0;
}

/* Plugin reference */
struct flb_input_plugin in_vdisk_plugin = {.name = "vdisk",
                                           .description =
                                               "Partition Disk Usage",
                                           .cb_init = in_vdisk_init,
                                           .cb_pre_run = NULL,
                                           .cb_collect = in_vdisk_collect,
                                           .cb_flush_buf = NULL,
                                           .cb_pause = in_vdisk_pause,
                                           .cb_resume = in_vdisk_resume,
                                           .cb_exit = in_vdisk_exit};
