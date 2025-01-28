/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_string.h"
#include "gx_variant.h"

namespace gx {
class Transform;
class TransformOperations;

// NOLINTBEGIN(readability-*)
namespace variant {
namespace detail {
// clang-format off
template<typename T> struct arg      { typedef const T &type; };
template<typename T> struct arg<T *> { typedef T *type; };
// clang-format on

class Converter {
public:
    explicit Converter(bool (*f)(Variant *variant, const void *) = nullptr,
                       const void *extra = nullptr)
        : m_converter(f), m_extra(extra) {}
    GX_NODISCARD bool isNull() const { return !m_converter; }
    bool operator()(Variant *variant) const { return m_converter(variant, m_extra); }

private:
    bool (*m_converter)(Variant *, const void *);
    const void *m_extra;
};

typedef Variant (*Evaluator)(const Variant &a, const Variant &b, float progress);

void install(Variant::type_t from, Variant::type_t to, Converter converter);
void install(Variant::type_t type, Evaluator evaluator);
Converter converter(Variant::type_t from, Variant::type_t to);
Evaluator evaluator(Variant::type_t type);
} // namespace detail

#define GX_VARIANT_CAST(T1, T2)                                                                    \
    template<> T2 variant::cast<T1, T2>(typename detail::arg<T1>::type x)

#define GX_VARIANT_TRY_CAST(T1, T2)                                                                \
    template<> std::pair<T2, bool> variant::try_cast<T1, T2>(typename detail::arg<T1>::type x)

// public function declaration
template<typename T1, typename T2> T2 cast(typename detail::arg<T1>::type x);
template<typename T1, typename T2> std::pair<T2, bool> try_cast(typename detail::arg<T1>::type x);
template<typename T> std::pair<T, bool> result(const T &x);
template<typename T> std::pair<T, bool> fail();
void setup(); // install all default converters and evaluators
void reset(); // unload all converters and evaluators

// public class declaration
template<typename T1, typename T2> class caster {
    static bool convert(Variant *variant, const void *) {
        GX_CRT_ASSERT(variant->is<T1>());
        T2 res = cast<T1, T2>(variant->get<T1>());
        variant->assign(utils::move(res));
        return true;
    }

public:
    typedef T1 from_type;
    typedef T2 value_type;
    caster() {
        detail::install(&Variant::typeId<T1>, &Variant::typeId<T2>, detail::Converter(&convert));
    }
};

template<typename T1, typename T2> class try_caster {
    static bool convert(Variant *variant, const void *) {
        GX_CRT_ASSERT(variant->is<T1>());
        std::pair<T2, bool> res = try_cast<T1, T2>(variant->get<T1>());
        if (!res.second)
            return false;
        variant->assign(utils::move(res.first));
        return true;
    }

public:
    typedef T1 from_type;
    typedef T2 value_type;
    try_caster() {
        detail::install(&Variant::typeId<T1>, &Variant::typeId<T2>, detail::Converter(&convert));
    }
};

template<typename Eval> class evaluator {
public:
    typedef typename Eval::value_type value_type;
    evaluator() { detail::install(&Variant::typeId<value_type>, &evaluate); }

private:
    static Variant evaluate(const Variant &a, const Variant &b, float progress) {
        return Variant(Eval()(a.get<value_type>(), b.get<value_type>(), progress));
    }
};

// public function definition
template<typename T> std::pair<T, bool> result(const T &x) { return std::pair<T, bool>(x, true); }

template<typename T> std::pair<T, bool> fail() { return std::pair<T, bool>(); }

template<typename T1, typename T2> T2 cast(typename detail::arg<T1>::type x) { return T2(x); }

template<typename T1, typename T2> std::pair<T2, bool> try_cast(typename detail::arg<T1>::type x) {
    return result(T2(x));
}
} // namespace variant
// NOLINTEND(readability-*)

// Default variant type converters
GX_VARIANT_TRY_CAST(String, int);
GX_VARIANT_TRY_CAST(String, float);
GX_VARIANT_TRY_CAST(String, double);
GX_VARIANT_CAST(String, bool);
GX_VARIANT_CAST(int, String);
GX_VARIANT_CAST(float, String);
GX_VARIANT_CAST(double, String);
GX_VARIANT_CAST(bool, String);
GX_VARIANT_CAST(AlignmentFlag, String);
GX_VARIANT_CAST(String, AlignmentFlag);
GX_VARIANT_CAST(ObjectFitMode, String);
GX_VARIANT_CAST(String, ObjectFitMode);
GX_VARIANT_CAST(Transform, TransformOperations);
GX_VARIANT_CAST(TransformOperations, Transform);
} // namespace gx
