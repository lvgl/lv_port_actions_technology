/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

/* DO NOT CHANGE ANYTHING! */

/* App Runtime version numbers. */
#define GX_RUNTIME_MAJOR_VERSION  0
#define GX_RUNTIME_MINOR_VERSION  3
#define GX_RUNTIME_PATCH_VERSION  0
#define GX_RUNTIME_BUILD_VERSION  "49"
#define GX_RUNTIME_VERSION_STRING "0.3.0-49"

/* Git commit hash and date. */
#define GX_RUNTIME_GIT_COMMIT_HASH "940d62f8"
#define GX_RUNTIME_GIT_COMMIT_DATE "Tue Nov 5 17:28:28 2024"

/** Get the Glyphix version code from MAJOR, MINOR, and PATCH version. */
#define GX_RUNTIME_VERSION_CODE(MAJOR, MINOR, PATCH) ((MAJOR) * 1000000 + (MINOR) * 1000 + (PATCH))

/** Glyphix Nucleus version code. */
#define GX_RUNTIME_VERSION GX_VERSION_CODE(GX_MAJOR_VERSION, GX_MINOR_VERSION, GX_PATCH_VERSION)
