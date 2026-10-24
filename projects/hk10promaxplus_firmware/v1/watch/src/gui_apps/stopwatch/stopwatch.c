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

#define MAX_LAPS 5

static lv_obj_t *time_label;
static lv_obj_t *btn_start_pause;
static lv_obj_t *btn_start_pause_label;
static lv_obj_t *lap_list;
static lv_timer_t *timer = NULL;

static uint32_t total_ms = 0;
static bool is_running = false;
static uint32_t laps[MAX_LAPS];
static uint8_t lap_count = 0;

static void update_time_display(void)
{
    uint32_t hundredths = (total_ms % 1000) / 10;
    uint32_t seconds = (total_ms / 1000) % 60;
    uint32_t minutes = (total_ms / 60000) % 60;

    char buf[16];
    snprintf(buf, sizeof(buf), "%02lu:%02lu.%02lu", (unsigned long)minutes, (unsigned long)seconds, (unsigned long)hundredths);
    if (time_label)
    {
        lv_label_set_text(time_label, buf);
    }
}

static void timer_cb(lv_timer_t *t)
{
    if (is_running)
    {
        total_ms += 50;
        update_time_display();
    }
}

static void update_lap_list_ui(void)
{
    if (!lap_list) return;

    lv_obj_clean(lap_list);
    for (int i = lap_count - 1; i >= 0; i--)
    {
        uint32_t ms = laps[i];
        uint32_t hundredths = (ms % 1000) / 10;
        uint32_t seconds = (ms / 1000) % 60;
        uint32_t minutes = (ms / 60000) % 60;

        char buf[32];
        snprintf(buf, sizeof(buf), "Lap %d    %02lu:%02lu.%02lu", i + 1, (unsigned long)minutes, (unsigned long)seconds, (unsigned long)hundredths);

        lv_obj_t *item = lv_list_add_btn(lap_list, NULL, buf);
        lv_obj_set_style_bg_color(item, lv_palette_darken(LV_PALETTE_GREY, 4), 0);
        lv_obj_set_style_text_color(item, lv_color_white(), 0);
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

static void btn_lap_reset_cb(lv_event_t *e)
{
    if (is_running)
    {
        /* Record Lap */
        if (lap_count < MAX_LAPS)
        {
            laps[lap_count++] = total_ms;
            update_lap_list_ui();
        }
    }
    else
    {
        /* Reset Timer & Laps */
        total_ms = 0;
        lap_count = 0;
        update_time_display();
        if (lap_list) lv_obj_clean(lap_list);
        if (btn_start_pause_label)
        {
            lv_label_set_text(btn_start_pause_label, "Start");
            lv_obj_set_style_bg_color(btn_start_pause, lv_palette_main(LV_PALETTE_GREEN), 0);
        }
    }
}

static void stopwatch_ui_init(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_HOR_RES_MAX, LV_VER_RES_MAX);
    lv_obj_set_style_bg_color(cont, lv_color_black(), 0);
    lv_obj_set_style_border_width(cont, 0, 0);

    /* Title */
    lv_obj_t *title = lv_label_create(cont);
    lv_label_set_text(title, "Stopwatch");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 25);

    /* Main Time Display */
    time_label = lv_label_create(cont);
    lv_obj_align(time_label, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_set_style_text_color(time_label, lv_palette_main(LV_PALETTE_LIGHT_BLUE), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_36, 0);
    lv_label_set_text(time_label, "00:00.00");

    /* Lap Records List */
    lap_list = lv_list_create(cont);
    lv_obj_set_size(lap_list, LV_HOR_RES_MAX - 40, 160);
    lv_obj_align(lap_list, LV_ALIGN_TOP_MID, 0, 130);
    lv_obj_set_style_bg_color(lap_list, lv_color_black(), 0);
    lv_obj_set_style_border_width(lap_list, 0, 0);

    /* Circular WatchOS Action Buttons */
    btn_start_pause = lv_btn_create(cont);
    lv_obj_set_size(btn_start_pause, 100, 50);
    lv_obj_align(btn_start_pause, LV_ALIGN_BOTTOM_RIGHT, -30, -25);
    lv_obj_set_style_bg_color(btn_start_pause, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_radius(btn_start_pause, 25, 0);
    lv_obj_add_event_cb(btn_start_pause, btn_start_pause_cb, LV_EVENT_CLICKED, NULL);

    btn_start_pause_label = lv_label_create(btn_start_pause);
    lv_label_set_text(btn_start_pause_label, "Start");
    lv_obj_center(btn_start_pause_label);

    lv_obj_t *btn_lap_reset = lv_btn_create(cont);
    lv_obj_set_size(btn_lap_reset, 100, 50);
    lv_obj_align(btn_lap_reset, LV_ALIGN_BOTTOM_LEFT, 30, -25);
    lv_obj_set_style_bg_color(btn_lap_reset, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_set_style_radius(btn_lap_reset, 25, 0);
    lv_obj_add_event_cb(btn_lap_reset, btn_lap_reset_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_reset_label = lv_label_create(btn_lap_reset);
    lv_label_set_text(btn_reset_label, "Lap / Reset");
    lv_obj_set_style_text_font(btn_reset_label, &lv_font_montserrat_12, 0);
    lv_obj_center(btn_reset_label);

    if (!timer)
    {
        timer = lv_timer_create(timer_cb, 50, NULL);
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
