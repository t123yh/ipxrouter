/*
Author: Jelle Geerts

Usage of the works is permitted provided that this instrument is
retained with the works, so that any entity that uses the works is
notified of this instrument.

DISCLAIMER: THE WORKS ARE WITHOUT WARRANTY.
*/

#ifndef COMPILER_SPECIFIC_H
#define COMPILER_SPECIFIC_H


# define MACRO_BEGIN do {
# define MACRO_END } while(0)

// # define ATTRIBUTE_UNUSED __attribute__((__unused__))

# if GCC_VERSION >= 3003
#  define ATTRIBUTE_NONNULL(i) __attribute__((__nonnull__(i)))
# else /* GCC_VERSION < 3003 */
// #  define ATTRIBUTE_NONNULL(i)
# endif /* GCC_VERSION < 3003 */

# if GCC_VERSION >= 4004
#  define ATTRIBUTE_SENTINEL(i) __attribute__((__sentinel__(i)))
# else /* GCC_VERSION < 4004 */
// #  define ATTRIBUTE_SENTINEL(i)
# endif /* GCC_VERSION < 4004 */

//# define ATTRIBUTE_PACKED __attribute__((__packed__))

//# define ATTRIBUTE_FORMAT(i,j,k) __attribute__((__format__(i,j,k))) ATTRIBUTE_NONNULL(j)
//# define ATTRIBUTE_FORMAT_PRINTF __printf__
//# define ATTRIBUTE_FORMAT_SCANF __scanf__

# define STDCALL __stdcall

#endif /* !defined(COMPILER_SPECIFIC_H) */
