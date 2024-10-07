Programming Language
====================

The kernel is written in the C programming language [c-language].
More precisely, the kernel is typically compiled with `gcc` [gcc]
under `-std=gnu11` [gcc-c-dialect-options]: the GNU dialect of ISO C11.
`clang` [clang] is also supported.

This dialect contains many extensions to the language [gnu-extensions],
and many of them are used within the kbd as a matter of course.

Attributes
---------------

One of the common extensions used throughout the kernel are
attributes [gcc-attribute-syntax]. Attributes allow to introduce
implementation-defined semantics to language entities (like variables,
functions or types) without having to make significant syntactic changes
to the language (e.g. adding a new keyword) [n2049].

In some cases, attributes are optional (i.e. a compiler not supporting them
should still produce proper code, even if it is slower or does not perform
as many compile-time checks/diagnostics).

Please refer to `src/libcommon/compiler_attributes.h` for more information.


[c-language] http://www.open-std.org/jtc1/sc22/wg14/www/standards
[gcc] https://gcc.gnu.org
[clang] https://clang.llvm.org
[gcc-c-dialect-options] https://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html
[gnu-extensions] https://gcc.gnu.org/onlinedocs/gcc/C-Extensions.html
[gcc-attribute-syntax] https://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html
[n2049] http://www.open-std.org/jtc1/sc22/wg14/www/docs/n2049.pdf
