/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_application.h"
#include "gx_style.h"

namespace gx {
void coreModulesMount(CoreApplication *app);
void coreModulesUnmount(CoreApplication *app);
void modulesMount(Application *app);
void modulesUnmount(Application *app);
} // namespace gx

// NOLINTBEGIN(bugprone-macro-parentheses)
#define GX_CORE_MODULES_MOUNT(app)   void ::gx::coreModulesMount(CoreApplication *app)
#define GX_CORE_MODULES_UNMOUNT(app) void ::gx::coreModulesUnmount(CoreApplication *app)
#define GX_MODULES_MOUNT(app)        void ::gx::modulesMount(Application *app)
#define GX_MODULES_UNMOUNT(app)      void ::gx::modulesUnmount(Application *app)
// NOLINTEND(bugprone-macro-parentheses)
