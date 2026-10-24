#include "rtconfig.h"
#include <stdio.h>
#include <time.h>
#include "lvsf.h"
#include <lv_ext_resource_manager.h>
#include <rtthread.h>
#include "gui_app_fwk.h"
#include "littlevgl2rtt.h"
#include "custom_trans_anim.h"

LV_IMG_DECLARE(img_clock);

#define APP_ID "Watchface"

static lv_obj_t *time_label = NULL;
static lv_obj_t *sec_label = NULL;
static lv_obj_t *date_label = NULL;
static lv_obj_t *bat_label = NULL;
static lv_obj_t *hr_label = NULL;
static lv_obj_t *step_label = NULL;
static lv_timer_t *wf_timer = NULL;

static void update_watchface_data(void)
{
    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    if (timeinfo == NULL)
    {
        /* Fallback if time not set */
        static int fake_sec = 0;
        fake_sec = (fake_sec + 1) % 60;

        if (time_label) lv_label_set_text(time_label, "10:09");
        if (sec_label)
        {
            char sbuf[8];
            snprintf(sbuf, sizeof(sbuf), "%02d", fake_sec);
            lv_label_set_text(sec_label, sbuf);
        }
        if (date_label) lv_label_set_text(date_label, "MON, OCT 24");
    }
    else
    {
        char tbuf[16];
        snprintf(tbuf, sizeof(tbuf), "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
        if (time_label) lv_label_set_text(time_label, tbuf);

        char sbuf[8];
        snprintf(sbuf, sizeof(sbuf), "%02d", timeinfo->tm_sec);
        if (sec_label) lv_label_set_text(sec_label, sbuf);

        const char *days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
        const char *months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

        char dbuf[32];
        snprintf(dbuf, sizeof(dbuf), "%s, %s %d", days[timeinfo->tm_wday % 7], months[timeinfo->tm_mon % 12], timeinfo->tm_mday);
        if (date_label) lv_label_set_text(date_label, dbuf);
    }

    if (bat_label) lv_label_set_text(bat_label, "85%");
    if (hr_label) lv_label_set_text(hr_label, "72 BPM");
    if (step_label) lv_label_set_text(step_label, "8,420");
}

static void wf_timer_cb(lv_timer_t *t)
{
    update_watchface_data();
}

static void watchface_ui_init(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_HOR_RES_MAX, LV_VER_RES_MAX);
    lv_obj_set_style_bg_color(cont, lv_color_black(), 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);

    /* Apple Watch Ultra Style Layout */

    /* Top Date Complication */
    date_label = lv_label_create(cont);
    lv_label_set_text(date_label, "MON, OCT 24");
    lv_obj_set_style_text_color(date_label, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_20, 0);
    lv_obj_align(date_label, LV_ALIGN_TOP_MID, 0, 65);

    /* Center Main Time (HH:MM) */
    time_label = lv_label_create(cont);
    lv_label_set_text(time_label, "10:09");
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, -25, -20);

    /* Seconds Counter Complication */
    sec_label = lv_label_create(cont);
    lv_label_set_text(sec_label, "32");
    lv_obj_set_style_text_color(sec_label, lv_palette_main(LV_PALETTE_LIGHT_BLUE), 0);
    lv_obj_set_style_text_font(sec_label, &lv_font_montserrat_24, 0);
    lv_obj_align(sec_label, LV_ALIGN_CENTER, 85, -12);

    /* Bottom Complication Card Grid */
    int card_w = 110;
    int card_h = 75;

    /* 1. Battery Card */
    lv_obj_t *c_bat = lv_obj_create(cont);
    lv_obj_set_size(c_bat, card_w, card_h);
    lv_obj_align(c_bat, LV_ALIGN_BOTTOM_LEFT, 25, -40);
    lv_obj_set_style_bg_color(c_bat, lv_palette_darken(LV_PALETTE_GREY, 4), 0);
    lv_obj_set_style_border_width(c_bat, 0, 0);
    lv_obj_set_style_radius(c_bat, 16, 0);

    lv_obj_t *bat_lbl_hdr = lv_label_create(c_bat);
    lv_label_set_text(bat_lbl_hdr, "BATTERY");
    lv_obj_set_style_text_color(bat_lbl_hdr, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_text_font(bat_lbl_hdr, &lv_font_montserrat_12, 0);
    lv_obj_align(bat_lbl_hdr, LV_ALIGN_TOP_LEFT, 0, 0);

    bat_label = lv_label_create(c_bat);
    lv_label_set_text(bat_label, "85%");
    lv_obj_set_style_text_color(bat_label, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_text_font(bat_label, &lv_font_montserrat_20, 0);
    lv_obj_align(bat_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    /* 2. Heart Rate Card */
    lv_obj_t *c_hr = lv_obj_create(cont);
    lv_obj_set_size(c_hr, card_w, card_h);
    lv_obj_align(c_hr, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_bg_color(c_hr, lv_palette_darken(LV_PALETTE_GREY, 4), 0);
    lv_obj_set_style_border_width(c_hr, 0, 0);
    lv_obj_set_style_radius(c_hr, 16, 0);

    lv_obj_t *hr_lbl_hdr = lv_label_create(c_hr);
    lv_label_set_text(hr_lbl_hdr, "HEART");
    lv_obj_set_style_text_color(hr_lbl_hdr, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_text_font(hr_lbl_hdr, &lv_font_montserrat_12, 0);
    lv_obj_align(hr_lbl_hdr, LV_ALIGN_TOP_LEFT, 0, 0);

    hr_label = lv_label_create(c_hr);
    lv_label_set_text(hr_label, "72 BPM");
    lv_obj_set_style_text_color(hr_label, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_text_font(hr_label, &lv_font_montserrat_16, 0);
    lv_obj_align(hr_label, LV_ALIGN_BOTTOM_LEFT, 0, -2);

    /* 3. Activity / Steps Card */
    lv_obj_t *c_step = lv_obj_create(cont);
    lv_obj_set_size(c_step, card_w, card_h);
    lv_obj_align(c_step, LV_ALIGN_BOTTOM_RIGHT, -25, -40);
    lv_obj_set_style_bg_color(c_step, lv_palette_darken(LV_PALETTE_GREY, 4), 0);
    lv_obj_set_style_border_width(c_step, 0, 0);
    lv_obj_set_style_radius(c_step, 16, 0);

    lv_obj_t *step_lbl_hdr = lv_label_create(c_step);
    lv_label_set_text(step_lbl_hdr, "STEPS");
    lv_obj_set_style_text_color(step_lbl_hdr, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_text_font(step_lbl_hdr, &lv_font_montserrat_12, 0);
    lv_obj_align(step_lbl_hdr, LV_ALIGN_TOP_LEFT, 0, 0);

    step_label = lv_label_create(c_step);
    lv_label_set_text(step_label, "8,420");
    lv_obj_set_style_text_color(step_label, lv_palette_main(LV_PALETTE_CYAN), 0);
    lv_obj_set_style_text_font(step_label, &lv_font_montserrat_16, 0);
    lv_obj_align(step_label, LV_ALIGN_BOTTOM_LEFT, 0, -2);

    if (!wf_timer)
    {
        wf_timer = lv_timer_create(wf_timer_cb, 1000, NULL);
    }
    update_watchface_data();
}

static void on_start(void)
{
    cust_trans_anim_config(CUST_ANIM_TYPE_3, NULL);
    watchface_ui_init(lv_scr_act());
}

static void on_stop(void)
{
    if (wf_timer)
    {
        lv_timer_del(wf_timer);
        wf_timer = NULL;
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
