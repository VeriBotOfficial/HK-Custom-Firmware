#include "rtconfig.h"
#include <stdio.h>
#include "lvsf.h"
#include <lv_ext_resource_manager.h>
#include <rtthread.h>
#include "gui_app_fwk.h"
#include "littlevgl2rtt.h"
#include "custom_trans_anim.h"

LV_IMG_DECLARE(img_stopwatch);

#define APP_ID "Stopwatch"

typedef enum {
    MODE_TIMER = 0,
    MODE_STOPWATCH
} app_mode_t;

static app_mode_t current_mode = MODE_TIMER;

/* Timer State */
static uint32_t timer_seconds_left = 60;
static uint32_t timer_initial_seconds = 60;
static bool timer_running = false;
static lv_obj_t *timer_arc = NULL;
static lv_obj_t *timer_label = NULL;
static lv_obj_t *timer_btn_play = NULL;
static lv_obj_t *timer_btn_play_lbl = NULL;
static lv_timer_t *timer_handle = NULL;

/* Stopwatch State */
static uint32_t sw_total_ms = 0;
static bool sw_running = false;
static lv_obj_t *sw_label = NULL;
static lv_obj_t *sw_btn_play = NULL;
static lv_obj_t *sw_btn_play_lbl = NULL;
static lv_timer_t *sw_handle = NULL;

static lv_obj_t *cont_timer = NULL;
static lv_obj_t *cont_sw = NULL;
static lv_obj_t *tab_btn_timer = NULL;
static lv_obj_t *tab_btn_sw = NULL;

/* -------------------------------------------------------------------
 * Timer Logic
 * ------------------------------------------------------------------- */

static void update_timer_display(void)
{
    if (timer_label)
    {
        uint32_t mins = timer_seconds_left / 60;
        uint32_t secs = timer_seconds_left % 60;
        char buf[16];
        snprintf(buf, sizeof(buf), "%02lu:%02lu", (unsigned long)mins, (unsigned long)secs);
        lv_label_set_text(timer_label, buf);
    }

    if (timer_arc)
    {
        int val = 0;
        if (timer_initial_seconds > 0)
        {
            val = (int)((timer_seconds_left * 100) / timer_initial_seconds);
        }
        lv_arc_set_value(timer_arc, val);
    }
}

static void timer_cb(lv_timer_t *t)
{
    if (timer_running)
    {
        if (timer_seconds_left > 0)
        {
            timer_seconds_left--;
            update_timer_display();
        }
        else
        {
            timer_running = false;
            if (timer_btn_play_lbl) lv_label_set_text(timer_btn_play_lbl, "Start");
            if (timer_btn_play) lv_obj_set_style_bg_color(timer_btn_play, lv_palette_main(LV_PALETTE_GREEN), 0);
        }
    }
}

static void timer_btn_play_cb(lv_event_t *e)
{
    if (timer_seconds_left == 0)
    {
        timer_seconds_left = timer_initial_seconds;
    }
    timer_running = !timer_running;
    if (timer_running)
    {
        if (timer_btn_play_lbl) lv_label_set_text(timer_btn_play_lbl, "Pause");
        if (timer_btn_play) lv_obj_set_style_bg_color(timer_btn_play, lv_palette_main(LV_PALETTE_ORANGE), 0);
    }
    else
    {
        if (timer_btn_play_lbl) lv_label_set_text(timer_btn_play_lbl, "Start");
        if (timer_btn_play) lv_obj_set_style_bg_color(timer_btn_play, lv_palette_main(LV_PALETTE_GREEN), 0);
    }
}

static void timer_btn_reset_cb(lv_event_t *e)
{
    timer_running = false;
    timer_seconds_left = timer_initial_seconds;
    update_timer_display();
    if (timer_btn_play_lbl) lv_label_set_text(timer_btn_play_lbl, "Start");
    if (timer_btn_play) lv_obj_set_style_bg_color(timer_btn_play, lv_palette_main(LV_PALETTE_GREEN), 0);
}

static void preset_btn_cb(lv_event_t *e)
{
    uint32_t mins = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    timer_running = false;
    timer_initial_seconds = mins * 60;
    timer_seconds_left = timer_initial_seconds;
    update_timer_display();
    if (timer_btn_play_lbl) lv_label_set_text(timer_btn_play_lbl, "Start");
    if (timer_btn_play) lv_obj_set_style_bg_color(timer_btn_play, lv_palette_main(LV_PALETTE_GREEN), 0);
}

/* -------------------------------------------------------------------
 * Stopwatch Logic
 * ------------------------------------------------------------------- */

static void sw_cb(lv_timer_t *t)
{
    if (sw_running)
    {
        sw_total_ms += 100;
        uint32_t seconds = (sw_total_ms / 1000) % 60;
        uint32_t minutes = (sw_total_ms / 60000) % 60;
        uint32_t tenths = (sw_total_ms % 1000) / 100;

        char buf[16];
        snprintf(buf, sizeof(buf), "%02lu:%02lu.%lu", (unsigned long)minutes, (unsigned long)seconds, (unsigned long)tenths);
        if (sw_label)
        {
            lv_label_set_text(sw_label, buf);
        }
    }
}

static void sw_btn_play_cb(lv_event_t *e)
{
    sw_running = !sw_running;
    if (sw_running)
    {
        if (sw_btn_play_lbl) lv_label_set_text(sw_btn_play_lbl, "Pause");
        if (sw_btn_play) lv_obj_set_style_bg_color(sw_btn_play, lv_palette_main(LV_PALETTE_ORANGE), 0);
    }
    else
    {
        if (sw_btn_play_lbl) lv_label_set_text(sw_btn_play_lbl, "Start");
        if (sw_btn_play) lv_obj_set_style_bg_color(sw_btn_play, lv_palette_main(LV_PALETTE_GREEN), 0);
    }
}

static void sw_btn_reset_cb(lv_event_t *e)
{
    sw_running = false;
    sw_total_ms = 0;
    if (sw_label) lv_label_set_text(sw_label, "00:00.0");
    if (sw_btn_play_lbl) lv_label_set_text(sw_btn_play_lbl, "Start");
    if (sw_btn_play) lv_obj_set_style_bg_color(sw_btn_play, lv_palette_main(LV_PALETTE_GREEN), 0);
}

/* -------------------------------------------------------------------
 * Navigation / Tabs
 * ------------------------------------------------------------------- */

static void switch_mode_cb(lv_event_t *e)
{
    app_mode_t mode = (app_mode_t)(uintptr_t)lv_event_get_user_data(e);
    current_mode = mode;

    if (current_mode == MODE_TIMER)
    {
        lv_obj_clear_flag(cont_timer, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(cont_sw, LV_OBJ_FLAG_HIDDEN);

        lv_obj_set_style_bg_color(tab_btn_timer, lv_palette_main(LV_PALETTE_ORANGE), 0);
        lv_obj_set_style_bg_color(tab_btn_sw, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    }
    else
    {
        lv_obj_add_flag(cont_timer, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(cont_sw, LV_OBJ_FLAG_HIDDEN);

        lv_obj_set_style_bg_color(tab_btn_timer, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
        lv_obj_set_style_bg_color(tab_btn_sw, lv_palette_main(LV_PALETTE_ORANGE), 0);
    }
}

/* -------------------------------------------------------------------
 * UI Initialization
 * ------------------------------------------------------------------- */

static void timer_ui_init(lv_obj_t *parent)
{
    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_set_size(root, LV_HOR_RES_MAX, LV_VER_RES_MAX);
    lv_obj_set_style_bg_color(root, lv_color_black(), 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_pad_all(root, 0, 0);

    /* Top Tab Switcher */
    tab_btn_timer = lv_btn_create(root);
    lv_obj_set_size(tab_btn_timer, 120, 38);
    lv_obj_align(tab_btn_timer, LV_ALIGN_TOP_LEFT, 60, 15);
    lv_obj_set_style_radius(tab_btn_timer, 19, 0);
    lv_obj_set_style_bg_color(tab_btn_timer, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_add_event_cb(tab_btn_timer, switch_mode_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)MODE_TIMER);

    lv_obj_t *t_lbl = lv_label_create(tab_btn_timer);
    lv_label_set_text(t_lbl, "Timer");
    lv_obj_center(t_lbl);

    tab_btn_sw = lv_btn_create(root);
    lv_obj_set_size(tab_btn_sw, 120, 38);
    lv_obj_align(tab_btn_sw, LV_ALIGN_TOP_RIGHT, -60, 15);
    lv_obj_set_style_radius(tab_btn_sw, 19, 0);
    lv_obj_set_style_bg_color(tab_btn_sw, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_add_event_cb(tab_btn_sw, switch_mode_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)MODE_STOPWATCH);

    lv_obj_t *s_lbl = lv_label_create(tab_btn_sw);
    lv_label_set_text(s_lbl, "Stopwatch");
    lv_obj_center(s_lbl);

    /* ---------------- TIMER VIEW ---------------- */
    cont_timer = lv_obj_create(root);
    lv_obj_set_size(cont_timer, LV_HOR_RES_MAX, LV_VER_RES_MAX - 60);
    lv_obj_align(cont_timer, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(cont_timer, lv_color_black(), 0);
    lv_obj_set_style_border_width(cont_timer, 0, 0);

    /* WatchOS Circular Progress Arc */
    timer_arc = lv_arc_create(cont_timer);
    lv_obj_set_size(timer_arc, 220, 220);
    lv_obj_align(timer_arc, LV_ALIGN_TOP_MID, 0, 10);
    lv_arc_set_rotation(timer_arc, 270);
    lv_arc_set_bg_angles(timer_arc, 0, 360);
    lv_arc_set_range(timer_arc, 0, 100);
    lv_arc_set_value(timer_arc, 100);
    lv_obj_remove_style(timer_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(timer_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(timer_arc, lv_palette_darken(LV_PALETTE_GREY, 3), LV_PART_MAIN);
    lv_obj_set_style_arc_color(timer_arc, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(timer_arc, 12, LV_PART_MAIN);
    lv_obj_set_style_arc_width(timer_arc, 12, LV_PART_INDICATOR);

    timer_label = lv_label_create(timer_arc);
    lv_label_set_text(timer_label, "01:00");
    lv_obj_set_style_text_color(timer_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(timer_label, &lv_font_montserrat_36, 0);
    lv_obj_center(timer_label);

    /* Preset Buttons (1m, 3m, 5m, 10m) */
    uint32_t presets[] = {1, 3, 5, 10};
    int p_w = 75, p_h = 36, p_gap = 10;
    int p_start_x = (LV_HOR_RES_MAX - (4 * p_w + 3 * p_gap)) / 2;

    for (int i = 0; i < 4; i++)
    {
        lv_obj_t *p_btn = lv_btn_create(cont_timer);
        lv_obj_set_size(p_btn, p_w, p_h);
        lv_obj_set_pos(p_btn, p_start_x + i * (p_w + p_gap), 245);
        lv_obj_set_style_radius(p_btn, 18, 0);
        lv_obj_set_style_bg_color(p_btn, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
        lv_obj_add_event_cb(p_btn, preset_btn_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)presets[i]);

        lv_obj_t *plbl = lv_label_create(p_btn);
        char pbuf[8];
        snprintf(pbuf, sizeof(pbuf), "%lum", (unsigned long)presets[i]);
        lv_label_set_text(plbl, pbuf);
        lv_obj_center(plbl);
    }

    /* Play / Reset Controls */
    timer_btn_play = lv_btn_create(cont_timer);
    lv_obj_set_size(timer_btn_play, 120, 50);
    lv_obj_align(timer_btn_play, LV_ALIGN_BOTTOM_LEFT, 30, -20);
    lv_obj_set_style_radius(timer_btn_play, 25, 0);
    lv_obj_set_style_bg_color(timer_btn_play, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_add_event_cb(timer_btn_play, timer_btn_play_cb, LV_EVENT_CLICKED, NULL);

    timer_btn_play_lbl = lv_label_create(timer_btn_play);
    lv_label_set_text(timer_btn_play_lbl, "Start");
    lv_obj_set_style_text_font(timer_btn_play_lbl, &lv_font_montserrat_16, 0);
    lv_obj_center(timer_btn_play_lbl);

    lv_obj_t *timer_btn_reset = lv_btn_create(cont_timer);
    lv_obj_set_size(timer_btn_reset, 120, 50);
    lv_obj_align(timer_btn_reset, LV_ALIGN_BOTTOM_RIGHT, -30, -20);
    lv_obj_set_style_radius(timer_btn_reset, 25, 0);
    lv_obj_set_style_bg_color(timer_btn_reset, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_add_event_cb(timer_btn_reset, timer_btn_reset_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *timer_btn_reset_lbl = lv_label_create(timer_btn_reset);
    lv_label_set_text(timer_btn_reset_lbl, "Reset");
    lv_obj_set_style_text_font(timer_btn_reset_lbl, &lv_font_montserrat_16, 0);
    lv_obj_center(timer_btn_reset_lbl);

    /* ---------------- STOPWATCH VIEW ---------------- */
    cont_sw = lv_obj_create(root);
    lv_obj_set_size(cont_sw, LV_HOR_RES_MAX, LV_VER_RES_MAX - 60);
    lv_obj_align(cont_sw, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(cont_sw, lv_color_black(), 0);
    lv_obj_set_style_border_width(cont_sw, 0, 0);
    lv_obj_add_flag(cont_sw, LV_OBJ_FLAG_HIDDEN);

    sw_label = lv_label_create(cont_sw);
    lv_label_set_text(sw_label, "00:00.0");
    lv_obj_set_style_text_color(sw_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(sw_label, &lv_font_montserrat_48, 0);
    lv_obj_align(sw_label, LV_ALIGN_CENTER, 0, -40);

    sw_btn_play = lv_btn_create(cont_sw);
    lv_obj_set_size(sw_btn_play, 120, 50);
    lv_obj_align(sw_btn_play, LV_ALIGN_BOTTOM_LEFT, 30, -20);
    lv_obj_set_style_radius(sw_btn_play, 25, 0);
    lv_obj_set_style_bg_color(sw_btn_play, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_add_event_cb(sw_btn_play, sw_btn_play_cb, LV_EVENT_CLICKED, NULL);

    sw_btn_play_lbl = lv_label_create(sw_btn_play);
    lv_label_set_text(sw_btn_play_lbl, "Start");
    lv_obj_set_style_text_font(sw_btn_play_lbl, &lv_font_montserrat_16, 0);
    lv_obj_center(sw_btn_play_lbl);

    lv_obj_t *sw_btn_reset = lv_btn_create(cont_sw);
    lv_obj_set_size(sw_btn_reset, 120, 50);
    lv_obj_align(sw_btn_reset, LV_ALIGN_BOTTOM_RIGHT, -30, -20);
    lv_obj_set_style_radius(sw_btn_reset, 25, 0);
    lv_obj_set_style_bg_color(sw_btn_reset, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(sw_btn_reset, sw_btn_reset_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *sw_btn_reset_lbl = lv_label_create(sw_btn_reset);
    lv_label_set_text(sw_btn_reset_lbl, "Reset");
    lv_obj_set_style_text_font(sw_btn_reset_lbl, &lv_font_montserrat_16, 0);
    lv_obj_center(sw_btn_reset_lbl);

    /* Timers */
    if (!timer_handle)
    {
        timer_handle = lv_timer_create(timer_cb, 1000, NULL);
    }
    if (!sw_handle)
    {
        sw_handle = lv_timer_create(sw_cb, 100, NULL);
    }
}

static void on_start(void)
{
    cust_trans_anim_config(CUST_ANIM_TYPE_3, NULL);
    timer_seconds_left = 60;
    timer_initial_seconds = 60;
    timer_running = false;
    sw_total_ms = 0;
    sw_running = false;

    timer_ui_init(lv_scr_act());
}

static void on_stop(void)
{
    if (timer_handle)
    {
        lv_timer_del(timer_handle);
        timer_handle = NULL;
    }
    if (sw_handle)
    {
        lv_timer_del(sw_handle);
        sw_handle = NULL;
    }
}

static void msg_handler(gui_app_msg_type_t msg, void *param)
{
    switch (msg)
    {
    case GUI_APP_MSG_ONSTART:
        on_start();
        break;
    case GUI_APP_MSG_ONSTOP:
        on_stop();
        break;
    default:
        break;
    }
}

static int app_main(intent_t i)
{
    gui_app_regist_msg_handler(APP_ID, msg_handler);
    return 0;
}

BUILTIN_APP_EXPORT(LV_EXT_STR_ID(Stopwatch),
                   LV_EXT_IMG_GET(img_stopwatch), APP_ID, app_main);
