#include <stdio.h>
#include <time.h>
#include "rtconfig.h"
#include "lvsf.h"
#include <lv_ext_resource_manager.h>
#include <rtthread.h>
#include "gui_app_fwk.h"
#include "littlevgl2rtt.h"
#include "custom_trans_anim.h"

LV_IMG_DECLARE(img_clock);

#define APP_ID "Watchface"

static lv_obj_t *time_label = NULL;
static lv_obj_t *date_label = NULL;
static lv_obj_t *sec_label = NULL;
static lv_obj_t *steps_bar = NULL;
static lv_obj_t *steps_val_label = NULL;
static lv_obj_t *battery_val_label = NULL;
static lv_timer_t *clock_timer = NULL;

static void update_time_cb(lv_timer_t *timer)
{
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);

    if (tm_now)
    {
        if (time_label)
        {
            char time_str[16];
            snprintf(time_str, sizeof(time_str), "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
            lv_label_set_text(time_label, time_str);
        }

        if (sec_label)
        {
            char sec_str[8];
            snprintf(sec_str, sizeof(sec_str), "%02d", tm_now->tm_sec);
            lv_label_set_text(sec_label, sec_str);
        }

        if (date_label)
        {
            char date_str[32];
            static const char *days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
            static const char *months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
            snprintf(date_str, sizeof(date_str), "%s, %s %d",
                     days[tm_now->tm_wday % 7],
                     months[tm_now->tm_mon % 12],
                     tm_now->tm_mday);
            lv_label_set_text(date_label, date_str);
        }
    }
}

static void watchface_click_cb(lv_event_t *e)
{
    /* Open Main Menu on screen tap with smooth WatchOS transition */
    lv_point_t center_pivot = {LV_HOR_RES_MAX / 2, LV_VER_RES_MAX / 2};
    cust_trans_anim_config(CUST_ANIM_TYPE_3, &center_pivot);
    gui_app_run("MainMenu");
}

static void watchface_ui_init(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_HOR_RES_MAX, LV_VER_RES_MAX);
    lv_obj_set_style_bg_color(cont, lv_color_black(), 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(cont, watchface_click_cb, LV_EVENT_CLICKED, NULL);

    /* Watch Brand Header */
    lv_obj_t *title = lv_label_create(cont);
    lv_label_set_text(title, "HK10 PRO MAX+");
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 35);

    /* Date Complication */
    date_label = lv_label_create(cont);
    lv_label_set_text(date_label, "MON, JAN 1");
    lv_obj_set_style_text_color(date_label, lv_palette_main(LV_PALETTE_LIGHT_BLUE), 0);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_20, 0);
    lv_obj_align(date_label, LV_ALIGN_TOP_MID, 0, 75);

    /* Main Digital Clock */
    time_label = lv_label_create(cont);
    lv_label_set_text(time_label, "10:08");
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, -25, -20);

    /* Seconds Counter */
    sec_label = lv_label_create(cont);
    lv_label_set_text(sec_label, "42");
    lv_obj_set_style_text_color(sec_label, lv_palette_main(LV_PALETTE_AMBER), 0);
    lv_obj_set_style_text_font(sec_label, &lv_font_montserrat_24, 0);
    lv_obj_align(sec_label, LV_ALIGN_CENTER, 85, -12);

    /* Activity & Health Cards Container */
    lv_obj_t *card = lv_obj_create(cont);
    lv_obj_set_size(card, LV_HOR_RES_MAX - 50, 140);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 80);
    lv_obj_set_style_bg_color(card, lv_palette_darken(LV_PALETTE_GREY, 4), 0);
    lv_obj_set_style_border_color(card, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 20, 0);

    /* Steps Progress */
    lv_obj_t *steps_title = lv_label_create(card);
    lv_label_set_text(steps_title, "Daily Steps");
    lv_obj_set_style_text_color(steps_title, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_align(steps_title, LV_ALIGN_TOP_LEFT, 10, 8);

    steps_val_label = lv_label_create(card);
    lv_label_set_text(steps_val_label, "8,420 / 10,000");
    lv_obj_set_style_text_color(steps_val_label, lv_color_white(), 0);
    lv_obj_align(steps_val_label, LV_ALIGN_TOP_RIGHT, -10, 8);

    steps_bar = lv_bar_create(card);
    lv_obj_set_size(steps_bar, LV_HOR_RES_MAX - 90, 10);
    lv_obj_align(steps_bar, LV_ALIGN_TOP_MID, 0, 38);
    lv_bar_set_range(steps_bar, 0, 10000);
    lv_bar_set_value(steps_bar, 8420, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(steps_bar, lv_palette_darken(LV_PALETTE_GREY, 2), LV_PART_MAIN);
    lv_obj_set_style_bg_color(steps_bar, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);

    /* Battery & Status Row */
    lv_obj_t *bat_icon = lv_label_create(card);
    lv_label_set_text(bat_icon, "BATTERY");
    lv_obj_set_style_text_color(bat_icon, lv_palette_main(LV_PALETTE_CYAN), 0);
    lv_obj_align(bat_icon, LV_ALIGN_BOTTOM_LEFT, 10, -12);

    battery_val_label = lv_label_create(card);
    lv_label_set_text(battery_val_label, "85%");
    lv_obj_set_style_text_color(battery_val_label, lv_color_white(), 0);
    lv_obj_align(battery_val_label, LV_ALIGN_BOTTOM_RIGHT, -10, -12);

    /* Tap Hint Button */
    lv_obj_t *btn_menu = lv_btn_create(cont);
    lv_obj_set_size(btn_menu, 220, 48);
    lv_obj_align(btn_menu, LV_ALIGN_BOTTOM_MID, 0, -35);
    lv_obj_set_style_bg_color(btn_menu, lv_palette_darken(LV_PALETTE_BLUE, 3), 0);
    lv_obj_set_style_radius(btn_menu, 24, 0);
    lv_obj_add_event_cb(btn_menu, watchface_click_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_lbl = lv_label_create(btn_menu);
    lv_label_set_text(btn_lbl, "Tap to Open Apps");
    lv_obj_set_style_text_color(btn_lbl, lv_palette_main(LV_PALETTE_LIGHT_BLUE), 0);
    lv_obj_center(btn_lbl);

    update_time_cb(NULL);
    if (!clock_timer)
    {
        clock_timer = lv_timer_create(update_time_cb, 1000, NULL);
    }
}

static void on_start(void)
{
    cust_trans_anim_config(CUST_ANIM_TYPE_3, NULL);
    watchface_ui_init(lv_scr_act());
}

static void on_stop(void)
{
    if (clock_timer)
    {
        lv_timer_del(clock_timer);
        clock_timer = NULL;
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

BUILTIN_APP_EXPORT(LV_EXT_STR_ID(WatchfaceString), LV_EXT_IMG_GET(img_clock), APP_ID, app_main);