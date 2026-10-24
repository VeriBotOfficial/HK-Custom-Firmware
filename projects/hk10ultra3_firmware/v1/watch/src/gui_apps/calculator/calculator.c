#include "rtconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvsf.h"
#include <lv_ext_resource_manager.h>
#include <rtthread.h>
#include "gui_app_fwk.h"
#include "littlevgl2rtt.h"
#include "custom_trans_anim.h"

LV_IMG_DECLARE(img_passbook);

#define APP_ID "Calculator"

static lv_obj_t *disp_label;
static char expr_buf[32] = "0";
static double num1 = 0;
static double num2 = 0;
static char op = 0;
static bool new_num = true;

static void update_display(const char *txt)
{
    if (disp_label)
    {
        lv_label_set_text(disp_label, txt);
    }
}

static void btn_click_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    if (!label) return;
    const char *txt = lv_label_get_text(label);

    if (strcmp(txt, "C") == 0)
    {
        expr_buf[0] = '0';
        expr_buf[1] = '\0';
        num1 = 0;
        num2 = 0;
        op = 0;
        new_num = true;
    }
    else if (txt[0] >= '0' && txt[0] <= '9')
    {
        if (new_num || strcmp(expr_buf, "0") == 0)
        {
            expr_buf[0] = txt[0];
            expr_buf[1] = '\0';
            new_num = false;
        }
        else if (strlen(expr_buf) < 10)
        {
            strcat(expr_buf, txt);
        }
    }
    else if (strcmp(txt, "+") == 0 || strcmp(txt, "-") == 0 ||
             strcmp(txt, "*") == 0 || strcmp(txt, "/") == 0)
    {
        num1 = atof(expr_buf);
        op = txt[0];
        new_num = true;
    }
    else if (strcmp(txt, "=") == 0)
    {
        if (op != 0)
        {
            num2 = atof(expr_buf);
            double res = 0;
            if (op == '+') res = num1 + num2;
            else if (op == '-') res = num1 - num2;
            else if (op == '*') res = num1 * num2;
            else if (op == '/') res = (num2 != 0) ? (num1 / num2) : 0;

            if (res == (long)res)
                snprintf(expr_buf, sizeof(expr_buf), "%ld", (long)res);
            else
                snprintf(expr_buf, sizeof(expr_buf), "%.2f", res);

            op = 0;
            new_num = true;
        }
    }
    update_display(expr_buf);
}

static void calc_ui_init(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_HOR_RES_MAX, LV_VER_RES_MAX);
    lv_obj_set_style_bg_color(cont, lv_color_black(), 0);
    lv_obj_set_style_border_width(cont, 0, 0);

    // Display Screen
    disp_label = lv_label_create(cont);
    lv_obj_set_size(disp_label, LV_HOR_RES_MAX - 40, 40);
    lv_obj_align(disp_label, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_text_color(disp_label, lv_color_white(), 0);
    lv_obj_set_style_text_align(disp_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(disp_label, "0");

    // Keypad Matrix
    const char *btn_map[] = {
        "C", "/", "*", "-",
        "7", "8", "9", "+",
        "4", "5", "6", "=",
        "1", "2", "3", "0"
    };

    int start_y = 80;
    int btn_w = 55;
    int btn_h = 45;
    int gap = 8;
    int start_x = (LV_HOR_RES_MAX - (4 * btn_w + 3 * gap)) / 2;

    for (int i = 0; i < 16; i++)
    {
        int row = i / 4;
        int col = i % 4;

        lv_obj_t *btn = lv_btn_create(cont);
        lv_obj_set_size(btn, btn_w, btn_h);
        lv_obj_set_pos(btn, start_x + col * (btn_w + gap), start_y + row * (btn_h + gap));

        if (strcmp(btn_map[i], "=") == 0 || strcmp(btn_map[i], "C") == 0)
            lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_ORANGE), 0);
        else if (btn_map[i][0] >= '0' && btn_map[i][0] <= '9')
            lv_obj_set_style_bg_color(btn, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
        else
            lv_obj_set_style_bg_color(btn, lv_palette_darken(LV_PALETTE_BLUE_GREY, 2), 0);

        lv_obj_add_event_cb(btn, btn_click_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btn_map[i]);
        lv_obj_center(lbl);
    }
}

static void on_start(void)
{
    cust_trans_anim_config(CUST_ANIM_TYPE_3, NULL);
    expr_buf[0] = '0';
    expr_buf[1] = '\0';
    num1 = 0;
    num2 = 0;
    op = 0;
    new_num = true;

    calc_ui_init(lv_scr_act());
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

BUILTIN_APP_EXPORT(LV_EXT_STR_ID(Calculator),
                   LV_EXT_IMG_GET(img_passbook), APP_ID, app_main);
