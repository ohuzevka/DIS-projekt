/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
#define LV_USE_PERF_MONITOR 0

#include "disp_example.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"

static lv_obj_t *recording_checkbox = NULL;
static lv_obj_t *playing_checkbox = NULL;
static lv_obj_t *volume_arc = NULL;
static lv_obj_t *clk1_bar = NULL;
static lv_obj_t *clk2_bar = NULL;
static lv_obj_t *clk1_label = NULL;
static lv_obj_t *clk2_label = NULL;


void disp_init(void)
{
    bsp_display_lock(0);

    /*Create a window*/
    lv_obj_t *win = lv_win_create(lv_scr_act(), 30);
    assert(win);
    lv_win_add_title(win, "Chess clock");

    static lv_obj_t *clk1_name = NULL;
    static lv_obj_t *clk2_name = NULL;


    // Player 1 clock
    clk1_bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(clk1_bar, 200, 20);
    lv_obj_align(clk1_bar, LV_ALIGN_CENTER, 0, -50);
    lv_bar_set_range(clk1_bar, 0, 60);
    lv_bar_set_value(clk1_bar, 60, LV_ANIM_OFF);

    clk1_label = lv_label_create(lv_scr_act());
    lv_label_set_text(clk1_label, "01 : 00");
    lv_obj_align_to(clk1_label, clk1_bar, LV_ALIGN_OUT_TOP_MID, 0, -5);
    
    clk1_name = lv_label_create(lv_scr_act());
    lv_label_set_text(clk1_name, "Player 1");
    lv_obj_align_to(clk1_name, clk1_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    // Player 2 clock
    clk2_bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(clk2_bar, 200, 20);
    lv_obj_align(clk2_bar, LV_ALIGN_CENTER, 0, 50);
    lv_bar_set_range(clk2_bar, 0, 60);
    lv_bar_set_value(clk2_bar, 60, LV_ANIM_OFF);

    clk2_label = lv_label_create(lv_scr_act());
    lv_label_set_text(clk2_label, "01 : 00");
    lv_obj_align_to(clk2_label, clk2_bar, LV_ALIGN_OUT_TOP_MID, 0, -5);

    clk2_name = lv_label_create(lv_scr_act());
    lv_label_set_text(clk2_name, "Player 2");
    lv_obj_align_to(clk2_name, clk2_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    // /* Volume arc */
    // volume_arc = lv_arc_create(lv_scr_act());
    // assert(volume_arc);
    // lv_arc_set_end_angle(volume_arc, 200);
    // lv_obj_set_size(volume_arc, 130, 130);
    // lv_obj_center(volume_arc);
    // lv_obj_t *volume_label = lv_label_create(volume_arc);
    // assert(volume_label);
    // lv_label_set_text_static(volume_label, "Volume");
    // lv_obj_set_align(volume_label, LV_ALIGN_BOTTOM_MID);

    // /* Checkboxes */
    // recording_checkbox = lv_checkbox_create(win);
    // assert(recording_checkbox);
    // lv_checkbox_set_text_static(recording_checkbox, "Recording");
    // playing_checkbox = lv_checkbox_create(win);
    // assert(playing_checkbox);
    // lv_checkbox_set_text_static(playing_checkbox, "Playing");

    bsp_display_unlock();
}

void disp_set_clock1(unsigned int max_time, unsigned int P1_sec)
{
    assert(clk1_bar);
    
    char str[20];
    sprintf(str, "%02d : %02d", P1_sec/60, P1_sec%60);
    
    bsp_display_lock(0);

    lv_bar_set_range(clk1_bar, 0, max_time);
    lv_bar_set_value(clk1_bar, P1_sec, LV_ANIM_OFF);
    lv_label_set_text(clk1_label, str);

    bsp_display_unlock();
}

void disp_set_clock2(unsigned int max_time, unsigned int P2_sec)
{
    assert(clk2_bar);
    
    char str[20];
    sprintf(str, "%02d : %02d", P2_sec/60, P2_sec%60);
    
    bsp_display_lock(0);

    lv_bar_set_range(clk2_bar, 0, max_time);
    lv_bar_set_value(clk2_bar, P2_sec, LV_ANIM_OFF);
    lv_label_set_text(clk2_label, str);

    bsp_display_unlock();
}

void disp_set_volume(int volume)
{
    assert(volume_arc);
    bsp_display_lock(0);
    lv_arc_set_value(volume_arc, volume);
    bsp_display_unlock();
}

void disp_set_playing(bool set)
{
    assert(playing_checkbox);
    bsp_display_lock(0);
    if (set) {
        lv_obj_add_state(playing_checkbox, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(playing_checkbox, LV_STATE_CHECKED);
    }
    bsp_display_unlock();
}

void disp_set_recording(bool set)
{
    assert(recording_checkbox);
    bsp_display_lock(0);
    if (set) {
        lv_obj_add_state(recording_checkbox, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(recording_checkbox, LV_STATE_CHECKED);
    }
    bsp_display_unlock();
}
