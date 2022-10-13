/*
 * Copyright (c) 2022 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <app.h>
#include <assert.h>
#include <lk/err.h>
#include <kernel/thread.h>
#include <lk/console_cmd.h>
#include <app/trinamic.h>

extern tmc_config_t tmc_pin_configs;

void tmc_status(void);

STATIC_COMMAND_START
STATIC_COMMAND("tmcstatus", "tmc2130 status", (console_cmd_func)&tmc_status)
STATIC_COMMAND_END(tmc2130);

static void tmc2130_init(const struct app_descriptor *app) {
    tmc_pin_configs.CSnPIN = 0;
}
static void tmc2130_entry(const struct app_descriptor *app, void *args) {}


void tmc_status(void) {}


APP_START(tmc2130)
.init = tmc2130_init,
.entry = tmc2130_entry,
APP_END
