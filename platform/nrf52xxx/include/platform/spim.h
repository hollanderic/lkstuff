/*
 * Copyright (c) 2022 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <nrfx.h>
#include <nrfx_spim.h>


uint32_t spim_init(uint32_t bus, nrfx_spim_config_t config);
