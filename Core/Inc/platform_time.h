#ifndef PLATFORM_TIME_H
#define PLATFORM_TIME_H

#include <stdint.h>

/**
 * @brief Initialize the microsecond timer timebase.
 */
void platform_time_init(void);

/**
 * @brief Get current microseconds from TIM2.
 */
uint32_t micros(void);

/**
 * @brief Microsecond busy-wait delay.
 */
void micros_delay(uint32_t us);

#endif /* PLATFORM_TIME_H */
