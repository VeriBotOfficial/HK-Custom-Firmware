#include "rtconfig.h"
#include <stdio.h>
#include "lvsf.h"
#include <lv_ext_resource_manager.h>
#include <rtthread.h>
#include "gui_app_fwk.h"
#include "littlevgl2rtt.h"
#include "custom_trans_anim.h"

LV_IMG_DECLARE(img_settings);

#define APP_ID "Settings"

static void switch_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    bool state = lv_obj_has_state(sw, LV_STATE_CHECKED);
    (void)state;
}

static lv_obj_t *create_card_row(lv_obj_t *parent, const char *title_txt, const char *desc_txt, bool has_switch)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_HOR_RES_MAX - 30, 60);
    lv_obj_set_style_bg_color(card, lv_palette_darken(LV_PALETTE_GREY, 4), 0);
    lv_obj_set_style_border_color(card, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 16, 0);

    lv_obj_t *lbl_title = lv_label_create(card);
    lv_label_set_text(lbl_title, title_txt);
    lv_obj_set_style_text_color(lbl_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 10, 6);

    if (desc_txt)
    {
        lv_obj_t *lbl_desc = lv_label_create(card);
        lv_label_set_text(lbl_desc, desc_txt);
        lv_obj_set_style_text_color(lbl_desc, lv_palette_main(LV_PALETTE_GREY), 0);
        lv_obj_set_style_text_font(lbl_desc, &lv_font_montserrat_12, 0);
        lv_obj_align(lbl_desc, LV_ALIGN_BOTTOM_LEFT, 10, -6);
    }

    if (has_switch)
    {
        lv_obj_t *sw = lv_switch_create(card);
        lv_obj_set_size(sw, 45, 24);
        lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -10, 0);
        lv_obj_add_state(sw, LV_STATE_CHECKED);
        lv_obj_add_event_cb(sw, switch_cb, LV_EVENT_VALUE_CHANGED, NULL);
    }

    return card;
}

static void settings_ui_init(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_HOR_RES_MAX, LV_VER_RES_MAX);
    lv_obj_set_style_bg_color(cont, lv_color_black(), 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(cont, 15, 0);
    lv_obj_set_style_pad_row(cont, 12, 0);

    /* Title */
    lv_obj_t *title = lv_label_create(cont);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);

    /* Section 1: Display & Brightness */
    create_card_row(cont, "Always-On Display", "Keep watch screen active", true);
    create_card_row(cont, "Raise to Wake", "Turn on display on tilt", true);

    /* Section 2: Sound & Haptics */
    create_card_row(cont, "Haptic Feedback", "Vibrate on touch & alerts", true);
    create_card_row(cont, "Silent Mode", "Mute all notifications", false);

    /* Section 3: System Info Card */
    char buf_ram[64];
    rt_uint32_t total = 0, used = 0, max_used = 0;

#ifndef _WIN32
    rt_memory_info(&total, &used, &max_used);
    snprintf(buf_ram, sizeof(buf_ram), "RAM Used: %lu / %lu KB", (unsigned long)(used / 1024), (unsigned long)(total / 1024));
#else
    snprintf(buf_ram, sizeof(buf_ram), "PC Simulator Mode (Windows)");
#endif

    create_card_row(cont, "Model", "HK10 Pro Max+", false);
    create_card_row(cont, "Chipset", "SF32LB525 Dual-Core 240MHz", false);
    create_card_row(cont, "Display", "2.02\" 410x494 AMOLED", false);
    create_card_row(cont, "Memory Info", buf_ram, false);
    create_card_row(cont, "Firmware", "v1.0.0 WatchOS Edition", false);
}

static void on_start(void)
{
    cust_trans_anim_config(CUST_ANIM_TYPE_3, NULL);
    settings_ui_init(lv_scr_act());
}

static void msg_handler(gui_app_msg_type_t msg, void *param)
{
    switch (msg)
    {
    case GUI_APP_MSG_ONSTART:
        on_start();
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

BUILTIN_APP_EXPORT(LV_EXT_STR_ID(Settings),
                   LV_EXT_IMG_GET(img_settings), APP_ID, app_main);
