#ifndef _CTYPE_H_
#define _CTYPE_H_


/*!
 * \brief Alphabet character or Digit character
 */
#define isalnum(_c)  (((((_c) | 0x20) - 'a') < 26U) || (((_c) - '0') < 10U))


/*!
 * \brief Alphabet character or not
 */
#define isalpha(_c)  \
    ((('a' <= (_c)) && ((_c) <= 'z')) ||  \
    (('A' <= (_c)) && ((_c) <= 'Z')))


/*!
 * \brief ASCII character or not
 */
#define isascii(_c)  (((_c) & ~0x7f) == 0)


/*!
 * \brief Convert to ASCII character
 */
#define toascii(_c)  ((_c) & 0x7f)


/*!
 * \brief Blank character or not (include tab character)
 */
#define isblank(_c)  (((_c) == ' ') || ((_c) == '\t') || ((_c) == '\v'))


/*!
 * \brief Control character or not
 */
#define iscntrl(_c)  (((_c) < 32) || ((_c) == 127))


/*!
 * \brief Decimal digit character or not
 */
#define isdigit(_c)  (('0' <= (_c)) && ((_c) <= '9'))


/*!
 * \brief Printable character except space or not
 */
#define isgraph(_c)  ('!' <= (_c) && (_c) <= '~')


/*!
 * \brief Lowercase character or not
 */
#define islower(_c)  (('a' <= (_c)) && ((_c) <= 'z'))


/*!
 * \brief Uppercase character or not
 */
#define isupper(_c)  (('A' <= (_c)) && ((_c) <= 'Z'))


/*!
 * \brief Convert to lowercase character
 */
#define tolower(_c)  (isupper(_c) ? (((_c) - 'A') + 'a') : (_c))


/*!
 * \brief Convert to uppercase character
 */
#define toupper(_c)  (islower(_c) ? (((_c) - 'a') + 'A') : (_c))


/*!
 * \brief Printable character or not
 */
#define isprint(_c)  (' ' <= (_c) && (_c) <= '~')


/*!
 * \brief Space character or not
 */
#define isspace(_c)  (((((unsigned int)(_c)) - '\t') < 5U) || ((_c) == ' '))


/*!
 * \brief Punctuation character or not
 */
#define ispunct(_c)  (isprint(_c) && !isalnum(_c) && !isspace(_c))


/*!
 * \brief Hexadecimal digit character or not
 */
#define isxdigit(_c)  \
    ((('0' <= (_c)) && ((_c) <= '9')) ||  \
    (('a' <= (_c)) && ((_c) <= 'f')) ||  \
    (('A' <= (_c)) && ((_c) <= 'F')))


/*!
 * \brief Convert to lowercase character (_c must be uppercase character)
 */
#define _tolower(_c)  ((_c) - 'A' + 'a')


/*!
 * \brief Convert to uppercase character (_c must be lowercase character)
 */
#define _toupper(_c)  ((_c) - 'a' + 'A')


#endif  /* _CTYPE_H_ */


