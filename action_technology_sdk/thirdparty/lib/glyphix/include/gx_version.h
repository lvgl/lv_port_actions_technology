/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

/* DO NOT CHANGE ANYTHING! */

/* Glyphix Nucleus version numbers. */
#define GX_MAJOR_VERSION  0
#define GX_MINOR_VERSION  3
#define GX_PATCH_VERSION  1
#define GX_BUILD_VERSION  "4"
#define GX_VERSION_STRING "0.3.1-4"

/** Minimum meta version required. */
#define GX_META_VERSION_REQUIRE 1015

/* Git commit hash and date. */
#define GX_GIT_COMMIT_HASH "5a524a75"
#define GX_GIT_COMMIT_DATE "Mon Nov 4 13:26:20 2024"

/** Get the Glyphix version code from MAJOR, MINOR, and PATCH version. */
#define GX_VERSION_CODE(MAJOR, MINOR, PATCH) ((MAJOR) * 1000000 + (MINOR) * 1000 + (PATCH))

/** Glyphix Nucleus version code. */
#define GX_VERSION GX_VERSION_CODE(GX_MAJOR_VERSION, GX_MINOR_VERSION, GX_PATCH_VERSION)
