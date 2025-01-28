/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

namespace gx {
/* 获取当前屏幕亮度，取值范围 0 ~255 */
int screen_brightness_get();
/* 设置屏幕亮度，取值范围 0 ~255 */
int screen_brightness_set(int brightness);
/* 获得当前屏幕亮度模式， 0 为手动调节屏幕亮度,1 为自动调节屏幕亮度 */
int screen_brightness_mode();
/* 设置当前屏幕亮度模式， 0 为手动调节屏幕亮度,1 为自动调节屏幕亮度 */
int screen_brightness_set_mode(int mode);
/* 熄灭屏幕 */
int screen_off();
/* 点亮屏幕 */
int screen_on();
/* 设置息屏时间 */
int screen_off_time_set(unsigned int time);
/* 设置屏幕是否常亮，0 关闭常亮，1 打开常亮 */
void screen_always_bright(int isAlways);
} // namespace gx
