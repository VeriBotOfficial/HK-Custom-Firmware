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

static bool toggle_bt = true;
static bool toggle_dnd = false;
static bool toggle_silent = false;
static bool toggle_aod = true;

static void switch_event_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    bool *state = (bool *)lv_event_get_user_data(e);
    if (state)
    {
        *state = lv_obj_has_state(sw, LV_STATE_CHECKED);
    }
}

static lv_obj_t *create_setting_card(lv_obj_t *parent, const char *title, const char *subtitle)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_width(card, LV_HOR_RES_MAX - 30);
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, lv_palette_darken(LV_PALETTE_GREY, 4), 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *t_label = lv_label_create(card);
    lv_label_set_text(t_label, title);
    lv_obj_set_style_text_color(t_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(t_label, &lv_font_montserrat_16, 0);

    if (subtitle)
    {
        lv_obj_t *sub_label = lv_label_create(card);
        lv_label_set_text(sub_label, subtitle);
        lv_obj_set_style_text_color(sub_label, lv_palette_main(LV_PALETTE_GREY), 0);
        lv_obj_set_style_text_font(sub_label, &lv_font_montserrat_12, 0);
        lv_obj_set_style_margin_top(sub_label, 2, 0);
    }

    return card;
}

static lv_obj_t *create_setting_toggle(lv_obj_t *parent, const char *title, bool *value_ptr)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_width(card, LV_HOR_RES_MAX - 30);
    lv_obj_set_height(card, 56);
    lv_obj_set_style_bg_color(card, lv_palette_darken(LV_PALETTE_GREY, 4), 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_pad_side(card, 14, 0);
    lv_obj_set_style_pad_top(card, 10, 0);
    lv_obj_set_style_pad_bottom(card, 10, 0);

    lv_obj_t *t_label = lv_label_create(card);
    lv_label_set_text(t_label, title);
    lv_obj_set_style_text_color(t_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(t_label, &lv_font_montserrat_16, 0);
    lv_obj_align(t_label, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *sw = lv_switch_create(card);
    lv_obj_align(sw, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(sw, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR | LV_STATE_CHECKED);
    if (value_ptr && *value_ptr)
    {
        lv_obj_add_state(sw, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(sw, switch_event_cb, LV_EVENT_VALUE_CHANGED, value_ptr);

    return card;
}

static void settings_ui_init(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_HOR_RES_MAX, LV_VER_RES_MAX);
    lv_obj_set_style_bg_color(cont, lv_color_black(), 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);

    /* WatchOS Style Header */
    lv_obj_t *hdr = lv_obj_create(cont);
    lv_obj_set_size(hdr, LV_HOR_RES_MAX, 50);
    lv_obj_set_style_bg_color(hdr, lv_color_black(), 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_pad_left(hdr, 20, 0);
    lv_obj_set_style_pad_top(hdr, 12, 0);

    lv_obj_t *hdr_title = lv_label_create(hdr);
    lv_label_set_text(hdr_title, "Settings");
    lv_obj_set_style_text_color(hdr_title, lv_color_white(), 0);
    lv_obj_set_style_text_font(hdr_title, &lv_font_montserrat_20, 0);

    /* Scrollable Container */
    lv_obj_t *scroll_cont = lv_obj_create(cont);
    lv_obj_set_size(scroll_cont, LV_HOR_RES_MAX, LV_VER_RES_MAX - 50);
    lv_obj_align(scroll_cont, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(scroll_cont, lv_color_black(), 0);
    lv_obj_set_style_border_width(scroll_cont, 0, 0);
    lv_obj_set_style_pad_all(scroll_cont, 10, 0);
    lv_obj_set_style_pad_gap(scroll_cont, 10, 0);
    lv_obj_set_flex_flow(scroll_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(scroll_cont, LV_SCROLLBAR_MODE_OFF);

    /* Toggles */
    create_setting_toggle(scroll_cont, "Bluetooth", &toggle_bt);
    create_setting_toggle(scroll_cont, "Do Not Disturb", &toggle_dnd);
    create_setting_toggle(scroll_cont, "Silent Mode", &toggle_silent);
    create_setting_toggle(scroll_cont, "Always On Display", &toggle_aod);

    /* System Info Cards */
    create_setting_card(scroll_cont, "HK10 Pro Max Plus", "Device Name");
    create_setting_card(scroll_cont, "SF32LB525 @ 240MHz", "Processor");
    create_setting_card(scroll_cont, "410 x 494 AMOLED", "Display Resolution");

    rt_uint32_t total, used, max_used;
    rt_memory_info(&total, &used, &max_used);
    char ram_buf[64];
    snprintf(ram_buf, sizeof(ram_buf), "%lu KB / %lu KB Used", (unsigned long)(used / 1024), (unsigned long)(total / 1024));
    create_setting_card(scroll_cont, ram_buf, "RAM Usage");

    create_setting_card(scroll_cont, "WatchOS v10.4 Custom", "Software Version");
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
