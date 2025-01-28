/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */

#pragma once
#include "gx_hashmap.h"
#include "gx_string.h"

namespace gx {
String mas_sign_rsa(String text, String key, String algorithm);

struct exec_rpc_result {
    int code;
    String msg;
    String data;
};

exec_rpc_result mas_exec_rpc(String name, String appId, String params, int timeout);
} // namespace gx
