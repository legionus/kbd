/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _KBD_COMMON_COMPILER_ATTRIBUTES_H_
#define _KBD_COMMON_COMPILER_ATTRIBUTES_H_

/*
 * Compilers that lack __has_attribute may object to
 *   #if defined __has_attribute && __has_attribute (...)
 * even though they do not need to evaluate the right-hand side of the &&.
 */
#ifndef __has_attribute
# define __has_attribute(x) 0
#endif

/*
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-format-function-attribute
 * clang: https://clang.llvm.org/docs/AttributeReference.html#format
 */
#define KBD_ATTR_PRINTF(a, b)			__attribute__((__format__(printf, a, b)))
#define KBD_ATTR_SCANF(a, b)			__attribute__((__format__(scanf, a, b)))

/*
 * Optional: only supported since gcc >= 8
 * Optional: not supported by clang
 * Optional: not supported by icc
 *
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html#index-nonstring-variable-attribute
 */
#if __has_attribute(__nonstring__)
# define KBD_ATTR_NONSTRING			__attribute__((__nonstring__))
#else
# define KBD_ATTR_NONSTRING
#endif

/*
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-nonnull-function-attribute
 */
#if __has_attribute(nonnull)
#define KBD_ATTR_NONNULL(...)			__attribute__((nonnull(__VA_ARGS__)))
#else
#define KBD_ATTR_NONNULL
#endif

/*
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-noreturn-function-attribute
 * clang: https://clang.llvm.org/docs/AttributeReference.html#noreturn
 * clang: https://clang.llvm.org/docs/AttributeReference.html#id1
 */
#define KBD_ATTR_NORETURN			__attribute__((__noreturn__))

/*
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-unused-function-attribute
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Type-Attributes.html#index-unused-type-attribute
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html#index-unused-variable-attribute
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Label-Attributes.html#index-unused-label-attribute
 * clang: https://clang.llvm.org/docs/AttributeReference.html#maybe-unused-unused
 */
#define KBD_ATTR_UNUSED				__attribute__((__unused__))

/*
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-used-function-attribute
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html#index-used-variable-attribute
 */
#define KBD_ATTR_USED				__attribute__((__used__))

/*
 *   gcc: https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#index-warn_005funused_005fresult-function-attribute
 * clang: https://clang.llvm.org/docs/AttributeReference.html#nodiscard-warn-unused-result
 */
#define KBD_ATTR_MUST_CHECK			__attribute__((__warn_unused_result__))

#endif /* _KBD_COMMON_COMPILER_ATTRIBUTES_H_ */
