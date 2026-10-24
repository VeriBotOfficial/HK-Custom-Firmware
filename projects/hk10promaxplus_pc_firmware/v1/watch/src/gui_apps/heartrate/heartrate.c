#include "rtconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "lvsf.h"
#include <lv_ext_resource_manager.h>
#include <rtthread.h>
#include <rtdevice.h>

#ifndef _WIN32
#include <sensor.h>
#endif

#include "gui_app_fwk.h"
#include "littlevgl2rtt.h"
#include "custom_trans_anim.h"

LV_IMG_DECLARE(img_red_heart);

#define APP_ID "HeartRate"

/* PPG DSP & Algorithm Constants */
#define PPG_SAMPLE_RATE_HZ     25     /* 25 Hz sampling frequency (40 ms per sample) */
#define PPG_WINDOW_SIZE        75     /* 3-second rolling window for dynamic threshold */
#define RR_BUFFER_SIZE         16     /* Buffer size for HRV (SDNN) computation */
#define PPG_CHART_POINT_COUNT  40     /* Points in live waveform visualizer */

/* State Structure for Heart Rate Algorithm */
typedef struct
{
    /* Raw & Filtered Signal State */
    float hp_y;
    float hp_x_prev;
    float lp_y;
    float ppg_window[PPG_WINDOW_SIZE];
    uint16_t window_idx;
    uint16_t window_count;

    /* Peak Detection State */
    uint32_t sample_counter;
    uint32_t last_peak_sample;
    float last_peak_val;

    /* Output Measurements */
    float current_bpm;
    float filtered_bpm;
    float min_bpm;
    float max_bpm;
    float avg_bpm;
    uint32_t bpm_sample_count;
    float bpm_sum;

    /* HRV (SDNN) State */
    float rr_intervals_ms[RR_BUFFER_SIZE];
    uint16_t rr_idx;
    uint16_t rr_count;
    float hrv_sdnn_ms;

    /* Status flags */
    bool is_measuring;
    bool is_touching;
    bool new_beat_detected;
    bool hw_sensor_present;
    rt_device_t hr_device;
} hr_algo_t;

static hr_algo_t g_hr_algo;

/* UI Widgets */
static lv_obj_t *main_cont = NULL;
static lv_obj_t *heart_img = NULL;
static lv_obj_t *bpm_val_label = NULL;
static lv_obj_t *status_label = NULL;
static lv_obj_t *chart_wave = NULL;
static lv_chart_series_t *wave_series = NULL;
static lv_obj_t *stat_avg_label = NULL;
static lv_obj_t *stat_minmax_label = NULL;
static lv_obj_t *stat_hrv_label = NULL;
static lv_obj_t *btn_start = NULL;
static lv_obj_t *btn_start_label = NULL;
static lv_timer_t *hr_timer = NULL;

/* -------------------------------------------------------------------
 * Advanced DSP Algorithm Implementation
 * ------------------------------------------------------------------- */

static void hr_algo_reset(hr_algo_t *algo)
{
    memset(algo, 0, sizeof(hr_algo_t));
    algo->min_bpm = 999.0f;
    algo->max_bpm = 0.0f;
    algo->filtered_bpm = 72.0f; /* Normal baseline default */
    algo->is_touching = true;
    algo->is_measuring = true;

    /* Try to find hardware RT-Thread sensor device */
    const char *sensor_names[] = {
        "hr_hrs33", "hr_vc32", "hr_gh30", "hr_pah80", "hr_sc7r3", "hr_sensor", "sensor"
    };
    algo->hw_sensor_present = false;
    algo->hr_device = NULL;

    for (size_t i = 0; i < sizeof(sensor_names)/sizeof(sensor_names[0]); i++)
    {
        rt_device_t dev = rt_device_find(sensor_names[i]);
        if (dev != NULL)
        {
            algo->hr_device = dev;
            algo->hw_sensor_present = true;
            break;
        }
    }
}

/**
 * Highpass + Lowpass Bandpass filter combined with Exponential Moving Average.
 * FC_highpass ~ 0.6 Hz, FC_lowpass ~ 3.5 Hz
 */
static float hr_algo_filter_sample(hr_algo_t *algo, float raw_sample)
{
    /* 1. High-Pass Filter (DC Offset & Motion Drift Removal) */
    float alpha_hp = 0.93f;
    algo->hp_y = alpha_hp * (algo->hp_y + raw_sample - algo->hp_x_prev);
    algo->hp_x_prev = raw_sample;

    /* 2. Low-Pass Filter (High-frequency noise attenuation) */
    float alpha_lp = 0.35f;
    algo->lp_y = algo->lp_y + alpha_lp * (algo->hp_y - algo->lp_y);

    return algo->lp_y;
}

/**
 * Process a single PPG sample and update BPM, Peak, HRV & Signal Quality
 */
static float hr_algo_process_sample(hr_algo_t *algo, float raw_sample)
{
    algo->sample_counter++;

    /* Filter the PPG signal */
    float filtered = hr_algo_filter_sample(algo, raw_sample);

    /* Store in rolling window */
    algo->ppg_window[algo->window_idx] = filtered;
    algo->window_idx = (algo->window_idx + 1) % PPG_WINDOW_SIZE;
    if (algo->window_count < PPG_WINDOW_SIZE)
        algo->window_count++;

    /* Compute Rolling Mean and Standard Deviation for Dynamic Thresholding */
    float sum = 0.0f;
    for (uint16_t i = 0; i < algo->window_count; i++)
    {
        sum += algo->ppg_window[i];
    }
    float mean = sum / (float)(algo->window_count > 0 ? algo->window_count : 1);

    float var_sum = 0.0f;
    for (uint16_t i = 0; i < algo->window_count; i++)
    {
        float diff = algo->ppg_window[i] - mean;
        var_sum += diff * diff;
    }
    float std_dev = sqrtf(var_sum / (float)(algo->window_count > 0 ? algo->window_count : 1));

    /* Off-Wrist / Contact Check: low variance means no pulse/skin contact */
    if (algo->window_count >= 25 && std_dev < 1.2f)
    {
        algo->is_touching = false;
        return filtered;
    }
    algo->is_touching = true;

    /* Dynamic Peak Threshold: Mean + 0.35 * StdDev */
    float threshold = mean + 0.35f * std_dev;

    /* Refractory Period Constraint: Min 280 ms (7 samples at 25Hz) between beats */
    uint32_t min_samples_between_beats = (uint32_t)(0.28f * PPG_SAMPLE_RATE_HZ);
    uint32_t samples_since_last_peak = algo->sample_counter - algo->last_peak_sample;

    algo->new_beat_detected = false;

    if (samples_since_last_peak >= min_samples_between_beats)
    {
        /* Check if peak condition met */
        if (filtered > threshold && filtered > algo->last_peak_val)
        {
            if (samples_since_last_peak < (uint32_t)(2.0f * PPG_SAMPLE_RATE_HZ)) /* max 2s between beats (~30 BPM) */
            {
                /* Beat detected! */
                algo->new_beat_detected = true;

                /* Calculate RR interval in milliseconds */
                float rr_ms = (float)samples_since_last_peak * (1000.0f / (float)PPG_SAMPLE_RATE_HZ);
                float raw_bpm = 60000.0f / rr_ms;

                /* Outlier Rejection & Physiological Range Clamping (45 - 200 BPM) */
                if (raw_bpm >= 45.0f && raw_bpm <= 200.0f)
                {
                    /* Rate of change limiter: max +/- 6 BPM change per beat */
                    float bpm_delta = raw_bpm - algo->filtered_bpm;
                    if (bpm_delta > 6.0f) raw_bpm = algo->filtered_bpm + 6.0f;
                    if (bpm_delta < -6.0f) raw_bpm = algo->filtered_bpm - 6.0f;

                    /* Exponential smoothing */
                    algo->filtered_bpm = 0.70f * algo->filtered_bpm + 0.30f * raw_bpm;
                    algo->current_bpm = algo->filtered_bpm;

                    /* Update Min/Max/Avg Statistics */
                    if (algo->current_bpm < algo->min_bpm) algo->min_bpm = algo->current_bpm;
                    if (algo->current_bpm > algo->max_bpm) algo->max_bpm = algo->current_bpm;

                    algo->bpm_sum += algo->current_bpm;
                    algo->bpm_sample_count++;
                    algo->avg_bpm = algo->bpm_sum / (float)algo->bpm_sample_count;

                    /* Update HRV (SDNN) Buffer */
                    algo->rr_intervals_ms[algo->rr_idx] = rr_ms;
                    algo->rr_idx = (algo->rr_idx + 1) % RR_BUFFER_SIZE;
                    if (algo->rr_count < RR_BUFFER_SIZE) algo->rr_count++;

                    if (algo->rr_count >= 4)
                    {
                        float rr_sum = 0.0f;
                        for (uint16_t k = 0; k < algo->rr_count; k++)
                            rr_sum += algo->rr_intervals_ms[k];
                        float rr_mean = rr_sum / (float)algo->rr_count;

                        float rr_var_sum = 0.0f;
                        for (uint16_t k = 0; k < algo->rr_count; k++)
                        {
                            float d = algo->rr_intervals_ms[k] - rr_mean;
                            rr_var_sum += d * d;
                        }
                        algo->hrv_sdnn_ms = sqrtf(rr_var_sum / (float)algo->rr_count);
                    }
                }
            }
            algo->last_peak_sample = algo->sample_counter;
        }
    }
    algo->last_peak_val = filtered;

    return filtered;
}

/**
 * Generate synthetic PPG wave if hardware sensor is absent/simulated.
 */
static float hr_algo_generate_synthetic_ppg(hr_algo_t *algo)
{
    static float t = 0.0f;
    t += 1.0f / (float)PPG_SAMPLE_RATE_HZ;

    float target_bpm = 72.0f + 3.0f * sinf(0.15f * t); /* Respiratory sinus arrhythmia */
    float freq = target_bpm / 60.0f;

    float cycle_pos = fmodf(t * freq, 1.0f);

    /* Cardiac pulse waveform: Systolic peak + Dicrotic notch */
    float ppg = 0.0f;
    if (cycle_pos < 0.35f)
    {
        /* Primary systolic wave */
        float p = cycle_pos / 0.35f;
        ppg = sinf(p * (float)M_PI);
    }
    else if (cycle_pos >= 0.35f && cycle_pos < 0.60f)
    {
        /* Secondary dicrotic wave */
        float p = (cycle_pos - 0.35f) / 0.25f;
        ppg = 0.35f * sinf(p * (float)M_PI);
    }

    /* Add slight baseline drift and white noise */
    float noise = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.05f;
    return (ppg + noise) * 50.0f + 100.0f;
}

/* -------------------------------------------------------------------
 * UI & Timer Callbacks
 * ------------------------------------------------------------------- */

static const char *hr_get_zone_name(float bpm)
{
    if (bpm < 60) return "Resting";
    if (bpm < 100) return "Normal";
    if (bpm < 130) return "Fat Burn";
    if (bpm < 160) return "Cardio";
    return "Peak";
}

static void hr_timer_cb(lv_timer_t *t)
{
    if (!g_hr_algo.is_measuring)
        return;

    float raw_sample = 0.0f;

#ifndef _WIN32
    if (g_hr_algo.hw_sensor_present && g_hr_algo.hr_device != NULL)
    {
        /* Read from RT-Thread sensor driver */
        struct rt_sensor_data data;
        rt_size_t res = rt_device_read(g_hr_algo.hr_device, 0, &data, 1);
        if (res > 0)
        {
            raw_sample = (float)data.data.hr;
        }
        else
        {
            raw_sample = hr_algo_generate_synthetic_ppg(&g_hr_algo);
        }
    }
    else
    {
        raw_sample = hr_algo_generate_synthetic_ppg(&g_hr_algo);
    }
#else
    raw_sample = hr_algo_generate_synthetic_ppg(&g_hr_algo);
#endif

    /* Run algorithm */
    float filtered_val = hr_algo_process_sample(&g_hr_algo, raw_sample);

    /* Update live waveform chart */
    if (chart_wave && wave_series)
    {
        lv_chart_set_next_value(chart_wave, wave_series, (lv_coord_t)(filtered_val * 1.5f + 50.0f));
    }

    /* Heartbeat animation pulse on beat detected */
    if (g_hr_algo.new_beat_detected && heart_img)
    {
        lv_img_set_zoom(heart_img, 300); /* Enlarge */
    }
    else if (heart_img)
    {
        uint16_t cur_zoom = lv_img_get_zoom(heart_img);
        if (cur_zoom > 256)
        {
            lv_img_set_zoom(heart_img, cur_zoom - 10); /* Decay back to standard size */
        }
    }

    /* Update UI text */
    if (!g_hr_algo.is_touching)
    {
        if (bpm_val_label) lv_label_set_text(bpm_val_label, "--");
        if (status_label) lv_label_set_text(status_label, "Place watch on wrist");
    }
    else if (g_hr_algo.bpm_sample_count == 0)
    {
        if (bpm_val_label) lv_label_set_text(bpm_val_label, "--");
        if (status_label) lv_label_set_text(status_label, "Measuring pulse...");
    }
    else
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", (int)roundf(g_hr_algo.current_bpm));
        if (bpm_val_label) lv_label_set_text(bpm_val_label, buf);

        snprintf(buf, sizeof(buf), "%s", hr_get_zone_name(g_hr_algo.current_bpm));
        if (status_label) lv_label_set_text(status_label, buf);

        snprintf(buf, sizeof(buf), "AVG: %d BPM", (int)roundf(g_hr_algo.avg_bpm));
        if (stat_avg_label) lv_label_set_text(stat_avg_label, buf);

        snprintf(buf, sizeof(buf), "MIN %d / MAX %d", (int)roundf(g_hr_algo.min_bpm), (int)roundf(g_hr_algo.max_bpm));
        if (stat_minmax_label) lv_label_set_text(stat_minmax_label, buf);

        snprintf(buf, sizeof(buf), "HRV: %d ms", (int)roundf(g_hr_algo.hrv_sdnn_ms));
        if (stat_hrv_label) lv_label_set_text(stat_hrv_label, buf);
    }
}

static void btn_start_cb(lv_event_t *e)
{
    g_hr_algo.is_measuring = !g_hr_algo.is_measuring;

    if (g_hr_algo.is_measuring)
    {
        if (btn_start_label) lv_label_set_text(btn_start_label, "Pause");
        if (btn_start) lv_obj_set_style_bg_color(btn_start, lv_palette_main(LV_PALETTE_ORANGE), 0);
    }
    else
    {
        if (btn_start_label) lv_label_set_text(btn_start_label, "Measure");
        if (btn_start) lv_obj_set_style_bg_color(btn_start, lv_palette_main(LV_PALETTE_RED), 0);
    }
}

static void btn_reset_cb(lv_event_t *e)
{
    hr_algo_reset(&g_hr_algo);
    if (bpm_val_label) lv_label_set_text(bpm_val_label, "--");
    if (status_label) lv_label_set_text(status_label, "Measuring pulse...");
    if (stat_avg_label) lv_label_set_text(stat_avg_label, "AVG: -- BPM");
    if (stat_minmax_label) lv_label_set_text(stat_minmax_label, "MIN -- / MAX --");
    if (stat_hrv_label) lv_label_set_text(stat_hrv_label, "HRV: -- ms");
}

/* -------------------------------------------------------------------
 * UI Initialization
 * ------------------------------------------------------------------- */

static void heartrate_ui_init(lv_obj_t *parent)
{
    main_cont = lv_obj_create(parent);
    lv_obj_set_size(main_cont, LV_HOR_RES_MAX, LV_VER_RES_MAX);
    lv_obj_set_style_bg_color(main_cont, lv_color_black(), 0);
    lv_obj_set_style_border_width(main_cont, 0, 0);
    lv_obj_set_style_pad_all(main_cont, 10, 0);

    /* Title Label */
    lv_obj_t *title = lv_label_create(main_cont);
    lv_label_set_text(title, "Heart Rate");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    /* Animated Pulsing Heart Icon */
    heart_img = lv_img_create(main_cont);
    lv_img_set_src(heart_img, LV_EXT_IMG_GET(img_red_heart));
    lv_obj_align(heart_img, LV_ALIGN_TOP_MID, -90, 55);

    /* Main BPM Value Display */
    bpm_val_label = lv_label_create(main_cont);
    lv_label_set_text(bpm_val_label, "--");
    lv_obj_set_style_text_color(bpm_val_label, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_text_font(bpm_val_label, &lv_font_montserrat_36, 0);
    lv_obj_align(bpm_val_label, LV_ALIGN_TOP_MID, 15, 52);

    lv_obj_t *bpm_unit_label = lv_label_create(main_cont);
    lv_label_set_text(bpm_unit_label, "BPM");
    lv_obj_set_style_text_color(bpm_unit_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(bpm_unit_label, &lv_font_montserrat_16, 0);
    lv_obj_align(bpm_unit_label, LV_ALIGN_TOP_MID, 80, 68);

    /* Status / Zone Label */
    status_label = lv_label_create(main_cont);
    lv_label_set_text(status_label, "Measuring pulse...");
    lv_obj_set_style_text_color(status_label, lv_palette_main(LV_PALETTE_LIGHT_BLUE), 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 110);

    /* Live Waveform Chart */
    chart_wave = lv_chart_create(main_cont);
    lv_obj_set_size(chart_wave, LV_HOR_RES_MAX - 40, 100);
    lv_obj_align(chart_wave, LV_ALIGN_TOP_MID, 0, 140);
    lv_chart_set_type(chart_wave, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart_wave, PPG_CHART_POINT_COUNT);
    lv_chart_set_range(chart_wave, LV_CHART_AXIS_PRIMARY_Y, -50, 150);
    lv_obj_set_style_bg_color(chart_wave, lv_palette_darken(LV_PALETTE_GREY, 4), 0);
    lv_obj_set_style_border_color(chart_wave, lv_palette_main(LV_PALETTE_RED), 0);

    wave_series = lv_chart_add_series(chart_wave, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);

    /* Statistics Card Container */
    lv_obj_t *stat_card = lv_obj_create(main_cont);
    lv_obj_set_size(stat_card, LV_HOR_RES_MAX - 40, 110);
    lv_obj_align(stat_card, LV_ALIGN_TOP_MID, 0, 255);
    lv_obj_set_style_bg_color(stat_card, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
    lv_obj_set_style_border_width(stat_card, 0, 0);

    stat_avg_label = lv_label_create(stat_card);
    lv_label_set_text(stat_avg_label, "AVG: -- BPM");
    lv_obj_set_style_text_color(stat_avg_label, lv_color_white(), 0);
    lv_obj_align(stat_avg_label, LV_ALIGN_TOP_LEFT, 5, 5);

    stat_minmax_label = lv_label_create(stat_card);
    lv_label_set_text(stat_minmax_label, "MIN -- / MAX --");
    lv_obj_set_style_text_color(stat_minmax_label, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(stat_minmax_label, LV_ALIGN_TOP_LEFT, 5, 32);

    stat_hrv_label = lv_label_create(stat_card);
    lv_label_set_text(stat_hrv_label, "HRV: -- ms");
    lv_obj_set_style_text_color(stat_hrv_label, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_align(stat_hrv_label, LV_ALIGN_TOP_LEFT, 5, 59);

    /* Control Buttons */
    btn_start = lv_btn_create(main_cont);
    lv_obj_set_size(btn_start, 110, 48);
    lv_obj_align(btn_start, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_set_style_bg_color(btn_start, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_add_event_cb(btn_start, btn_start_cb, LV_EVENT_CLICKED, NULL);

    btn_start_label = lv_label_create(btn_start);
    lv_label_set_text(btn_start_label, "Pause");
    lv_obj_center(btn_start_label);

    lv_obj_t *btn_reset = lv_btn_create(main_cont);
    lv_obj_set_size(btn_reset, 110, 48);
    lv_obj_align(btn_reset, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_set_style_bg_color(btn_reset, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(btn_reset, btn_reset_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_reset_label = lv_label_create(btn_reset);
    lv_label_set_text(btn_reset_label, "Reset");
    lv_obj_center(btn_reset_label);

    /* Start 25 Hz sampling timer (40ms interval) */
    if (!hr_timer)
    {
        hr_timer = lv_timer_create(hr_timer_cb, 40, NULL);
    }
}

static void on_start(void)
{
    cust_trans_anim_config(CUST_ANIM_TYPE_3, NULL);
    hr_algo_reset(&g_hr_algo);
    heartrate_ui_init(lv_scr_act());
}

static void on_stop(void)
{
    if (hr_timer)
    {
        lv_timer_del(hr_timer);
        hr_timer = NULL;
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

BUILTIN_APP_EXPORT(LV_EXT_STR_ID(HeartRate),
                   LV_EXT_IMG_GET(img_red_heart), APP_ID, app_main);
