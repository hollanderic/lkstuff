/*
 * Copyright (c) 2022 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <nrfx.h>
/*
  To use the spi master driver, NRFX_SPIM_ENABLED must be set to 1 in the GLOBAL_DEFINES
  additionally, NRFX_SPIM0_ENABLED and/or NRFX_SPIM1_ENABLED must also be 1 to specify
  which modules are active.  These would ideally be set in either a project rule.mk
  or in a targets rules.mk.

  The pins to be used for each of the active SPIM modules should be defined as:
  TWIM0_SCL_PIN, TWIM0_SDA_PIN  (if twim0 is used)
  TWIM1_SCL_PIN, TWIM1_SDA_PIN  (if twim1 is used)
  and ideally be defined in the targets include/gpioconfig.h since it is included here.
*/