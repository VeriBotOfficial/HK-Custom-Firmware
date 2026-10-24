/*********************
 *      INCLUDES
 *********************/
#include <rtthread.h>
#include <rtdevice.h>
#include "littlevgl2rtt.h"
#include "lvgl.h"
#include "gui_app_fwk.h"
#include "custom_trans_anim.h"

#ifdef APP_TRANS_ANIMATION_SCALE

static void enter_or_exit_anim_watchos(gui_anim_obj_t img, void *user_data, uint32_t flag, gui_anim_value_t process)
{
#if LV_USE_IMG
    if (!lv_obj_check_type(img, &lv_img_class)) return;
#endif
    lv_point_t *pivot = (lv_point_t *)user_data;
    uint16_t zoom;
    uint16_t zoom_start, zoom_end;
    lv_opa_t opa_v;

    /*
     * Apple WatchOS app transition logic:
     * Open (Entering):
     *   Target app screen starts small at clicked icon pivot, zooms up smoothly from LV_IMG_ZOOM_NONE/8 to LV_IMG_ZOOM_NONE, and fades in (0 to 255 opa).
     * Close (Exiting / Going back to app list):
     *   Current app screen shrinks smoothly down from LV_IMG_ZOOM_NONE to LV_IMG_ZOOM_NONE/8 back towards the icon pivot, and fades out (255 to 0 opa).
     */

    if (flag & FLAG_TRANS_ANIM_REVERSE) /* Exiting / Closing app */
    {
        if (flag & FLAG_TRANS_ANIM_FG)
        {
            /* Current app window shrinking & fading out */
            zoom_start = LV_IMG_ZOOM_NONE;
            zoom_end = LV_IMG_ZOOM_NONE / 8;
            opa_v = 255 - (255 * process / MANUAL_TRAN_ANIM_MAX_PROCESS);
        }
        else
        {
            /* Background app list fading in */
            zoom_start = LV_IMG_ZOOM_NONE;
            zoom_end = LV_IMG_ZOOM_NONE;
            opa_v = 255 * process / MANUAL_TRAN_ANIM_MAX_PROCESS;
        }
    }
    else /* Entering / Opening app */
    {
        if (flag & FLAG_TRANS_ANIM_FG)
        {
            /* Target app window expanding from icon pivot & fading in */
            zoom_start = LV_IMG_ZOOM_NONE / 8;
            zoom_end = LV_IMG_ZOOM_NONE;
            opa_v = 255 * process / MANUAL_TRAN_ANIM_MAX_PROCESS;
        }
        else
        {
            /* Main menu background zooming out slightly and fading */
            zoom_start = LV_IMG_ZOOM_NONE;
            zoom_end = LV_IMG_ZOOM_NONE * 2;
            opa_v = 255 - (255 * process / MANUAL_TRAN_ANIM_MAX_PROCESS);
        }
    }

    zoom = zoom_start + (int32_t)(zoom_end - zoom_start) * process / MANUAL_TRAN_ANIM_MAX_PROCESS;

    lv_img_set_zoom(img, zoom);

    if (pivot && (pivot->x != 0 || pivot->y != 0))
    {
        lv_img_set_pivot(img, pivot->x, pivot->y);
    }
    else
    {
        lv_img_set_pivot(img, lv_obj_get_width(img) >> 1, lv_obj_get_height(img) >> 1);
    }

    lv_obj_set_style_img_opa(img, opa_v, LV_PART_MAIN);
}

void cust_trans_anim_config(CUST_ANIM_TYPE_E type, lv_point_t *pivot)
{
    gui_app_trans_anim_t enter_anim_cfg, exit_anim_cfg;

    gui_app_trans_anim_init_cfg(&enter_anim_cfg, GUI_APP_TRANS_ANIM_CUSTOM);
    gui_app_trans_anim_init_cfg(&exit_anim_cfg, GUI_APP_TRANS_ANIM_CUSTOM);

    enter_anim_cfg.cfg.cust.exe_cb = enter_or_exit_anim_watchos;
    enter_anim_cfg.cfg.cust.user_data = pivot;

    exit_anim_cfg.cfg.cust.exe_cb = enter_or_exit_anim_watchos;
    exit_anim_cfg.cfg.cust.user_data = pivot;

    gui_app_set_enter_trans_anim(&enter_anim_cfg);
    gui_app_set_exit_trans_anim(&exit_anim_cfg);
}

#else
void cust_trans_anim_config(CUST_ANIM_TYPE_E type, lv_point_t *pivot)
{
}
#endif /* APP_TRANS_ANIMATION_SCALE */
