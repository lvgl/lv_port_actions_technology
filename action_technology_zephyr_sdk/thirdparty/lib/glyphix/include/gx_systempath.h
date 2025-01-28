/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_internal.h"

namespace gx {
#define AUTHORITY_SPLIT "."
#define APP_AUTHORITY   "app"
#define DIAL_AUTHORITY  "dial"
#define PRC_PATH_SPLIT  "/@/"

class SystemPath {
public:
    enum PathTags {
        Fonts = 0,
        Icons,
        Images,
        Modules,
        Widgets,
        SystemApps,
        UserApps,
        SystemDials,
        UserDials,
        Data,
        Download,
        Temp,
        UnknownPath,
    };
    enum UserPermission { AccessDenied = 0, Read = 1, Write = 2, ReadWrite = Read | Write };
    static const char *path(int tag);
    static const char *authority(int tag);
    static int permission(int tag);
    static void setItem(int tag, const char *path, const char *authority,
                        int permission = AccessDenied);

private:
    struct Private;
};
} // namespace gx
