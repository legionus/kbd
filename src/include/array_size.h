// SPDX-License-Identifier: LGPL-2.0-or-later
#ifndef _KBD_ARRAY_SIZE_H_
#define _KBD_ARRAY_SIZE_H_

/**
 * FAIL_BUILD_ON_ZERO - causes a syntax error if the @expr is 0.
 *
 * This macro was thankfully taken from strace.
 */
#define FAIL_BUILD_ON_ZERO(ex)	(sizeof(int[-1 + 2 * !!(ex)]) * 0)

/**
 * IS_SAME_TYPE - check are two types/vars the same type.
 *
 * Returns: 1 if the given two types are known to be the same or 0 otherwise.
 */
#define IS_SAME_TYPE(a, b)	__builtin_types_compatible_p(__typeof__(a), __typeof__(b))

/**
 * MUST_BE_ARRAY - causes a syntax error if the @arr is not array.
 */
#define MUST_BE_ARRAY(arr)	FAIL_BUILD_ON_ZERO(!IS_SAME_TYPE((arr), &(arr)[0]))

/**
 * ARRAY_SIZE - get the number of elements in array @arr
 * @arr: array to be sized.
 */
#define ARRAY_SIZE(arr)		(sizeof(arr) / sizeof((arr)[0]) + MUST_BE_ARRAY(arr))

#endif  /* _KBD_ARRAY_SIZE_H_ */
