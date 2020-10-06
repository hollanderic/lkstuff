/*
 * Copyright (c) 2020 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

// Must be called before other functions can be used.
status_t nrf52_clock_init(void);

status_t nrf52_clock_hf_use_xtal_source(void);
void nrf52_clock_hf_use_internal_source(void);