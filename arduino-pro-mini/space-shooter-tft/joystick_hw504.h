/*
 * joystick_hw504.h — ماژول جویستیک HW-504
 * خواندن محور X، Y و دکمه SW با ناحیه مرده
 */

#ifndef SPACE_SHOOTER_JOYSTICK_HW504_H
#define SPACE_SHOOTER_JOYSTICK_HW504_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * مقدار خروجی جهت: -1 چپ/بالا، 0 بدون حرکت، +1 راست/پایین
 */
typedef int8_t joy_dir_t;

/**
 * مقداردهی اولیه پین‌های جویستیک.
 * باید یک بار در setup() فراخوانی شود.
 */
void joystick_init(void);

/**
 * خواندن وضعیت جویستیک.
 * @param dx  خروجی جهت افقی: -1 چپ، 0 وسط، +1 راست
 * @param dy  خروجی جهت عمودی: -1 بالا، 0 وسط، +1 پایین
 * @param fire خروجی دکمه: 0 رها، 1 فشرده
 */
void joystick_read(joy_dir_t *dx, joy_dir_t *dy, uint8_t *fire);

#ifdef __cplusplus
}
#endif

#endif /* SPACE_SHOOTER_JOYSTICK_HW504_H */
