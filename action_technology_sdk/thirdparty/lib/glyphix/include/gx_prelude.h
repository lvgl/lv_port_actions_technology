/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_assert.h"
#include "gx_version.h"

#if defined(__CC_ARM) && !defined(GX_COPMATIBLE_CXX_98)
#define GX_COPMATIBLE_CXX_98
#endif

#ifdef GX_COPMATIBLE_CXX_98
#include <stdint.h>
#else
#include <cstdint>
#endif

#ifdef GX_COPMATIBLE_CXX_98
class nullptr_t {
public:
    template<class T> inline operator T *() const { return 0; }
    template<class C, class T> inline operator T C::*() const { return 0; }

private:
    void operator&() const;
};

// clang-format off
#define nullptr nullptr_t()
#define GX_OVERRIDE
#define GX_FINAL
#define GX_DEFAULT {}
#define GX_DELETE
#define GX_NOEXCEPT
#define GX_CONSTEXPR
#define GX_STATIC_ASSERT
// clang-format on
#else
#define GX_OVERRIDE           override
#define GX_FINAL              final
#define GX_DEFAULT            = default
#define GX_DELETE             = delete
#define GX_NOEXCEPT           noexcept
#define GX_CONSTEXPR          constexpr
#define GX_STATIC_ASSERT(...) static_assert(__VA_ARGS__)
#endif

#if __cplusplus >= 201703L || (defined(_MSC_VER) && _MSC_VER >= 1920)
#define GX_NODISCARD   [[nodiscard]]
#define GX_UNUSED(var) ((void)(var))
#elif defined(__GNUC__) || defined(__clang__)
#define GX_NODISCARD   __attribute__((warn_unused_result))
#define GX_UNUSED(var) ::gx::discard_value(var)
#else
#define GX_NODISCARD
#define GX_UNUSED(var) ((void)(var))
#endif

#if __cplusplus >= 201402L || (defined(_MSC_VER) && _MSC_VER >= 1900)
#define GX_DEPRECATED          [[deprecated]]
#define GX_DEPRECATED_MSG(msg) [[deprecated(msg)]]
#elif defined(__GNUC__) || defined(__clang__)
#define GX_DEPRECATED          __attribute__((deprecated))
#define GX_DEPRECATED_MSG(msg) __attribute__((deprecated(msg)))
#else
#define GX_DEPRECATED
#define GX_DEPRECATED_MSG(msg)
#endif

#if __GNUC__ || __clang__
#define GX_LIKELY(x)   __builtin_expect(!!(x), 1)
#define GX_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define GX_LIKELY(x)   (x)
#define GX_UNLIKELY(x) (x)
#endif

#ifdef GX_HARDWARE_A4_PIXEL
#define GX_FONT_GLYPH_DEPTH 4
#else
#define GX_FONT_GLYPH_DEPTH 8
#endif

#if GX_FONT_GLYPH_DEPTH != 8 && GX_FONT_GLYPH_DEPTH != 4 && GX_FONT_GLYPH_DEPTH != 2
#error "Invalid GX_FONT_GLYPH_DEPTH, it must be 2, 4, or 8"
#endif

// remove some macro definitions
#undef min
#undef max

namespace gx {
class Logger;

//! The BiDi layout direction. Control the layout direction of UI elements (e.g. text, images).
enum LayoutDirection {
    LayoutAuto = 0, //!< The layout mode inherited from the parent widget.
    LayoutLTR = 1,  //!< Left-to-right layout mode (default), which is the most typical layout.
    LayoutRTL = 2   //!< Right to left layout, currently unused.
};

enum AlignmentFlag {
    AlignLeft = 0x0001,
    AlignRight = 0x0002,
    AlignHCenter = 0x0004,
    AlignJustify = 0x0008,
    AlignTop = 0x0010,
    AlignBottom = 0x0020,
    AlignVCenter = 0x0040,
    AlignBaseline = 0x0080,
    AlignCenter = AlignHCenter | AlignVCenter,
    AlignHorizontalMask = 0x000f,
    AlignVerticalMask = 0x00f0,
    AlignmentMask = 0x00ff
};

//! The text overflow mode.
enum TextOverflowMode {
    TextClip = 0x0000,        //!< The overflowed text is clipped.
    TextEllipsis = 0x0100,    //!< The overflowed text is replaced with ellipsis.
    TextOverflowMask = 3 << 8 //!< The TextOverflowMode value mask.
};

//! The BiDi text layout direction.
enum TextLayoutDirection {
    TextLayoutAuto = LayoutAuto << 10, //!< The text layout direction inherited from the parent
                                       //!< widget (default).
    TextLayoutLTR = LayoutLTR << 10,   //!< Left-to-right text layout direction.
    TextLayoutRTL = LayoutRTL << 10,   //!< Right-to-left text layout direction.
    TextLayoutDirectionMask = 3 << 10  //!< The TextLayoutDirection value mask.
};

enum ObjectFitMode {
    ObjectFitNone = 0x0000,
    ObjectFitContain = 0x1000,
    ObjectFitFill = 0x2000,
    ObjectFitCover = 0x3000,
    ObjectFitScaleDown = 0x4000,
    ObjectFitMask = 0xf000,
};

enum Direction { NoDirection = 0, Horizontal = 1, Vertical = 2, AllDirections = 3 };

//! Manipulate the pixel format.
class PixelFormat {
public:
    /**
     * Enumerates the pixel formats built into Glyphix. Glyphix only uses the int type to
     * represent the pixel format without restricting it in Format, so you can extend the
     * pixel format as needed for specific hardware.
     * @see Pixmap
     */
    enum Format {
        RGB565,        //!< RGB format, 16bpp.
        RGB888,        //!< RGB format, 24bpp.
        XRGB8888,      //!< RGB format, 32bpp, X is a useless channel.
        ARGB8888,      //!< ARGB format, 32bpp, support an alpha channel.
        ARGB4444,      //!< ARGB format, 16bpp, support an alpha channel.
        A8,            //!< Alpha format, 8bpp.
        A4,            //!< Alpha format, 4bpp.
        A2,            //!< Alpha format, 2bpp.
        Mono,          //!< Binary color type, not yet supported.
        Palette,       //!< Palette format, work with Color Lookup Table (CLUT) to map pixel values.
        UnknownFormat, //!< Unknown pixel format, which implies an illegal value (do not use).
        //! User-defined pixel formats starting from this enumeration value. Glyphix does not
        //! recognise these formats, therefore, you need implement your own paint engine. The user
        //! defined pixel format is not cross platform, this means that the same enumeration value
        //! may have different meanings on different platforms and cannot be used universally.
        UserFormat
    };
    //! Get the bit depth of the specified pixel format. This function does not recognise custom
    //! pixel formats.
    //! @param pixfmt A pixel format value. It must be a value in the \ref Format enumeration that
    //! is smaller than \ref UnknownFormat.
    //! @return The bit depth of the pixel format by \p pixfmt.
    static int depth(int pixfmt);
    //! Gets the number of bytes occupied by a row of pixels in the specified pixel format. This
    //! function does not recognise custom pixel formats.
    //! @param pixfmt A pixel format value. It must be a value in the \ref Format enumeration that
    //! is smaller than \ref UnknownFormat.
    //! @param length Number of pixels.
    //! @return The number of bytes occupied by \p length pixels by \p pixfmt format.
    static int pitch(int pixfmt, int length);
    //! Gets the name string for the specified pixel format.
    //! @param pixfmt A pixel format value. It must be a value in the \ref Format enumeration that
    //! is smaller than \ref UnknownFormat.
    //! @return The name string of pixel format specified by \p pixfmt.
    static const char *name(int pixfmt);
};

/**
 * @brief Get the lesser of two values.
 * @tparam T The type of the values being compared (must support the less than operator `<`).
 * @param x The first value to compare.
 * @param y The second value to compare.
 * @return The smaller of `x` and `y`.
 */
template<typename T> const T &min(const T &x, const T &y) { return x < y ? x : y; }

/**
 * @brief Get the greater of two values.
 * @tparam T The type of the values being compared (must support the greater than operator `>`).
 * @param x The first value to compare.
 * @param y The second value to compare.
 * @return The greater of `x` and `y`.
 */
template<typename T> const T &max(const T &x, const T &y) { return x > y ? x : y; }

/**
 * @brief Computes the absolute value of a generic arithmetic type.
 * @tparam T The numeric type of the value for which the absolute value is calculated
 *           (must support comparison and negation operators).
 * @param x The input value to find the absolute value of.
 * @return The absolute value of \p x.
 */
template<typename T> T abs(T x) { return x < 0 ? -x : x; }

/**
 * @brief Bounds a value within a specified range.
 *
 * This function ensures that the value falls within the inclusive range defined by `lower` and
 * `upper`, that is `lower <= value <= upper`.
 * @tparam T The type of the values defining the range and the value being bounded
 *           (must support the less than and greater than operators).
 * @param lower The lower bound of the range.
 * @param value The value to be bounded within the range.
 * @param upper The upper bound of the range.
 * @return The bounded value, guaranteed to be within the `[lower, upper]` range.
 */
template<typename T> const T &bound(const T &lower, const T &value, const T &upper) {
    return max(lower, min(value, upper));
}

/**
 * @brief Tests if a float value is approximately zero within a specified tolerance.
 *
 * This function checks whether the given float value `a` is effectively zero, taking into
 * account a tolerance value `epsilon`. It returns `true` if `a` lies within the interval
 * (-epsilon, epsilon), and `false` otherwise.
 * @param a The float value to evaluate.
 * @param epsilon A small positive value that defines the acceptable deviation from zero.
 *                The choice of epsilon depends on the desired level of precision.
 * @return True if `a` is within `epsilon` of zero, false otherwise.
 */
inline bool fuzzyZero(float a, float epsilon) { return a > -epsilon && a < epsilon; }

/**
 * @brief Tests if a float value is approximately zero using a default tolerance.
 *
 * This function is a convenience overload of `fuzzyZero()` that employs a default tolerance value
 * of `1e-5f`. It checks whether the given float value `a` is effectively zero within this
 * tolerance.
 */
inline bool fuzzyZero(float a) { return fuzzyZero(a, 1e-5f); }

/**
 * @brief Compares two floating-point values for approximate equality.
 *
 * This function determines if \p a and \p b are equal within a specified tolerance \p epsilon.
 * It calculates the absolute difference between \p a and \p b, then compares it with the minimum
 * absolute value of \p a and \p b multiplied by \p epsilon.
 * @param a The first floating-point value to compare.
 * @param b The second floating-point value to compare.
 * @param epsilon A small positive value that defines the acceptable difference between \p a and
 *                \p b. This value should be chosen based on the desired level of precision.
 * @return True if the difference between \p a and \p b is less than or equal to \p epsilon
 *         times the minimum of their absolute values, false otherwise.
 */
inline bool fuzzyEqual(float a, float b, float epsilon) {
    return abs(a - b) <= min(abs(a), abs(b)) * epsilon;
}

/**
 * @brief Compares two floating-point values for approximate equality using a default tolerance.
 *
 * This function is a convenience overload of `fuzzyEqual()` that employs a default tolerance
 * value of `1e-5f`. It checks whether `a` and `b` are equal within this tolerance.
 */
inline bool fuzzyEqual(float a, float b) { return fuzzyEqual(a, b, 1e-5f); }

/**
 * @brief Tests if a double-precision floating-point value is approximately zero within a tolerance.
 *
 * This function determines whether the given double value `a` is effectively zero, considering
 * a small tolerance represented by `epsilon`.
 * @param a The floating-point value to test.
 * @param epsilon A small positive value that defines the acceptable deviation from zero.
 *                Typically, this value is chosen based on the desired level of precision.
 * @return True if `a` is within `epsilon` of zero, false otherwise.
 */
inline bool fuzzyZero(double a, double epsilon) { return a > -epsilon && a < epsilon; }

/**
 * @brief Determines if a double-precision floating-point number is approximately zero using a
 * default tolerance.
 *
 * This function checks if the provided double value `a` is effectively zero within a default
 * tolerance of `1e-5`. It serves as a convenience overload of `fuzzyZero()` with a predefined
 * epsilon.
 * @param a The double-precision floating-point number to check.
 * @return True if `a` is approximately zero, false otherwise.
 */
inline bool fuzzyZero(double a) { return fuzzyZero(a, 1e-5); }

/**
 * @brief Compares two double-precision floating-point numbers for approximate equality.
 *
 * This function checks if `a` and `b` are approximately equal within a specified tolerance
 * `epsilon`. It calculates the absolute difference between `a` and `b`, then compares it
 * with the minimum absolute value of `a` and `b` multiplied by `epsilon`.
 * @param a The first double-precision floating-point number to compare.
 * @param b The second double-precision floating-point number to compare.
 * @param epsilon A small positive value that defines the acceptable difference between `a` and `b`.
 *                This value should be chosen based on the desired level of precision.
 * @return True if the difference between `a` and `b` is less than or equal to `epsilon`
 *         times the minimum of their absolute values, false otherwise.
 */
inline bool fuzzyEqual(double a, double b, double epsilon) {
    return abs(a - b) <= min(abs(a), abs(b)) * epsilon;
}

/**
 * @brief Checks approximate equality between two double-precision numbers using a default
 * tolerance.
 *
 * This function is a convenience overload of `fuzzyEqual()` that employs a default tolerance
 * value of `1e-5`. It tests whether `a` and `b` are nearly equal within this tolerance.
 */
inline bool fuzzyEqual(double a, double b) { return fuzzyEqual(a, b, 1e-5); }

template<typename Out, typename In> Out function_pointer_cast(In in) { // NOLINT(readability-*)
    // Suppress warnings with the -Wcast-function-type option.
    return *reinterpret_cast<Out *>(&in);
}

//! Discard the value to avoid compiler warnings.
template<typename T> void discard_value(const T &) {} // NOLINT(*-identifier-naming)

/* Borrowed from Boost::noncopyable. Add the template to give each noncopyable
 * a different signature. */
template<typename T> class NonCopyable {
protected:
    NonCopyable() GX_DEFAULT;
    ~NonCopyable() GX_DEFAULT;

#ifndef GX_COPMATIBLE_CXX_98
public:
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;
#else
private: // emphasize the following members are private
    NonCopyable(const noncopyable &);
    NonCopyable &operator=(const noncopyable &);
#endif
};

#if !defined(DOXYGEN_DOCUMENTATION_BUILD)
// NOLINTBEGIN(bugprone-macro-parentheses)
#define GX_FLAGS(Enum)                                                                             \
    friend inline Enum operator|(Enum a, Enum b) { return Enum(int(a) | int(b)); }                 \
    friend inline Enum operator&(Enum a, Enum b) { return Enum(int(a) & int(b)); }                 \
    friend inline Enum operator~(Enum a) { return Enum(~int(a)); }                                 \
    friend inline Enum &operator|=(Enum &a, Enum b) { return a = Enum(int(a) | int(b)); }          \
    friend inline Enum &operator&=(Enum &a, Enum b) { return a = Enum(int(a) & int(b)); }
// NOLINTEND(bugprone-macro-parentheses)
#else
#define GX_FLAGS(Enum)
#endif

const Logger &operator<<(const Logger &, PixelFormat::Format);
} // namespace gx
