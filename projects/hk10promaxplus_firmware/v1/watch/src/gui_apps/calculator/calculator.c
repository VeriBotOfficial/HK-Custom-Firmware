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
    else if (strcmp(txt, "+/-") == 0)
    {
        if (expr_buf[0] != '0')
        {
            if (expr_buf[0] == '-')
            {
                memmove(expr_buf, expr_buf + 1, strlen(expr_buf));
            }
            else if (strlen(expr_buf) < 15)
            {
                memmove(expr_buf + 1, expr_buf, strlen(expr_buf) + 1);
                expr_buf[0] = '-';
            }
        }
    }
    else if (strcmp(txt, "%") == 0)
    {
        double val = atof(expr_buf) / 100.0;
        snprintf(expr_buf, sizeof(expr_buf), "%.4g", val);
        new_num = true;
    }
    else if ((txt[0] >= '0' && txt[0] <= '9') || txt[0] == '.')
    {
        if (txt[0] == '.' && strchr(expr_buf, '.')) return;

        if (new_num || strcmp(expr_buf, "0") == 0)
        {
            if (txt[0] == '.')
            {
                strcpy(expr_buf, "0.");
            }
            else
            {
                expr_buf[0] = txt[0];
                expr_buf[1] = '\0';
            }
            new_num = false;
        }
        else if (strlen(expr_buf) < 12)
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

            if (res == (long long)res)
                snprintf(expr_buf, sizeof(expr_buf), "%lld", (long long)res);
            else
                snprintf(expr_buf, sizeof(expr_buf), "%.6g", res);

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
    lv_obj_set_style_pad_all(cont, 12, 0);

    /* WatchOS Large LCD Screen Display */
    disp_label = lv_label_create(cont);
    lv_obj_set_size(disp_label, LV_HOR_RES_MAX - 24, 60);
    lv_obj_align(disp_label, LV_ALIGN_TOP_MID, 0, 25);
    lv_obj_set_style_text_color(disp_label, lv_color_white(), 0);
    lv_obj_set_style_text_align(disp_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(disp_label, &lv_font_montserrat_36, 0);
    lv_label_set_text(disp_label, "0");

    /* WatchOS Circular Keypad Matrix */
    const char *btn_map[] = {
        "C", "+/-", "%", "/",
        "7", "8", "9", "*",
        "4", "5", "6", "-",
        "1", "2", "3", "+",
        "0", ".", "="
    };

    int start_y = 95;
    int btn_w = 80;
    int btn_h = 60;
    int gap_x = 12;
    int gap_y = 10;
    int start_x = (LV_HOR_RES_MAX - (4 * btn_w + 3 * gap_x)) / 2;

    int idx = 0;
    for (int r = 0; r < 4; r++)
    {
        for (int c = 0; c < 4; c++)
        {
            lv_obj_t *btn = lv_btn_create(cont);
            lv_obj_set_size(btn, btn_w, btn_h);
            lv_obj_set_pos(btn, start_x + c * (btn_w + gap_x), start_y + r * (btn_h + gap_y));
            lv_obj_set_style_radius(btn, 30, 0); /* Round WatchOS pill buttons */

            if (c == 3) /* Rightmost operation column */
            {
                lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_ORANGE), 0);
            }
            else if (r == 0) /* Top control row */
            {
                lv_obj_set_style_bg_color(btn, lv_palette_darken(LV_PALETTE_GREY, 1), 0);
            }
            else /* Number pad */
            {
                lv_obj_set_style_bg_color(btn, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
            }

            lv_obj_add_event_cb(btn, btn_click_cb, LV_EVENT_CLICKED, NULL);

            lv_obj_t *lbl = lv_label_create(btn);
            lv_label_set_text(lbl, btn_map[idx++]);
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
            lv_obj_center(lbl);
        }
    }

    /* Bottom Row: "0" (Wide pill), ".", "=" */
    /* Button "0" */
    lv_obj_t *btn_0 = lv_btn_create(cont);
    lv_obj_set_size(btn_0, btn_w * 2 + gap_x, btn_h);
    lv_obj_set_pos(btn_0, start_x, start_y + 4 * (btn_h + gap_y));
    lv_obj_set_style_radius(btn_0, 30, 0);
    lv_obj_set_style_bg_color(btn_0, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_add_event_cb(btn_0, btn_click_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_0 = lv_label_create(btn_0);
    lv_label_set_text(lbl_0, "0");
    lv_obj_set_style_text_font(lbl_0, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl_0, LV_ALIGN_LEFT_MID, 28, 0);

    /* Button "." */
    lv_obj_t *btn_dot = lv_btn_create(cont);
    lv_obj_set_size(btn_dot, btn_w, btn_h);
    lv_obj_set_pos(btn_dot, start_x + 2 * (btn_w + gap_x), start_y + 4 * (btn_h + gap_y));
    lv_obj_set_style_radius(btn_dot, 30, 0);
    lv_obj_set_style_bg_color(btn_dot, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_add_event_cb(btn_dot, btn_click_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_dot = lv_label_create(btn_dot);
    lv_label_set_text(lbl_dot, ".");
    lv_obj_set_style_text_font(lbl_dot, &lv_font_montserrat_20, 0);
    lv_obj_center(lbl_dot);

    /* Button "=" */
    lv_obj_t *btn_eq = lv_btn_create(cont);
    lv_obj_set_size(btn_eq, btn_w, btn_h);
    lv_obj_set_pos(btn_eq, start_x + 3 * (btn_w + gap_x), start_y + 4 * (btn_h + gap_y));
    lv_obj_set_style_radius(btn_eq, 30, 0);
    lv_obj_set_style_bg_color(btn_eq, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_add_event_cb(btn_eq, btn_click_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_eq = lv_label_create(btn_eq);
    lv_label_set_text(lbl_eq, "=");
    lv_obj_set_style_text_font(lbl_eq, &lv_font_montserrat_20, 0);
    lv_obj_center(lbl_eq);
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
