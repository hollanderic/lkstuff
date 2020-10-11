/*
 * Copyright (c) 2020 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <nrfx.h>

#include <sys/types.h>

typedef enum {
  RTC0 = 0,
  RTC1 = 1,
  RTC2 = 2
} lk_rtc_instances_t;

typedef struct {
  nrfx_rtc_t *dev;
  void *cbs[4];
  void *cb_args[4];
} lk_rtc_dev_t;


status_t nrf52_rtc_init(lk_rtc_instances_t instance);


