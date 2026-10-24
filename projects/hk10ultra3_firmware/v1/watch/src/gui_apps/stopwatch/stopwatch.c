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

static lv_obj_t *time_label;
static lv_obj_t *btn_start_pause;
static lv_obj_t *btn_start_pause_label;
static lv_timer_t *timer = NULL;

static uint32_t total_ms = 0;
static bool is_running = false;

static void timer_cb(lv_timer_t *t)
{
    if (is_running)
    {
        total_ms += 100;
        uint32_t seconds = (total_ms / 1000) % 60;
        uint32_t minutes = (total_ms / 60000) % 60;
        uint32_t tenths = (total_ms % 1000) / 100;

        char buf[16];
        snprintf(buf, sizeof(buf), "%02lu:%02lu.%lu", (unsigned long)minutes, (unsigned long)seconds, (unsigned long)tenths);
        if (time_label)
        {
            lv_label_set_text(time_label, buf);
        }
    }
}

static void btn_start_pause_cb(lv_event_t *e)
{
    is_running = !is_running;
    if (is_running)
    {
        lv_label_set_text(btn_start_pause_label, "Pause");
        lv_obj_set_style_bg_color(btn_start_pause, lv_palette_main(LV_PALETTE_ORANGE), 0);
    }
    else
    {
        lv_label_set_text(btn_start_pause_label, "Start");
        lv_obj_set_style_bg_color(btn_start_pause, lv_palette_main(LV_PALETTE_GREEN), 0);
    }
}

static void btn_reset_cb(lv_event_t *e)
{
    is_running = false;
    total_ms = 0;
    if (time_label)
    {
        lv_label_set_text(time_label, "00:00.0");
    }
    if (btn_start_pause_label)
    {
        lv_label_set_text(btn_start_pause_label, "Start");
        lv_obj_set_style_bg_color(btn_start_pause, lv_palette_main(LV_PALETTE_GREEN), 0);
    }
}

static void stopwatch_ui_init(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_HOR_RES_MAX, LV_VER_RES_MAX);
    lv_obj_set_style_bg_color(cont, lv_color_black(), 0);
    lv_obj_set_style_border_width(cont, 0, 0);

    time_label = lv_label_create(cont);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_label_set_text(time_label, "00:00.0");

    // Start / Pause button
    btn_start_pause = lv_btn_create(cont);
    lv_obj_set_size(btn_start_pause, 100, 50);
    lv_obj_align(btn_start_pause, LV_ALIGN_CENTER, -60, 40);
    lv_obj_set_style_bg_color(btn_start_pause, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_add_event_cb(btn_start_pause, btn_start_pause_cb, LV_EVENT_CLICKED, NULL);

    btn_start_pause_label = lv_label_create(btn_start_pause);
    lv_label_set_text(btn_start_pause_label, "Start");
    lv_obj_center(btn_start_pause_label);

    // Reset button
    lv_obj_t *btn_reset = lv_btn_create(cont);
    lv_obj_set_size(btn_reset, 100, 50);
    lv_obj_align(btn_reset, LV_ALIGN_CENTER, 60, 40);
    lv_obj_set_style_bg_color(btn_reset, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(btn_reset, btn_reset_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_reset_label = lv_label_create(btn_reset);
    lv_label_set_text(btn_reset_label, "Reset");
    lv_obj_center(btn_reset_label);

    if (!timer)
    {
        timer = lv_timer_create(timer_cb, 100, NULL);
    }
}

static void on_start(void)
{
    cust_trans_anim_config(CUST_ANIM_TYPE_3, NULL);
    total_ms = 0;
    is_running = false;
    stopwatch_ui_init(lv_scr_act());
}

static void on_stop(void)
{
    if (timer)
    {
        lv_timer_del(timer);
        timer = NULL;
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
