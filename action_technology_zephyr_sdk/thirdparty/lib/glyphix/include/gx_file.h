/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_string.h"
#include "platform/gx_os.h"

namespace gx {
class ByteArray;

/*!
 * @brief The File class provides resources to read and write to source files.
 */
class File : public NonCopyable<File> {
public:
    //! This enumeration specifies the mode in which the open() function opens the file. These
    //! enumeration values can be combined using the `|` operator.
    enum OpenFlags {
        //! No flags. The file is in this state when it has not been open.
        NoneFlag = 0,
        //! Open the file in text mode. In Windows, this flag converts '\\n' to '\\r\\n' when
        //! reading or writing.
        Text = 0x01,
        //! Fail if the file to be opened does not exist. This flag must be specified alongside
        //! \ref ReadOnly, \ref WriteOnly, or \ref ReadWrite. Using this flag with \ref ReadOnly
        //! alone is redundant, as \ref ReadOnly already fails when the file does not exist.
        ExistingOnly = 0x02,
        //! Open the file as read-only.
        ReadOnly = 0x04,
        //! Open the file as write-only. This flag alone implies \ref Truncate unless combined with
        //! the \ref ReadOnly or \ref Append flags.
        WriteOnly = 0x08,
        //! Open the file as read and write. This mode does not imply \ref Truncate file.
        ReadWrite = ReadOnly | WriteOnly,
        //! The file is opened in append mode (write mode) so that all data is written to the end of
        //! the file.
        Append = 0x10 | WriteOnly,
        //! Open the file (write mode) and clear the contents of the original file first. This mode
        //! is equivalent to \ref WriteOnly when used alone.
        Truncate = 0x20 | WriteOnly
    };
    GX_FLAGS(OpenFlags);
    //! The type used to indicate the file offset is actually an alias for os::off_t.
    typedef os::off_t off_t; // NOLINT(readability-*)

    //! The default constructor, which constructs an empty file object. The uri() property of this
    //! file object is empty.
    File();
    //! Constructs a file object. \p uri specifies the URI of this file object.
    //! @note Executing the constructor does not open the file, so you must call the open() member
    //! function to open the file before reading or writing it.
    explicit File(const String &uri);
    File(File &&other) GX_NOEXCEPT;
    ~File(); //!< Destructor, which will automatically close if the file is already open.

    //! Check if the current file object is already open.
    GX_NODISCARD bool isOpened() const { return m_flags != 0; }
    //! Check that the file is writable.
    //! @return This function returns true when the file is opened in write mode, otherwise it
    //! returns false.
    GX_NODISCARD bool isWritable() const { return m_flags & WriteOnly; }
    //! Check that the file is readable.
    //! @return This function returns true when the file is opened in read mode, otherwise it
    //! returns false.
    GX_NODISCARD bool isReadable() const { return m_flags & ReadOnly; }
    //! Get the URI of the file.
    //! @return URI of the file. If the file has been opened or checked for existence, this URI is
    //! the absolute URI of the file in the resource framework, not necessarily the URI specified
    //! when the file was opened.
    GX_NODISCARD const String &uri() const { return m_uri; }
    //! Set the URI of the current file object, this function is only valid if the file is not open.
    void setUri(const String &uri);
    /**
     * Opens the file with \p flags, returning true if successful, otherwise false.
     * @note In \ref WriteOnly or \ref ReadWrite mode, if the relevant file does not already exist,
     * this function will try to create a new file before opening it.
     */
    bool open(OpenFlags flags = ReadOnly);
    bool close();
    GX_NODISCARD off_t size();
    GX_NODISCARD bool exists();
    std::size_t write(const ByteArray &data);
    std::size_t write(const void *data, std::size_t count);
    std::size_t read(void *buffer, std::size_t count);
    GX_NODISCARD ByteArray read(std::size_t maxsize);
    GX_NODISCARD ByteArray read();
    GX_NODISCARD const void *rawptr();
    GX_NODISCARD off_t tell();
    off_t seek(off_t offset);
    off_t seek(off_t offset, int whence);
    void sync();
    void swap(File &file) GX_NOEXCEPT;
    File &operator=(File &&other) GX_NOEXCEPT;

    GX_NODISCARD static bool exists(const String &uri);
    GX_NODISCARD static ByteArray read(const String &uri);
    GX_NODISCARD static String locate(const String &origin);
    static bool copy(const String &srcUri, const String &dstUri);

private:
    uint8_t m_flags;
    class ResourceFile *m_file;
    String m_uri;
};

inline File &File::operator=(File &&other) GX_NOEXCEPT {
    swap(other);
    return *this;
}
} // namespace gx
