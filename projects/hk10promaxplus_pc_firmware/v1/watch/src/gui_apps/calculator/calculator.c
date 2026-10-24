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

    if (strcmp(txt, "AC") == 0)
    {
        expr_buf[0] = '0';
        expr_buf[1] = '\0';
        num1 = 0;
        num2 = 0;
        op = 0;
        new_num = true;
    }
    else if (strcmp(txt, "DEL") == 0)
    {
        size_t len = strlen(expr_buf);
        if (len > 1)
        {
            expr_buf[len - 1] = '\0';
        }
        else
        {
            expr_buf[0] = '0';
            expr_buf[1] = '\0';
            new_num = true;
        }
    }
    else if (strcmp(txt, ".") == 0)
    {
        if (new_num)
        {
            snprintf(expr_buf, sizeof(expr_buf), "0.");
            new_num = false;
        }
        else if (strchr(expr_buf, '.') == NULL && strlen(expr_buf) < 10)
        {
            strcat(expr_buf, ".");
        }
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
            bool err = false;

            if (op == '+') res = num1 + num2;
            else if (op == '-') res = num1 - num2;
            else if (op == '*') res = num1 * num2;
            else if (op == '/')
            {
                if (num2 != 0) res = num1 / num2;
                else err = true;
            }

            if (err)
            {
                snprintf(expr_buf, sizeof(expr_buf), "Error");
            }
            else if (res == (long)res)
            {
                snprintf(expr_buf, sizeof(expr_buf), "%ld", (long)res);
            }
            else
            {
                snprintf(expr_buf, sizeof(expr_buf), "%.4g", res);
            }

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

    /* WatchOS Style Calculator Display */
    disp_label = lv_label_create(cont);
    lv_obj_set_size(disp_label, LV_HOR_RES_MAX - 40, 50);
    lv_obj_align(disp_label, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_style_text_color(disp_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(disp_label, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_align(disp_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(disp_label, "0");

    /* Circular WatchOS Keypad Matrix */
    const char *btn_map[16] = {
        "AC", "DEL", "/", "*",
        "7",  "8",   "9", "-",
        "4",  "5",   "6", "+",
        "1",  "2",   "3", "="
    };

    int start_y = 100;
    int btn_size = 52;
    int gap = 12;
    int start_x = (LV_HOR_RES_MAX - (4 * btn_size + 3 * gap)) / 2;

    for (int i = 0; i < 16; i++)
    {
        int row = i / 4;
        int col = i % 4;

        lv_obj_t *btn = lv_btn_create(cont);
        lv_obj_set_size(btn, btn_size, btn_size);
        lv_obj_set_pos(btn, start_x + col * (btn_size + gap), start_y + row * (btn_size + gap));
        lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);

        if (strcmp(btn_map[i], "=") == 0 || strcmp(btn_map[i], "/") == 0 ||
            strcmp(btn_map[i], "*") == 0 || strcmp(btn_map[i], "-") == 0 ||
            strcmp(btn_map[i], "+") == 0)
        {
            lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_ORANGE), 0);
        }
        else if (strcmp(btn_map[i], "AC") == 0 || strcmp(btn_map[i], "DEL") == 0)
        {
            lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_GREY), 0);
            lv_obj_set_style_text_color(btn, lv_color_black(), 0);
        }
        else
        {
            lv_obj_set_style_bg_color(btn, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
        }

        lv_obj_add_event_cb(btn, btn_click_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btn_map[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
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
