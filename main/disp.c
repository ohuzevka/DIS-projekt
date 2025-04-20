/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "disp.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"

static lv_obj_t *P1_checkbox = NULL;
static lv_obj_t *P2_checkbox = NULL;
// static lv_obj_t *volume_arc = NULL;
static lv_obj_t *clk1_bar = NULL;
static lv_obj_t *clk2_bar = NULL;
// static lv_obj_t *clk1_label = NULL;
// static lv_obj_t *clk2_label = NULL;

static lv_obj_t *clk1_time_spangroup = NULL;
static lv_obj_t *clk2_time_spangroup = NULL;
static lv_span_t *clk1_time_span = NULL;
static lv_span_t *clk2_time_span = NULL;


void disp_init(void)
{
    bsp_display_lock(0);

    // /*Create a window*/
    // lv_obj_t *win = lv_win_create(lv_scr_act(), 30);
    // assert(win);
    // lv_win_add_title(win, "Chess clock");

    // Chess clock sign
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_border_width(&style, 1);
    lv_style_set_border_color(&style, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_pad_all(&style, 2);

    lv_obj_t * spans = lv_spangroup_create(lv_scr_act());
    lv_obj_set_size(spans, 200, 30);
    lv_obj_align(spans, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_add_style(spans, &style, 0);

    lv_spangroup_set_align(spans, LV_TEXT_ALIGN_CENTER);
    lv_spangroup_set_overflow(spans, LV_SPAN_OVERFLOW_CLIP);
    lv_spangroup_set_mode(spans, LV_SPAN_MODE_FIXED);

    lv_span_t * span = lv_spangroup_new_span(spans);

    span = lv_spangroup_new_span(spans);
    lv_span_set_text_static(span, "Chess clock");
    lv_style_set_text_font(&span->style,  &lv_font_montserrat_24);

    lv_spangroup_refr_mode(spans);

    // Clock 1 Time
    clk1_time_spangroup = lv_spangroup_create(lv_scr_act());
    lv_obj_set_size(clk1_time_spangroup, 100, 30);
    lv_obj_align(clk1_time_spangroup, LV_ALIGN_BOTTOM_LEFT, 10, -30);
    lv_obj_add_style(clk1_time_spangroup, &style, 0);

    lv_spangroup_set_align(clk1_time_spangroup, LV_TEXT_ALIGN_CENTER);
    lv_spangroup_set_overflow(clk1_time_spangroup, LV_SPAN_OVERFLOW_CLIP);
    lv_spangroup_set_mode(clk1_time_spangroup, LV_SPAN_MODE_FIXED);

    clk1_time_span = lv_spangroup_new_span(clk1_time_spangroup);
    lv_span_set_text(clk1_time_span, "01 : 00");
    lv_style_set_text_font(&clk1_time_span->style,  &lv_font_montserrat_24);
    lv_spangroup_refr_mode(clk1_time_spangroup);

    // Clock 2 Time
    clk2_time_spangroup = lv_spangroup_create(lv_scr_act());
    lv_obj_set_size(clk2_time_spangroup, 100, 30);
    lv_obj_align(clk2_time_spangroup, LV_ALIGN_BOTTOM_RIGHT, -10, -30);
    lv_obj_add_style(clk2_time_spangroup, &style, 0);

    lv_spangroup_set_align(clk2_time_spangroup, LV_TEXT_ALIGN_CENTER);
    lv_spangroup_set_overflow(clk2_time_spangroup, LV_SPAN_OVERFLOW_CLIP);
    lv_spangroup_set_mode(clk2_time_spangroup, LV_SPAN_MODE_FIXED);

    clk2_time_span = lv_spangroup_new_span(clk2_time_spangroup);
    lv_span_set_text(clk2_time_span, "01 : 00");
    lv_style_set_text_font(&clk2_time_span->style,  &lv_font_montserrat_24);
    lv_spangroup_refr_mode(clk2_time_spangroup);


    // Player 1 clock
    clk1_bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(clk1_bar, 50, 170);
    lv_obj_align(clk1_bar, LV_ALIGN_CENTER, -60, -30);
    lv_bar_set_range(clk1_bar, 0, 60);
    lv_bar_set_value(clk1_bar, 60, LV_ANIM_OFF);

    // clk1_label = lv_label_create(lv_scr_act());
    // lv_label_set_text(clk1_label, "01 : 00");
    // lv_obj_align_to(clk1_label, clk1_bar, LV_ALIGN_OUT_TOP_MID, 0, -5);
    

    // Player 2 clock
    clk2_bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(clk2_bar, 50, 170);
    lv_obj_align(clk2_bar, LV_ALIGN_CENTER, 60, -30);
    lv_bar_set_range(clk2_bar, 0, 60);
    lv_bar_set_value(clk2_bar, 60, LV_ANIM_OFF);

    // clk2_label = lv_label_create(lv_scr_act());
    // lv_label_set_text(clk2_label, "01 : 00");
    // lv_obj_align_to(clk2_label, clk2_bar, LV_ALIGN_OUT_TOP_MID, 0, -5);

    // buttons label
    lv_obj_t * buttons_label = lv_label_create(lv_scr_act());
    lv_label_set_text(buttons_label, "< P1 |  -  | Reset | Pause |  +  | P2 >");
    lv_obj_align(buttons_label, LV_ALIGN_BOTTOM_MID, 0, -5);


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

    /* Checkboxes */
    P1_checkbox = lv_checkbox_create(lv_scr_act());
    lv_checkbox_set_text_static(P1_checkbox, "Player 1");
    lv_obj_align_to(P1_checkbox, clk1_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    P2_checkbox = lv_checkbox_create(lv_scr_act());
    assert(P2_checkbox);
    lv_checkbox_set_text_static(P2_checkbox, "Player 2");
    lv_obj_align_to(P2_checkbox, clk2_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

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
    // lv_label_set_text(clk1_label, str);
    lv_span_set_text(clk1_time_span, str);
    lv_spangroup_refr_mode(clk1_time_spangroup);

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
    // lv_label_set_text(clk2_label, str);
    lv_span_set_text(clk2_time_span, str);
    lv_spangroup_refr_mode(clk2_time_spangroup);

    bsp_display_unlock();
}

// void disp_set_volume(int volume)
// {
//     assert(volume_arc);
//     bsp_display_lock(0);
//     lv_arc_set_value(volume_arc, volume);
//     bsp_display_unlock();
// }

void disp_set_P1_cb(bool set)
{
    bsp_display_lock(0);
    if (set) {
        lv_obj_add_state(P1_checkbox, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(P1_checkbox, LV_STATE_CHECKED);
    }
    bsp_display_unlock();
}

void disp_set_P2_cb(bool set)
{
    bsp_display_lock(0);
    if (set) {
        lv_obj_add_state(P2_checkbox, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(P2_checkbox, LV_STATE_CHECKED);
    }
    bsp_display_unlock();
}
