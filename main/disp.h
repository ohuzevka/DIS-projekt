/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once
#include <stdbool.h>

/**
 * @brief Example: Create basic graphics widgets
 *
 */
void disp_init(void);

void disp_set_clock1(unsigned int max_time, unsigned int P1_sec);
void disp_set_clock2(unsigned int max_time, unsigned int P2_sec);

void disp_set_P1_cb(bool set);

void disp_set_P2_cb(bool set);
