/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_string.h"

namespace gx {
class Uri {
public:
    /**
     * @brief Specifies the base URI preprocessing method for joining.
     * @see join()
     */
    enum JoinMode {
        ConnectPath = 0, //!< Direct join mode.
        SplitDirectory   //!< Split the path and remove the file name before splicing.
    };
    template<JoinMode> class Join;

    //! The URI query string parser.
    class QueryParser {
    public:
        //! Construct a query parser. Expected a query string, like "key1=value1&key2=value2".
        explicit QueryParser(const String &query);
        //! Parse the next query key-value pair. Return false if there is no more key-value pair.
        bool next();
        //! Get the current parsed query key. Must be called after next() returns true.
        const String &key() const { return m_pair.first; }
        //! Get the current parsed query value. Must be called after next() returns true.
        const String &value() const { return m_pair.second; }
        //! Get the current parsed query key-value pair. Must be called after next() returns true.
        const std::pair<String, String> &operator*() const { return m_pair; }
        //! Get the current parsed query key-value pair pointer. Must be called after next() returns
        //! true.
        const std::pair<String, String> *operator->() const { return &m_pair; }

    private:
        String m_query;
        String::const_iterator m_iterator;
        std::pair<String, String> m_pair;
    };

    Uri();
    explicit Uri(const String &uri);
    ~Uri() GX_DEFAULT;

    bool isPath() const { return m_scheme.empty(); }
    const String &uri() const;
    const String &scheme() const { return m_scheme; }
    const String &authority() const { return m_authority; }
    const String &path() const { return m_path; }
    const String &query() const { return m_query; }
    QueryParser parseQuery() const { return QueryParser(m_query); }
    void setUri(const String &uri);
    void setScheme(const String &scheme);
    void setAuthority(const String &authority);
    void setPath(const String &path);
    void setQuery(const String &query);

    static bool isPath(const String &uri);
    static String splitBasic(const String &uri);
    static String splitDir(const String &uri);
    /**
     * @brief Join two URI strings \p base and \p uri.
     * The joining rules.
     *   - Returns the \p uri directly if it is a full URI.
     *   - If \p base is a full URI and \p uri is a path, return the URI string by combined the path
     *     field of \p base with \p uri.
     *   - If \p base and \p uri are both paths, return the result of combining \p base and \p uri
     *     for paths. The path joining method is identical to Dir::join().
     *
     * The \p mode specifies how the path is joined:
     *   - ConnectPath (default): Join the path field of \p base with \p uri directly, in which case
     *     `/a/b` and `c` are joined as `/a/b/c`.
     *   - SplitDirectory: First, split the path field of \p base into directory path and file name,
     *     then join the directory path and \p uri. For example, the result of `/a/b` and `c` will
     *     be `/a/c`.
     * @param base The base URI or path used for joining.
     * @param uri The URI or path that will be joined.
     * @param mode Specifies the joining strategy for the path.
     * @return The joined URI.
     *
     * Examples:
     * @code
     * Uri::join("path", "file:///icon.png"); // file:///icon.png
     * Uri::join("file://clock.app/assets", "icon.png"); // file://clock.app/assets/icon.png
     * Uri::join("pkg://clock/dir/a.js", "b.js", Uri::SplitDirectory); // pkg://clock/dir/b.js
     * @endcode
     */
    static String join(const String &base, const String &uri, JoinMode mode = ConnectPath);
    static String fieldEncode(const String &src);
    static String fieldEncodeStrict(const String &src);
    static String fieldDecode(const String &src) { return fieldDecode(src.begin(), src.end()); }
    static String fieldDecode(const char *first, const char *last);

private:
    mutable String m_uri;
    String m_scheme, m_authority, m_path, m_query;
    mutable bool m_updated;
};

template<Uri::JoinMode mode = Uri::ConnectPath> class Uri::Join {
public:
#ifndef GX_COPMATIBLE_CXX_98
    explicit Join(String base = String()) : m_base(std::move(base)) {}
#else
    explicit Join(const String &base = String()) : m_base(base) {}
#endif
    String operator()(const String &uri) const { return Uri::join(m_base, uri, mode); }

private:
    String m_base;
};
} // namespace gx
