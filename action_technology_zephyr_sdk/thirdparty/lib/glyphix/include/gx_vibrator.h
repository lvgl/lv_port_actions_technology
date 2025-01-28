/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

namespace gx {
enum VibratorMode { LongVibrate, ShortVibrate };

/* 触发振动, LongVibrate 为长振动，ShortVibrate 为短振动 */
bool vibrator_set(VibratorMode mode);
}
