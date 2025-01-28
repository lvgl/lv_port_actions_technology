/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_hashmap.h"
#include "gx_sharedref.h"
#include "gx_string.h"
#include "gx_vector.h"

namespace gx {
class ByteArray;

class JsonValue {
public:
    enum Type { Null, Boolean, Number, String, Array, Object };

    JsonValue() : m_type(Null) {}                             // NOLINT(*-pro-type-member-init)
    JsonValue(bool value) : m_type(Boolean) { u.i = value; }  // NOLINT(*-pro-type-member-init)
    JsonValue(int value) : m_type(Number) { u.d = value; }    // NOLINT(*-pro-type-member-init)
    JsonValue(double value) : m_type(Number) { u.d = value; } // NOLINT(*-pro-type-member-init)
    JsonValue(const gx::String &string);
    JsonValue(const char *string);
    JsonValue(const char *string, std::size_t length);
    JsonValue(const JsonValue &other);
    ~JsonValue();

    GX_NODISCARD Type type() const { return m_type; }
    GX_NODISCARD bool isNull() const { return type() == Null; }
    GX_NODISCARD bool isBoolean() const { return type() == Boolean; }
    GX_NODISCARD bool isNumber() const { return type() == Number; }
    GX_NODISCARD bool isString() const { return type() == String; }
    GX_NODISCARD bool isObject() const { return type() == Object; }
    GX_NODISCARD bool isArray() const { return type() == Array; }

    GX_NODISCARD bool toBoolean() const { return isBoolean() && u.i; }
    GX_NODISCARD int toInt(int def = 0) const { return isNumber() ? int(u.d) : def; }
    GX_NODISCARD float toFloat(float def = 0) const { return isNumber() ? float(u.d) : def; }
    GX_NODISCARD double toNumber(double def = 0) const { return isNumber() ? u.d : def; }
    GX_NODISCARD gx::String toString(const gx::String &def = gx::String()) const;
    GX_NODISCARD const Vector<JsonValue> &array() const;
    GX_NODISCARD const HashMap<gx::String, JsonValue> &object() const;
    GX_NODISCARD ByteArray serialize() const;

    GX_NODISCARD bool empty() const;
    GX_NODISCARD std::size_t size() const;
    GX_NODISCARD bool contains(int index) const;
    GX_NODISCARD bool contains(const gx::String &key) const;

    void push_back(const JsonValue &value); // NOLINT(readability-*)
    bool set(int index, const JsonValue &value);
    bool set(const gx::String &key, const JsonValue &value);
    GX_NODISCARD JsonValue operator[](int index) const;
    GX_NODISCARD JsonValue operator[](const gx::String &key) const;

    JsonValue &operator=(const JsonValue &other);

    static JsonValue newObject();
    static JsonValue newArray();
    static JsonValue read(const ByteArray &data);
    static JsonValue read(const uint8_t *data, std::size_t size);
    static JsonValue read(const uint8_t *first, const uint8_t *last);
    static JsonValue parse(const gx::String &source); // TODO

private:
    explicit JsonValue(Type type) : m_type(type) {} // NOLINT(*-pro-type-member-init)
    template<class T> T &get() { return reinterpret_cast<T &>(u); }
    template<class T> const T &get() const { return reinterpret_cast<const T &>(u); }
    template<class T> SharedRef<T> &ref() { return get<SharedRef<T> >(); }
    template<class T> const SharedRef<T> &ref() const { return get<SharedRef<T> >(); }

private:
    struct ObjectData;
    struct ArrayData;

    Type m_type;
    union {
        int i;
        double d;
        uint8_t s[sizeof(String)];
        uint8_t o[sizeof(SharedRef<ObjectData>)];
        uint8_t a[sizeof(SharedRef<ArrayData>)];
    } u;
};

// NOLINTNEXTLINE(*-pro-type-member-init)
inline JsonValue::JsonValue(const gx::String &string) : m_type(String) {
    new (&u) gx::String(string);
}

// NOLINTNEXTLINE(*-pro-type-member-init)
inline JsonValue::JsonValue(const char *string) : m_type(String) { new (&u) gx::String(string); }

// NOLINTNEXTLINE(*-pro-type-member-init)
inline JsonValue::JsonValue(const char *string, std::size_t length) : m_type(String) {
    new (&u) gx::String(string, length);
}

inline gx::String JsonValue::toString(const gx::String &def) const {
    return isString() ? get<gx::String>() : def;
}

inline JsonValue JsonValue::read(const uint8_t *data, std::size_t size) {
    return read(data, data + size);
}
} // namespace gx
