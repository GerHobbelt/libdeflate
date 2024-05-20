/*
 * arm/cpu_features.h - feature detection for ARM CPUs
 *
 * Copyright 2018 Eric Biggers
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef LIB_ARM_CPU_FEATURES_H
#define LIB_ARM_CPU_FEATURES_H

#include "../lib_common.h"

#define HAVE_DYNAMIC_ARM_CPU_FEATURES	0

#if defined(ARCH_ARM32) || defined(ARCH_ARM64)

#if !defined(FREESTANDING) && \
    (defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER)) && \
    (defined(__linux__) || \
     (defined(__APPLE__) && defined(ARCH_ARM64)) || \
     (defined(_WIN32) && defined(ARCH_ARM64)))
#  undef HAVE_DYNAMIC_ARM_CPU_FEATURES
#  define HAVE_DYNAMIC_ARM_CPU_FEATURES	1
#endif

#define ARM_CPU_FEATURE_NEON		(1 << 0)
#define ARM_CPU_FEATURE_PMULL		(1 << 1)
/*
 * PREFER_PMULL indicates that the CPU has very high pmull throughput, and so
 * the 12x wide pmull-based CRC-32 implementation is likely to be faster than an
 * implementation based on the crc32 instructions.
 */
#define ARM_CPU_FEATURE_PREFER_PMULL	(1 << 2)
#define ARM_CPU_FEATURE_CRC32		(1 << 3)
#define ARM_CPU_FEATURE_SHA3		(1 << 4)
#define ARM_CPU_FEATURE_DOTPROD		(1 << 5)

#define HAVE_NEON(features)	(HAVE_NEON_NATIVE    || ((features) & ARM_CPU_FEATURE_NEON))
#define HAVE_PMULL(features)	(HAVE_PMULL_NATIVE   || ((features) & ARM_CPU_FEATURE_PMULL))
#define HAVE_CRC32(features)	(HAVE_CRC32_NATIVE   || ((features) & ARM_CPU_FEATURE_CRC32))
#define HAVE_SHA3(features)	(HAVE_SHA3_NATIVE    || ((features) & ARM_CPU_FEATURE_SHA3))
#define HAVE_DOTPROD(features)	(HAVE_DOTPROD_NATIVE || ((features) & ARM_CPU_FEATURE_DOTPROD))

#if HAVE_DYNAMIC_ARM_CPU_FEATURES
#define ARM_CPU_FEATURES_KNOWN		(1U << 31)
extern volatile u32 libdeflate_arm_cpu_features;

void libdeflate_init_arm_cpu_features(void);

static inline u32 get_arm_cpu_features(void)
{
	if (libdeflate_arm_cpu_features == 0)
		libdeflate_init_arm_cpu_features();
	return libdeflate_arm_cpu_features;
}
#else /* HAVE_DYNAMIC_ARM_CPU_FEATURES */
static inline u32 get_arm_cpu_features(void) { return 0; }
#endif /* !HAVE_DYNAMIC_ARM_CPU_FEATURES */

/* NEON */
#if defined(__ARM_NEON) || defined(ARCH_ARM64)
#  define HAVE_NEON_NATIVE	1
#else
#  define HAVE_NEON_NATIVE	0
#endif
/*
 * With both gcc and clang, NEON intrinsics require that the main target has
 * NEON enabled already.  Exception: with gcc 6.1 and later (r230411 for arm32,
 * r226563 for arm64), hardware floating point support is sufficient.
 */
#if HAVE_NEON_NATIVE || \
	(HAVE_DYNAMIC_ARM_CPU_FEATURES && GCC_PREREQ(6, 1) && defined(__ARM_FP))
#  define HAVE_NEON_INTRIN	1
#else
#  define HAVE_NEON_INTRIN	0
#endif

/* PMULL */
#ifdef __ARM_FEATURE_CRYPTO
#  define HAVE_PMULL_NATIVE	1
#else
#  define HAVE_PMULL_NATIVE	0
#endif
#if defined(ARCH_ARM64) && \
	(HAVE_PMULL_NATIVE || \
	 (HAVE_DYNAMIC_ARM_CPU_FEATURES && \
	  (GCC_PREREQ(6, 1) || defined(__clang__) || defined(_MSC_VER))))
#  define HAVE_PMULL_INTRIN	CPU_IS_LITTLE_ENDIAN() /* untested on big endian */
   /* Work around MSVC's vmull_p64() taking poly64x1_t instead of poly64_t */
#  ifdef _MSC_VER
#    define compat_vmull_p64(a, b)  vmull_p64(vcreate_p64(a), vcreate_p64(b))
#  else
#    define compat_vmull_p64(a, b)  vmull_p64((a), (b))
#  endif
#else
#  define HAVE_PMULL_INTRIN	0
#endif
/*
 * Set USE_PMULL_TARGET_EVEN_IF_NATIVE if a workaround for a gcc bug that was
 * fixed by commit 11a113d501ff ("aarch64: Simplify feature definitions") in gcc
 * 13 is needed.  A minimal program that fails to build due to this bug when
 * compiled with -mcpu=emag, at least with gcc 10 through 12, is:
 *
 *    static inline __attribute__((always_inline,target("+crypto"))) void f() {}
 *    void g() { f(); }
 *
 * The error is:
 *
 *    error: inlining failed in call to ‘always_inline’ ‘f’: target specific option mismatch
 *
 * The workaround is to explicitly add the crypto target to the non-inline
 * function g(), even though this should not be required due to -mcpu=emag
 * enabling 'crypto' natively and causing __ARM_FEATURE_CRYPTO to be defined.
 */
#if HAVE_PMULL_NATIVE && defined(ARCH_ARM64) && \
		GCC_PREREQ(6, 1) && !GCC_PREREQ(13, 1)
#  define USE_PMULL_TARGET_EVEN_IF_NATIVE	1
#else
#  define USE_PMULL_TARGET_EVEN_IF_NATIVE	0
#endif

/* CRC32 */
#ifdef __ARM_FEATURE_CRC32
#  define HAVE_CRC32_NATIVE	1
#else
#  define HAVE_CRC32_NATIVE	0
#endif
#if defined(ARCH_ARM64) && (HAVE_CRC32_NATIVE || defined(__GNUC__) || \
			    defined(__clang__) || defined(_MSC_VER))
#  define HAVE_CRC32_INTRIN	1
#else
#  define HAVE_CRC32_INTRIN	0
#endif

/* SHA3 (needed for the eor3 instruction) */
#if defined(ARCH_ARM64) && !defined(_MSC_VER)
#  ifdef __ARM_FEATURE_SHA3
#    define HAVE_SHA3_NATIVE	1
#  else
#    define HAVE_SHA3_NATIVE	0
#  endif
#  define HAVE_SHA3_TARGET	(HAVE_DYNAMIC_ARM_CPU_FEATURES && \
				 (GCC_PREREQ(8, 1) /* r256478 */ || \
				  CLANG_PREREQ(7, 0, 10010463) /* r338010 */))
#  define HAVE_SHA3_INTRIN	(HAVE_NEON_INTRIN && \
				 (HAVE_SHA3_NATIVE || HAVE_SHA3_TARGET) && \
				 (GCC_PREREQ(9, 1) /* r268049 */ || \
				  CLANG_PREREQ(13, 0, 13160000)))
#else
#  define HAVE_SHA3_NATIVE	0
#  define HAVE_SHA3_TARGET	0
#  define HAVE_SHA3_INTRIN	0
#endif

/* dotprod */
#ifdef ARCH_ARM64
#  ifdef __ARM_FEATURE_DOTPROD
#    define HAVE_DOTPROD_NATIVE	1
#  else
#    define HAVE_DOTPROD_NATIVE	0
#  endif
#  if HAVE_DOTPROD_NATIVE || \
	(HAVE_DYNAMIC_ARM_CPU_FEATURES && \
	 (GCC_PREREQ(8, 1) || CLANG_PREREQ(7, 0, 10010000) || \
	  defined(_MSC_VER)))
#    define HAVE_DOTPROD_INTRIN	1
#  else
#    define HAVE_DOTPROD_INTRIN	0
#  endif
#else
#  define HAVE_DOTPROD_NATIVE	0
#  define HAVE_DOTPROD_INTRIN	0
#endif

/*
 * Work around bugs in arm_acle.h and arm_neon.h where sometimes intrinsics are
 * only defined when the corresponding __ARM_FEATURE_* macro is defined.  The
 * intrinsics actually work in target attribute functions too if they are
 * defined, though, so work around this by temporarily defining the
 * corresponding __ARM_FEATURE_* macros while including the headers.
 */
#if HAVE_CRC32_INTRIN && !HAVE_CRC32_NATIVE && defined(__clang__)
#  define __ARM_FEATURE_CRC32	1
#endif
#if HAVE_SHA3_INTRIN && !HAVE_SHA3_NATIVE && defined(__clang__)
#  define __ARM_FEATURE_SHA3	1
#endif
#if HAVE_DOTPROD_INTRIN && !HAVE_DOTPROD_NATIVE && defined(__clang__)
#  define __ARM_FEATURE_DOTPROD	1
#endif
#if HAVE_CRC32_INTRIN && !HAVE_CRC32_NATIVE && defined(__clang__)
#  include <arm_acle.h>
#  undef __ARM_FEATURE_CRC32
#endif
#if HAVE_SHA3_INTRIN && !HAVE_SHA3_NATIVE && defined(__clang__)
#  include <arm_neon.h>
#  undef __ARM_FEATURE_SHA3
#endif
#if HAVE_DOTPROD_INTRIN && !HAVE_DOTPROD_NATIVE && defined(__clang__)
#  include <arm_neon.h>
#  undef __ARM_FEATURE_DOTPROD
#endif

#endif /* ARCH_ARM32 || ARCH_ARM64 */

#endif /* LIB_ARM_CPU_FEATURES_H */
