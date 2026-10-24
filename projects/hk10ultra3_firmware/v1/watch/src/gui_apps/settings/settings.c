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

static void settings_ui_init(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_HOR_RES_MAX, LV_VER_RES_MAX);
    lv_obj_set_style_bg_color(cont, lv_color_black(), 0);
    lv_obj_set_style_border_width(cont, 0, 0);

    lv_obj_t *list = lv_list_create(cont);
    lv_obj_set_size(list, LV_HOR_RES_MAX - 20, LV_VER_RES_MAX - 40);
    lv_obj_center(list);
    lv_obj_set_style_bg_color(list, lv_color_black(), 0);
    lv_obj_set_style_border_width(list, 0, 0);

    lv_list_add_text(list, "System Info");

    char buf[64];
    snprintf(buf, sizeof(buf), "Model: HK Watch Custom");
    lv_obj_t *item1 = lv_list_add_btn(list, NULL, buf);
    lv_obj_set_style_text_color(item1, lv_color_white(), 0);

    snprintf(buf, sizeof(buf), "Chip: SF32LB525 (240MHz)");
    lv_obj_t *item2 = lv_list_add_btn(list, NULL, buf);
    lv_obj_set_style_text_color(item2, lv_color_white(), 0);

    snprintf(buf, sizeof(buf), "Display: 410x494 AMOLED");
    lv_obj_t *item3 = lv_list_add_btn(list, NULL, buf);
    lv_obj_set_style_text_color(item3, lv_color_white(), 0);

    rt_uint32_t total, used, max_used;
    rt_memory_info(&total, &used, &max_used);
    snprintf(buf, sizeof(buf), "RAM: %lu / %lu KB", (unsigned long)(used / 1024), (unsigned long)(total / 1024));
    lv_obj_t *item4 = lv_list_add_btn(list, NULL, buf);
    lv_obj_set_style_text_color(item4, lv_color_white(), 0);

    snprintf(buf, sizeof(buf), "Firmware: v1.0.0-custom");
    lv_obj_t *item5 = lv_list_add_btn(list, NULL, buf);
    lv_obj_set_style_text_color(item5, lv_color_white(), 0);
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
