#ifndef VIBRATOR_PERF_H
#define VIBRATOR_PERF_H

#include <stdint.h>
#include <stddef.h>

void vibrator_init(void);
void vibrator_on(void);
void vibrator_off(void);
void vibrator_buzz(uint16_t ms);
void vibrator_burst_pattern(const uint16_t *bursts, size_t count);

#endif