/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_string.h"
#include <cstddef>

namespace gx {
const char *splitPath(const char *begin, const char *end);
const char *splitFileName(const char *begin, const char *end);
int str2int(const char *str, const char **end);
float str2float(const char *str, const char **end);
int int2str(char *str, std::size_t size, int x);
String bin2hex(const uint8_t *begin, const uint8_t *end);
} // namespace gx
