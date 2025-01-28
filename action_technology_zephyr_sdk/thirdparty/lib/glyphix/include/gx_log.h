/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include <stdio.h>

#define LOG_I(...) printf(__VA_ARGS__); printf("\n")
#define LOG_W(...) printf(__VA_ARGS__); printf("\n")
#define LOG_E(...) printf(__VA_ARGS__); printf("\n")
#define LOG_D(...) printf(__VA_ARGS__); printf("\n")