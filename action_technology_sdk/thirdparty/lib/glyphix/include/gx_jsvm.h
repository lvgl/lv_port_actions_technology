/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_bytearray.h"
#include "gx_string.h"
#include "gx_vector.h"
#include <deque>
#ifndef GX_COPMATIBLE_CXX_98
#include <functional>
#include <utility>
#endif

// Enable JerryScript by default
#if !defined(GX_ENABLE_JERRYSCRIPT) && !defined(GX_ENABLE_QUICKJS)
#define GX_ENABLE_JERRYSCRIPT
#endif // !GX_ENABLE_JERRYSCRIPT

namespace gx {
class Variant;
class TaskEvent;
class PrimitiveObject;
class JsonValue;
class JsValue;
class JsFunctor;
class JsCallContext;

typedef Vector<JsValue> JsValueList;
typedef JsValue (*JsFunction)(const JsCallContext &context);

/**
 * The JsValue class provides interaction between JavaScript and C++ and is actually a wrapper
 * around the JavaScript interpreter. The JsValue instance will refer to the JavaScript value until
 * destructed.
 *
 * The reference count of a newly constructed JavaScript value is 1. This reference count is
 * increased or decreased when copying or destructing, and the JavaScript interpreter internally
 * changes the reference count of the value. If you only use the C++ API provided by the JsVM
 * framework (e.g. JsValue and JsVM classes), then you don't need to care about reference counting
 * and just use C++ value operations such as assignment and copy (just like int, float types).
 *
 * JsValue supports printing using the Logger framework, for example:
 * ```
 * LogInfo() << JsValue("Lucky number is") << JsValue(8); // "Lucky number is 8"
 * ```
 * In fact, Glyphix's implementation of the `console` module in JavaScript is a wrapper around the
 * Logger framework.
 */
class JsValue {
public:
#ifdef GX_NATIVE_JSVALUE
    typedef GX_NATIVE_JSVALUE native_value; // NOLINT(readability-*)
#else
#ifdef GX_ENABLE_QUICKJS
#if INTPTR_MAX >= INT64_MAX
    /**
     * The native value type of the JavaScript engine. The native_value has different definitions
     * for different JavaScript engines.
     * @see toNative()
     * @see fromNative()
     */
    struct native_value { // NOLINT(readability-*)
        union {
            int32_t int32;
            int64_t int64;
            void *ptr;
        } u;
        int64_t tag;
        bool operator==(const native_value &rhs) const {
            return tag == rhs.tag && u.int64 == rhs.u.int64;
        }
        bool operator<(const native_value &rhs) const {
            return tag < rhs.tag || u.int64 < rhs.u.int64;
        }
    };
#else
    /**
     * The native value type of the JavaScript engine. The native_value has different definitions
     * for different JavaScript engines.
     * @see toNative()
     * @see fromNative()
     */
    typedef uint64_t native_value;
#endif
#else // GX_ENABLE_QUICKJS
    /**
     * The native value type of the JavaScript engine. The native_value has different definitions
     * for different JavaScript engines.
     * @see toNative()
     * @see fromNative()
     */
    typedef uint32_t native_value; // NOLINT(readability-*)
#endif
#endif

    /**
     * Enumerations of JavaScript value types.
     * @see type()
     */
    enum ValueType {
        TypeUndefined, //!< The **undefined** type.
        TypeNull,      //!< The **null** type.
        TypeBoolean,   //!< The **boolean** type.
        TypeNumber,    //!< The **number** type.
        TypeString,    //!< The **string** type.
        TypeObject,    //!< The **object** type. To distinguish between subtypes of objects (e.g.
                       //!< functions, arrays, etc.), use methods such as \ref isFunction(), \ref
                       //!< isArray(), etc.
        TypeException, //!< The **exception** type.
        TypeUnknown    //!< Unknown type, it should not be used.
    };
    /**
     * Enumerations of JavaScript promise state.
     * @see promiseState()
     */
    enum PromiseState {
        PromiseStateNone, //!< Invalid/unknown state (a non-promise value).
        PromisePending,   //!< Promise is in "Pending" state.
        PromiseFulfilled, //!< Promise is in "Fulfilled" state.
        PromiseRejected   //!< Promise is in "Rejected" state.
    };

    /**
     * Default constructor that constructs a JavaScript **undefined** value. These constructors
     * cannot construct **null** and **object** types and require the JsVM::newXxx() family of
     * functions.
     * @see JsVM::newNull() @see JsVM::newObject()
     */
    JsValue();
    /**
     * Constructs a JavaScript Boolean value. Since C++ allows implicit conversion of many types to
     * bool, that means that expressions like `JsValue((void *)nullptr)` are converted to bool.
     * Watch out for this!
     */
    JsValue(bool value);
    //! Constructs a JavaScript Number value from an integer.
    JsValue(int value);
    //! Constructs a JavaScript Number value from a double.
    JsValue(double value);
    //! Constructs a JavaScript String value from a String object.
    JsValue(const String &string);
    //! Constructs a JavaScript String value from a C-style string.
    JsValue(const char *string);
    //! Constructs a JavaScript String value from a C-style string with length.
    JsValue(const char *string, std::size_t length);
    //! Export a C++ function as a JavaScript function.
    JsValue(JsFunction function);
    //! Export a JsFunctor object as a JavaScript function.
    JsValue(JsFunctor *function);
#ifndef GX_COPMATIBLE_CXX_98
    /**
     * Export a std::function<> object as a JavaScript function. This function is only available in
     * compilers that support C++11 or higher. You can use Lambda expressions to write very concise
     * code, for example:
     * ```
     * #include "gx_jsvm.h"
     * #include "gx_logger.h"
     *
     * JsValue f{[&](const JsCallContext &ctx) { return ctx.arg(0).toInt() * 5; }};
     * LogInfo() << f.call(JsValue(), {3}); // 15
     * ```
     */
    JsValue(std::function<JsValue(const JsCallContext &)> function);
#endif
    //! The copy constructor automatically handles reference counting.
    JsValue(const JsValue &value);
#ifndef GX_COPMATIBLE_CXX_98
    JsValue(JsValue &&value) noexcept;
#endif
    /**
     * Destructor, which dereference JavaScript value. However, note that GC may be delayed. If
     * JavaScript values are associated with C++ objects and need to deleted when the JavaScript
     * values are GCed, then this delay needs to be taken into account.
     * @note If delayed deletion of C++ objects may cause problems, then consider using the
     * moveObject() function to remove the C++ object pointer and delete it manually.
     */
    ~JsValue();

    /**
     * Get the type of the JavaScript value.
     * \see ValueType
     */
    GX_NODISCARD ValueType type() const;
    //! Check if the JavaScript value is **undefined**.
    GX_NODISCARD bool isUndefined() const;
    //! Checks if the JavaScript value is **null**.
    GX_NODISCARD bool isNull() const;
    //! Checks if the type of the JavaScript value is **boolean**.
    GX_NODISCARD bool isBoolean() const;
    //! Checks if the type of the JavaScript value is **number**.
    GX_NODISCARD bool isNumber() const;
    //! Checks if the type of the JavaScript value is **string**.
    GX_NODISCARD bool isString() const;
    /**
     * Checks if the type of the JavaScript value is **object**. Since **object** has many subtypes,
     * when checking a value is an object or some derived type, you need to check if it is a derived
     * type first, for example:
     * ```
     * JsValue value = someGetValue();
     * if (value.isArray()) // First check if the type is array
     *    someHandleArray(value);
     * else if (value.isObject()) // Then check if the type is object
     *    someHandleObject(value);
     * else
     *    handleValue();
     * ```
     * @note The JsValue C++ API does not provide all possible JavaScript object subtype detection
     * functions.
     * \see isArray() const
     * \see isProxy() const
     * \see isPromise() const
     * \see isArrayBuffer() const
     * \see isFunction() const
     */
    GX_NODISCARD bool isObject() const;
    //! Checks if the type of the JavaScript value is **array**.
    GX_NODISCARD bool isArray() const;
    //! Checks if the type of the JavaScript value is **proxy**.
    GX_NODISCARD bool isProxy() const;
    //! Checks if the type of the JavaScript value is **promise**.
    GX_NODISCARD bool isPromise() const;
    //! Checks if the type of the JavaScript value is **array buffer**.
    GX_NODISCARD bool isArrayBuffer() const;
    //! Checks if the type of the JavaScript value is **function**.
    GX_NODISCARD bool isFunction() const;
    //! Checks if the type of the JavaScript value is **exception**.
    GX_NODISCARD bool isException() const;
    //! Checks if the type of the JavaScript value is **simple object** (not a Array, Date, etc.).
    GX_NODISCARD bool isSimpleObject() const;
    /**
     * Checks if the type of the JavaScript value is **native object**.
     * @see object() const
     * @see moveObject() const
     */
    GX_NODISCARD bool isNativeObject() const;

    /**
     * Converts JavaScript value to numeric value. Due to the ECMAScript specification, if the value
     * not a numeric type then the conversion is done automatically. The conversion does not change
     * the original JavaScript values.
     * @return The double value after conversion.
     * @see toInt() const
     * @see toFloat() const
     */
    GX_NODISCARD double toNumber() const;
    /**
     * Similar to toNumber(), but return a int value.
     * @see toNumber() const
     * @see toFloat() const
     */
    GX_NODISCARD int toInt() const;
    /**
     * Similar to toNumber(), but return a float value.
     * @see toNumber() const
     * @see toInt() const
     */
    GX_NODISCARD float toFloat() const { return static_cast<float>(toNumber()); }
    /**
     * Converts JavaScript values to boolean. The conversion conforms to the ECMAScript standard and
     * does not change the original JavaScript values.
     * @return The converted bool value.
     */
    GX_NODISCARD bool toBoolean() const;
    /**
     * Converts JavaScript values to string. The conversion conforms to the ECMAScript standard and
     * does not change the original JavaScript values.
     * @return The converted string value. The string is encoded in
     * [CESU-8](https://en.wikipedia.org/wiki/CESU-8) (a variant of UTF-8).
     */
    GX_NODISCARD String toString() const;
    /**
     * Gets the value as a double numeric, or returns the default value specified by \p def if the
     * value is not a numeric type.
     */
    GX_NODISCARD double asNumber(double def = 0) const { return isNumber() ? toNumber() : def; }
    /**
     * Gets the value as a float numeric, or returns the default value specified by \p def if the
     * value is not a numeric type.
     */
    GX_NODISCARD float asFloat(float def = 0) const { return isNumber() ? toFloat() : def; }
    /**
     * Gets the value as a int numeric, or returns the default value specified by \p def if the
     * value is not a numeric type.
     */
    GX_NODISCARD int asInt(int def = 0) const { return isNumber() ? toInt() : def; }
    /**
     * Gets the value as a string, or returns the default value specified by \p def if the value is
     * not a string type.
     */
    GX_NODISCARD String asString(const String &def = String()) const {
        return isString() ? toString() : def;
    }
    /**
     * Get JavaScript exception values. The exception value can be obtained when isException() is
     * true. Printing exception values using LogInfo etc. will output relevant exception
     * information.
     */
    GX_NODISCARD JsValue exceptionValue() const;
    /**
     * Convert a JavaScript value to a JSON string. This is similar to JavaScript's
     * `JSON.stringify()`.
     * @return The JavaScript value of the JSON string, use toString() to get the String object.
     */
    GX_NODISCARD JsValue stringfyJSON() const;
    GX_NODISCARD ByteArray serializeJSON() const;
    /**
     * Get a deep copy of a JavaScript value. Object properties and Array items are copied
     * recursively, other values are not copied.
     * @warning This function is only applicable to JSON compatible values, and does not guarantee
     * the correctness of operations on value types such as functions.
     * @return The deep copied object.
     */
    GX_NODISCARD JsValue clone() const;
    GX_NODISCARD ByteArray serialize() const;

    //! Get the prototype of the JavaScript Object.
    GX_NODISCARD JsValue prototype() const;
    /**
     * Get the property value of the JavaScript Object.
     * @param name The name of the property. You can use String or const char * because of the
     * implicit conversion. But using JsValue directly is often faster because there is no
     * conversion.
     * @return The property value mapped by name.
     * @see operator[](const JsValue &) const
     * @see setPrototype()
     */
    GX_NODISCARD JsValue property(const JsValue &name) const;
    /**
     * Sets the prototype of the Object. Only valid for values of type Object.
     * @param value The prototype object, which must be an object.
     * @return Return true if the operation was successful.
     */
    bool setPrototype(const JsValue &value) const; // NOLINT(*-use-nodiscard)
    /**
     * Set the property value of the Javascript Object.
     * @param name The name of the property. You can use String or const char * because of the
     * implicit conversion. But using JsValue directly is often faster because there is no
     * conversion.
     * @param value The property value to be set.
     * @return Return true if the operation was successful.
     * @see property()
     */
    bool setProperty(const JsValue &name, const JsValue &value) const; // NOLINT(*-use-nodiscard)
    /**
     * Delete a property of the JavaScript Object.
     * @param name The name of the property. You can use String or const char * because of the
     * implicit conversion. But using JsValue directly is often faster because there is no
     * conversion.
     * @return Return true if the operation was successful.
     * @see property()
     */
    bool deleteProperty(const JsValue &name) const; // NOLINT(*-use-nodiscard)
    /**
     * Tests the specified property is contained by the JavaScript Object.
     * @see property()
     */
    GX_NODISCARD bool hasProperty(const JsValue &name) const;
    /**
     * Get the element value of the JavaScript Array by index.
     * @param index The index of the element.
     * @see operator[](int) const
     */
    GX_NODISCARD JsValue indexOf(int index) const;
    /**
     * Sets the value of the specified element of the JavaScript Array according to.
     * @param index The index of the element.
     * @param value The value to be set.
     */
    void setIndex(int index, const JsValue &value) const; // NOLINT(*-use-nodiscard)
    //! Get the length of JavaScript Array. Returns 0 if it is a non-Array value.
    GX_NODISCARD int arrayLength() const;
    //!  Get the target object of the JavaScript Proxy object.
    GX_NODISCARD JsValue proxyTarget() const;
    JsValue promiseResolve(const JsValue &argument) const; // NOLINT(*-use-nodiscard)
    JsValue promiseReject(const JsValue &argument) const;  // NOLINT(*-use-nodiscard)
    GX_NODISCARD JsValue promiseResult() const;
    /**
     * Get the promise state, or return PromiseStateNone on exception.
     * @see PromiseState
     */
    GX_NODISCARD PromiseState promiseState() const;

    GX_NODISCARD uint8_t *arrayBuffer() const;
    GX_NODISCARD std::size_t arrayBufferLength() const;

    /**
     * Get the C++ PrimitiveObject pointer associated with the JavaScript **object**. If there is no
     * associated PrimitiveObject object or the associated object has been deleted, a null pointer
     * is returned.
     * @see moveObject()
     * @see JsVM::newObject(PrimitiveObject *)
     */
    GX_NODISCARD PrimitiveObject *object() const;
    /**
     * Moves the C++ PrimitiveObject pointer associated with the JavaScript **object**. If there is
     * no associated PrimitiveObject object or the PrimitiveObject object has been deleted, a null
     * pointer is returned. The move operation will disassociate this JavaScript **object** from the
     * C++ PrimitiveObject object.
     * @note After disassociation, the GC of JavaScript **object** will not trigger the destruction
     * of automatically managed PrimitiveObject object.
     * @see object()
     * @see JsVM::newObject(PrimitiveObject *)
     */
    GX_NODISCARD PrimitiveObject *moveObject() const;

    /**
     * Call a JavaScript function and get the return value.
     * @param thisValue The value to which `this` is bound in the called JavaScript function. Bind
     * `this` to the global object if passed a JsValue() value.
     * @return The return value of the JavaScript function.
     * @see call(const JsValue &, const JsValue *, int) const
     * @see callMethod()
     */
    JsValue call(const JsValue &thisValue) const { return call(thisValue, nullptr, 0); }
    /**
     * Call a JavaScript function and get the return value.
     * @param thisValue The value to which `this` is bound in the called JavaScript function. Bind
     * `this` to the global object if passed a JsValue() value.
     * @param args The argument list when calling, the default is empty.
     * @return The return value of the JavaScript function.
     * @see call(const JsValue &, const JsValue *, int) const
     * @see callMethod()
     */
    JsValue call(const JsValue &thisValue, const JsValueList &args) const;
    /**
     * Call a JavaScript function and get the return value.
     * @param thisValue The value to which `this` is bound in the called JavaScript function. Bind
     * `this` to the global object if passed a JsValue() value.
     * @param args The argument list when calling, the default is empty.
     * @param argc The count of arguments.
     * @return The return value of the JavaScript function.
     * @note Since a C-style array is used for this function's argument list, it often has better
     * performance than @ref call(const JsValue &, const JsValueList &) const. This is because
     * C-style arrays are only constructed on the stack, while JsValueList needs to allocate
     * memory from the heap.
     * @see call(const JsValue &, const JsValueList &) const
     * @see callMethod()
     */
    JsValue call(const JsValue &thisValue, const JsValue *args, int argc) const;
    /**
     * Call a method of an JavaScript Object. It is similar to call(), but \p thisValue is bound to
     * the object.
     * @param name The name of the method.
     * @return The return value of the method.
     * @see property() const
     * @see call() const
     * @see callMethod(const JsValue &, const JsValue *, int) const
     */
    JsValue callMethod(const JsValue &name) const { return callMethod(name, nullptr, 0); }
    /**
     * Call a method of an JavaScript Object. It is similar to call(), but \p thisValue is bound to
     * the object.
     * @param name The name of the method.
     * @param args The argument list when calling, the default is empty.
     * @return The return value of the method.
     * @see property() const
     * @see call() const
     * @see callMethod(const JsValue &, const JsValue *, int) const
     */
    JsValue callMethod(const JsValue &name, const JsValueList &args) const;
    /**
     * Call a method of an JavaScript Object. It is similar to call(), but \p thisValue is bound to
     * the object.
     * @param name The name of the method.
     * @param args The argument list when calling.
     * @param argc The count of arguments.
     * @return The return value of the method.
     * @see property() const
     * @see call() const
     * @see callMethod(const JsValue &, const JsValueList &) const
     */
    JsValue callMethod(const JsValue &name, const JsValue *args, int argc) const;
    JsValue construct(const JsValueList &args = JsValueList()) const; // NOLINT(*-use-nodiscard)
    JsValue construct(const JsValue *args, int argc) const;

    /**
     * Print a JavaScript value, which is equivalent to `LogInfo() << this`.
     */
    void print() const;
    /**
     * Print a JavaScript value using the specified Logger object.
     */
    void print(const Logger &logger) const;
    /**
     * Use LogError() to print and return false if JavaScript is an exception, otherwise just return
     * true.
     */
    bool reportError() const; // NOLINT(*-use-nodiscard)

    /**
     * Overload operator= to automatically handle reference counting.
     */
    JsValue &operator=(const JsValue &value);
#ifndef GX_COPMATIBLE_CXX_98
    JsValue &operator=(JsValue &&value) noexcept;
#endif

    // bracket operator for JavaScript property operations.
    /**
     * Overload the operator[], equivalent to property().
     * @see property()
     */
    JsValue operator[](const JsValue &name) const { return property(name); }
    /**
     * Overload the operator[], equivalent to indexOf().
     * @see indexOf()
     */
    JsValue operator[](int index) const { return indexOf(index); }
    operator JsonValue() const;

    // relational operator only for C++ algorithms
    bool operator==(const JsValue &value) const { return m_value == value.m_value; }
    bool operator!=(const JsValue &value) const { return !(*this == value); }
    bool operator<(const JsValue &value) const { return m_value < value.m_value; }
    bool operator>(const JsValue &value) const { return value < *this; }
    bool operator<=(const JsValue &value) const { return !(*this > value); }
    bool operator>=(const JsValue &value) const { return !(*this < value); }

    // reserved feature and should not be used
    /**
     * Gets the JavaScript interpreter native value stored by the JsValue instance. This function
     * does not change the reference count.
     * @return Native value for the JavaScript interpreter, with data structure determined by the
     * specific JavaScript interpreter.
     * @warning This API allows you to use the native API of the JavaScript interpreter. Avoid using
     * this function: it will make the code less portable to JavaScript interpreters, and you may
     * face tedious reference counting details.
     * @warning It is important to ensure that the reference count of the returned native value is
     * balanced without additional increase or decrease, otherwise it will lead to memory leaks or
     * errors.
     * @see native_value
     * @see fromNative()
     */
    GX_NODISCARD const native_value &toNative() const { return m_value; }
    /**
     * Converts the JavaScript interpreter's native value to JsValue. This function does not change
     * the reference count of the value. This means that when the native value is stored in the
     * JsValue it will continue to be referenced until the JsValue is destructured. Therefore do not
     * additionally reduce the reference to the native value.
     * @param value Native value for the JavaScript interpreter, with data structure determined by
     * the specific JavaScript interpreter.
     * @return JsValue instances that contain native values.
     * @note This function is generally used in conjunction with toNative() (and not recommended).
     * @see native_value
     * @see toNative()
     */
    GX_NODISCARD static JsValue fromNative(const native_value &value) {
        return JsValue(value, Move());
    }

private:
    struct Move {};
    JsValue(native_value value, Move) : m_value(value) {}

    native_value m_value;
    class Private;
    friend class JsVM;
    friend class JsWeakRef;
    friend class JsCallContext;
    friend class JsPropertyIterator;
};

/**
 * The value of JsWeakRef is borrowed from other JsValue, so it does not change the reference count
 * of the JavaScript value. JsWeakRef is used to eliminate circular references. The most typical
 * case is when a native C++ object exported to JavaScript needs to access its bound JsValue, in
 * which case should use JsWeakRef field in C++ object to avoid circular references.
 * @warning JsWeakRef is only valid when the referenced JsValue is not destructured, otherwise it
 * will result in a dangling reference.
 */
class JsWeakRef {
public:
    //! Native JavaScript value type, defined from JsValue::native_value.
    typedef JsValue::native_value native_value; // NOLINT(readability-*)

    //! Default constructor. Constructs an empty reference, but the empty reference is also
    //! accessible.
    JsWeakRef() : m_value() {}
    //! Constructor to create a reference from the specified JsValue.
    //! @param value The referenced JsValue object.
    explicit JsWeakRef(const JsValue &value) : m_value(value.m_value) {}

    //! @return Returns the referenced JsValue.
    JsValue &operator*() { return *operator->(); }
    //! @return Returns the referenced const JsValue.
    const JsValue &operator*() const { return *operator->(); }
    //! @return Returns the referenced const JsValue pointer.
    const JsValue *operator->() const { return reinterpret_cast<const JsValue *>(&m_value); }
    //! @return Returns the referenced JsValue pointer.
    JsValue *operator->() { return reinterpret_cast<JsValue *>(&m_value); }

private:
    native_value m_value;
};

class JsVM : public NonCopyable<JsVM> {
public:
    enum BuiltinString {
        Builtin__,     // NOLINT(readability-*, *-reserved-identifier)
        Builtin_$emit, // NOLINT(readability-*)
        Builtin_close,
        Builtin_components,
        Builtin_computed,
        Builtin_data,
        Builtin_get,
        Builtin_onDestroy,
        Builtin_onInit,
        Builtin_onReady,
        Builtin_push,
        Builtin_set,
        Builtin_text,
        Builtin_watch,
        Builtin_END_OF_ENUMS // NOLINT(readability-*)
    };
    enum BacktraceLevel { AllLevel, SingleLevel };
    struct HeapStats {
        std::size_t size;          // heap size bytes
        std::size_t allocated;     // allocated bytes
        std::size_t peakAllocated; // peak allocated bytes
    };
    class Realm {
    public:
        Realm() : m_realm() {}
        Realm(const Realm &other);
        ~Realm();

        GX_NODISCARD JsValue globalObject() const;
        Realm &operator=(const Realm &rhs);

    private:
#ifdef GX_ENABLE_QUICKJS
        typedef void *realm_t; // NOLINT(readability-*)
#elif defined(GX_ENABLE_JERRYSCRIPT)
        typedef JsValue::native_value realm_t; // NOLINT(readability-*)
#endif
        explicit Realm(realm_t realm) : m_realm(realm) {}
        realm_t m_realm;
        friend class JsVM;
        friend class JsValue;
    };
    class RealmGuard : public NonCopyable<RealmGuard> {
    public:
        RealmGuard(JsVM &vm, const Realm &realm);
        ~RealmGuard();

    private:
        JsVM &m_vm;
        Realm::realm_t m_back;
    };
    class Backtrace {
    public:
        Backtrace() : m_vm() {}
        explicit Backtrace(JsVM &vm) : m_vm(&vm) {}
        GX_NODISCARD JsVM &vm() const { return m_vm ? *m_vm : current(); }

    private:
        JsVM *m_vm;
    };

    JsVM();
    ~JsVM();

    //! Constructing a **null** JavaScript value.
    GX_NODISCARD JsValue newNull();
    //! Creates a JavaScript **object** that does not contain any properties.
    GX_NODISCARD JsValue newObject();
    /**
     * Creates a JavaScript **object** and references a C++ PrimitiveObject object. The memory
     * management of the lats determined by the value of PrimitiveObject::objectKinds().
     * If the value contains the PrimitiveObject::RootKind flag but not the
     * PrimitiveObject::ExplicitDeleteKind, then when the JavaScript **object** object is GC, the
     * C++ PrimitiveObject object will also be deleted.
     * @note Not including the RootKind flag means that the PrimitiveObject object is actually a
     * child node and should be managed by the parent. The ExplicitDeleteKind flag instead declares
     * that the PrimitiveObject object is managed by the C++ program itself. In these cases the JsVM
     * framework does not delete the PrimitiveObject object.
     * @see JsValue::object()
     * @see JsValue::moveObject()
     */
    GX_NODISCARD JsValue newObject(PrimitiveObject *object);
    GX_NODISCARD JsValue newArray(int size = 0);
    GX_NODISCARD JsValue newArrayBuffer(std::size_t size);
    GX_NODISCARD JsValue newProxy(const JsValue &target, const JsValue &handler);
    GX_NODISCARD JsValue newPromise();
    GX_NODISCARD JsValue newError(const char *message);
    GX_NODISCARD JsValue newError(const String &message) { return newError(message.c_str()); }
    GX_NODISCARD Realm newRealm();
    GX_NODISCARD Realm realm() const;
    GX_NODISCARD Realm globalRealm() const;
    GX_NODISCARD JsValue globalObject() const { return realm().globalObject(); }
    GX_NODISCARD const JsValue &builtin(BuiltinString index) const;
    void mountIntrinsicModules(const Realm &realm);

    JsValue eval(const String &source);
    GX_NODISCARD JsValue importModule(const String &name);
    GX_NODISCARD JsValue parse(const ByteArray &source, const String &sourceName = String());
    GX_NODISCARD JsValue parseJSON(const char *source, std::size_t size);
    GX_NODISCARD JsValue parseJSON(const String &source) {
        return parseJSON(source.c_str(), source.length());
    }
    GX_NODISCARD JsValue readJSON(const ByteArray &data);
    GX_NODISCARD JsValue readJSON(const uint8_t *first, const uint8_t *last);
    GX_NODISCARD JsValue readJSON(const uint8_t *data, std::size_t size) {
        return readJSON(data, data + size);
    }

    JsValue backtrace(BacktraceLevel level = AllLevel) const;
    void backtrace(const Logger &log) const;
    //! Get the filename of the current deepest script call.
    GX_NODISCARD String sourceName() const;
    void pushJob(TaskEvent *job);
#ifndef GX_COPMATIBLE_CXX_98
    void pushJob(std::function<void()> job);
#endif
    bool executePendingJobs();
    void garbageCollection(); // run gc
    GX_NODISCARD HeapStats heapStats() const;
    void setHeapLimit(std::size_t limit);
    void setStackLimit(std::size_t limit);
    void setCompilerMode(bool status);

#ifdef GX_COMPATIBLE_V4
    static bool isBytecode(const String &source);
    static bool isBytecode(const char *source, size_t length);
    JsValue parse(const char *source, int length, const String &source_name = String());
    JsValue parse(const String &source, const String &source_name = String());
    JsValue run(const JsValue &code);
#endif

    static JsVM &current() { return *globalVM; }
    static bool isSerialized(const ByteArray &array);
    GX_NODISCARD static String resolveModule(const String &base, const String &name, int join = 1);

private:
    bool executePendingJobs(int);
    void setup();
    void cleanup();

    Realm m_context;
    JsValue m_preset[Builtin_END_OF_ENUMS];
    std::deque<TaskEvent *> m_pendingJobs;

    static JsVM *globalVM;
    friend class JsValue;
};

class JsFunctor {
public:
    JsFunctor() GX_DEFAULT;
    virtual ~JsFunctor() GX_DEFAULT;
    virtual JsValue operator()(const JsCallContext &context) = 0;
};

class JsCallContext : public NonCopyable<JsCallContext> {
public:
    GX_NODISCARD const JsValue &thisObject() const { return m_thisObject; }
    /**
     * Get the specified actual arguments for the JavaScript function call.
     * @param index The index of the argument, the value range is [0, argc()).
     * @return The argument reference at index.
     */
    GX_NODISCARD const JsValue &arg(int index) const {
        GX_CRT_ASSERT(index >= 0 && index < argc());
        // Since JsValue and native_value have the same memory layout, wee can cast m_args[index] to
        // const JsValue & directly.
        return reinterpret_cast<const JsValue &>(m_args[index]);
    }
    /**
     * Get the actual argument count of the JavaScript function call.
     */
    GX_NODISCARD int argc() const { return m_argc; }
    GX_NODISCARD JsVM &vm() const { return m_vm; }

private:
    typedef JsValue::native_value native_value; // NOLINT(readability-*)
    JsCallContext(JsVM &vm, native_value thisObject, const native_value *args, int argc);
    JsVM &m_vm;
    JsValue m_thisObject;
    const native_value *m_args;
    int m_argc;
    friend class JsValue::Private;
};

class JsModule {
public:
    struct Data {
        const char *name;
        JsValue (*require)(JsVM &vm);
    };
    explicit JsModule(const Data *data) noexcept;
    GX_NODISCARD JsValue require(JsVM &vm);
    void install();
    GX_NODISCARD static JsModule *find(const String &name) { return find(name.c_str()); }
    GX_NODISCARD static JsModule *find(const char *name);
    static void printAll(const Logger &logger);

private:
    static void resetAll();
    const Data *const m_data;
    JsModule *const m_next;
    friend class JsVM;
};

/**
 * Declare a JavaScript native module. Declared modules need to be imported using
 * GX_JSVM_MODULE_IMPORT macros before they can be referenced in JavaScript.
 * @param name The variable name of the module, which must be a valid C++ name.
 * @param path JavaScript import path for the module (does not contain the "@" character), for
 * example, "system.router".
 * @param require The module's initialization function that returns a JsValue value and can be used
 * in JavaScript.
 * @see GX_JSVM_MODULE_IMPORT
 */
#define GX_JSVM_MODULE(name, path, require)                                                        \
    static const ::gx::JsModule::Data jsvm_module_##name##_data = {(path), (require)};             \
    ::gx::JsModule jsvm_module_##name(&jsvm_module_##name##_data);

/**
 * Import a JavaScript module declared by GX_JSVM_MODULE. Only imported native modules can be
 * use in JavaScript.
 * @param name The variable name of the module (the name argument of GX_JSVM_MODULE).
 * @see GX_JSVM_MODULE
 */
#define GX_JSVM_MODULE_IMPORT(name)                                                                \
    extern ::gx::JsModule jsvm_module_##name;                                                      \
    jsvm_module_##name.install();

class JsPropertyIterator {
public:
    explicit JsPropertyIterator(const JsValue &object);
    ~JsPropertyIterator();
    GX_NODISCARD bool hasNext() const { return m_index < m_count; }
    void next() { ++m_index; }
    GX_NODISCARD JsValue name() const;
    GX_NODISCARD JsValue value() const;

private:
    JsValue m_object;
    int m_index, m_count;
#ifdef GX_ENABLE_QUICKJS
    void *m_names;
#elif defined(GX_ENABLE_JERRYSCRIPT)
    JsValue::native_value *m_names;
    void *m_collection;
#endif
};

using JsCtx = const JsCallContext &;

/**
 * Converts the value to a JsValue type. Can be specialised to a custom implementation.
 * @tparam T The type to be converted.
 * @param x The value to be converted.
 * @return The converted JsValue.
 */
template<class T> JsValue js_cast(const T &x) { return JsValue(x); } // NOLINT(readability-*)

template<> JsValue js_cast<JsonValue>(const JsonValue &x);

template<> inline JsValue js_cast<ByteArray>(const ByteArray &x) {
    JsValue buffer = JsVM::current().newArrayBuffer(x.size());
    std::memcpy(buffer.arrayBuffer(), x.constData(), x.size());
    return buffer;
}

template<class T> JsValue js_cast(const Vector<T> &x) { // NOLINT(readability-*)
    JsVM &vm = JsVM::current();
    JsValue array = vm.newArray(int(x.size()));
    for (std::size_t i = 0; i < x.size(); ++i)
        array.setIndex(int(i), js_cast(x[i]));
    return array;
}

namespace utils {
template<> struct hash<JsValue> {
    std::size_t operator()(const JsValue &v);
};

#if !defined(GX_ENABLE_QUICKJS) || !(INTPTR_MAX >= INT64_MAX)
inline std::size_t hash<JsValue>::operator()(const JsValue &v) {
    return hash<std::size_t>()(std::size_t(v.toNative()));
}
#endif
} // namespace utils

namespace prof {
#ifdef GX_ENABLE_PROFILING
class RecursionGeneric;

class PromiseProfiler {
public:
    explicit PromiseProfiler(const char *tag, const String &msg = String());
    ~PromiseProfiler();
    JsValue trace(const JsValue &promise);

private:
    RecursionGeneric *m_prof;
};
#else
class PromiseProfiler {
public:
    explicit PromiseProfiler(const char *, const String & = String()) {}
    JsValue trace(const JsValue &promise) { return promise; }
};
#endif
} // namespace prof

const Logger &operator<<(const Logger &, const JsValue &);
const Logger &operator<<(const Logger &, const JsVM::HeapStats &);
const Logger &operator<<(const Logger &, const JsVM::Backtrace &);
} // namespace gx
