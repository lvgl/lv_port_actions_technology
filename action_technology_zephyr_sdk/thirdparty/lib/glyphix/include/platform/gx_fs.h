/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "platform/gx_os.h"
#include <cstddef>

namespace gx {
namespace os {
void chroot(const char *path);
file_t open(const char *path, int mode);
bool close(file_t file);
int write(file_t file, const void *buffer, std::size_t count);
int read(file_t file, void *buffer, std::size_t count);
off_t lseek(file_t file, off_t offset, int whence);
void fsync(file_t file);
void *rawptr(file_t file);
bool exists(const char *path);
char *getcwd(char *buffer, int length);
bool isfile(const char *path);
bool isdir(const char *path);
bool mkdir(const char *path);
bool rename(const char *from, const char *to);
bool remove(const char *path);
bool dirfirst(dirinfo_t *info, const char *path);
bool dirnext(dirinfo_t *info);
bool dirclose(dirinfo_t *info);
} // namespace os
} // namespace gx
