/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_hashmap.h"
#include "gx_resourceinterface.h"

namespace gx {
class ResourceInterfaceMem : public ResourceInterface {
    GX_OBJECT

public:
    bool set(const String &uri, const void *ptr, std::size_t size);

    virtual Location locate(const String &domain);
    virtual Status resolve(const String &package, Uri *uri);
    virtual ResourceFile *open(const Uri &uri, int flags);
    void refresh(const String &package);

private:
    HashMap<String, std::pair<const void *, std::size_t> > m_map;
};
} // namespace gx
