/*
 * include/common/compiler.h
 * This files contains some compiler-specific settings.
 *
 * Copyright (C) 2000-2009 Willy Tarreau - w@1wt.eu
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, version 2.1
 * exclusively.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _COMMON_COMPILER_H
#define _COMMON_COMPILER_H


/*
 * Gcc before 3.0 needs [0] to declare a variable-size array
 */
#ifndef VAR_ARRAY
#if  __GNUC__  < 3
#define VAR_ARRAY	0
#else
#define VAR_ARRAY
#endif
#endif


/* Support passing function parameters in registers. For this, the
 * CONFIG_REGPARM macro has to be set to the maximal number of registers
 * allowed. Some functions have intentionally received a regparm lower than
 * their parameter count, it is in order to avoid register clobbering where
 * they are called.
 */
#ifndef REGPRM1
#if CONFIG_REGPARM >= 1 && __GNUC__ >= 3
#define REGPRM1	__attribute__((regparm(1)))
#else
#define REGPRM1
#endif
#endif

#ifndef REGPRM2
#if CONFIG_REGPARM >= 2 && __GNUC__ >= 3
#define REGPRM2	__attribute__((regparm(2)))
#else
#define REGPRM2 REGPRM1
#endif
#endif

#ifndef REGPRM3
#if CONFIG_REGPARM >= 3 && __GNUC__ >= 3
#define REGPRM3	__attribute__((regparm(3)))
#else
#define REGPRM3 REGPRM2
#endif
#endif


/* By default, gcc does not inline large chunks of code, but we want it to
 * respect our choices.
 */
#if !defined(forceinline)
#if __GNUC__ < 3
#define forceinline inline
#else
#define forceinline inline __attribute__((always_inline))
#endif
#endif

#ifndef __maybe_unused
/* silence the "unused" warnings without having to place painful #ifdefs.
 * For use with variables or functions.
 */
#define __maybe_unused __attribute__((unused))
#endif

/* This allows gcc to know that some locations are never reached, for example
 * after a longjmp() in the Lua code, hence that some errors caught by such
 * methods cannot propagate further. This is important with gcc versions 6 and
 * above which can more aggressively detect null dereferences. The builtin
 * below was introduced in gcc 4.5, and before it we didn't care.
 */
#if __GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#define my_unreachable() __builtin_unreachable()
#else
#define my_unreachable() do { } while (1)
#endif

/* This macro may be used to block constant propagation that lets the compiler
 * detect a possible NULL dereference on a variable resulting from an explicit
 * assignment in an impossible check. Sometimes a function is called which does
 * safety checks and returns NULL if safe conditions are not met. The place
 * where it's called cannot hit this condition and dereferencing the pointer
 * without first checking it will make the compiler emit a warning about a
 * "potential null pointer dereference" which is hard to work around. This
 * macro "washes" the pointer and prevents the compiler from emitting tests
 * branching to undefined instructions. It may only be used when the developer
 * is absolutely certain that the conditions are guaranteed and that the
 * pointer passed in argument cannot be NULL by design.
 */
#define ALREADY_CHECKED(p) do { asm("" : "=rm"(p) : "0"(p)); } while (0)

/* Implements a static event counter where it's used. This is typically made to
 * report some warnings only once, either during boot or at runtime. It only
 * returns true on the very first call, and zero later. It's thread-safe and
 * uses a single byte of memory per call place. It relies on the atomic xchg
 * defined in atomic.h which is also part of the common API.
 */
#define ONLY_ONCE() ({ static char __cnt; !_HA_ATOMIC_XCHG(&__cnt, 1); })

/*
 * Gcc >= 3 provides the ability for the programme to give hints to the
 * compiler about what branch of an if is most likely to be taken. This
 * helps the compiler produce the most compact critical paths, which is
 * generally better for the cache and to reduce the number of jumps.
 */
#if !defined(likely)
#if __GNUC__ < 3
#define __builtin_expect(x,y) (x)
#define likely(x) (x)
#define unlikely(x) (x)
#elif __GNUC__ < 4 || __GNUC__ >= 5
/* gcc 3.x and 5.x do the best job at this */
#define likely(x) (__builtin_expect((x) != 0, 1))
#define unlikely(x) (__builtin_expect((x) != 0, 0))
#else
/* GCC 4.x is stupid, it performs the comparison then compares it to 1,
 * so we cheat in a dirty way to prevent it from doing this. This will
 * only work with ints and booleans though.
 */
#define likely(x) (x)
#define unlikely(x) (__builtin_expect((unsigned long)(x), 0))
#endif
#endif

#ifndef __GNUC_PREREQ__
#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#define __GNUC_PREREQ__(ma, mi) \
        (__GNUC__ > (ma) || __GNUC__ == (ma) && __GNUC_MINOR__ >= (mi))
#else
#define __GNUC_PREREQ__(ma, mi) 0
#endif
#endif

#ifndef offsetof
#if __GNUC_PREREQ__(4, 1)
#define offsetof(type, field)  __builtin_offsetof(type, field)
#else
#define offsetof(type, field) \
        ((size_t)(uintptr_t)((const volatile void *)&((type *)0)->field))
#endif
#endif

/* Some architectures have a double-word CAS, sometimes even dual-8 bytes.
 * Some architectures support unaligned accesses, others are fine with them
 * but only for non-atomic operations. Also mention those supporting unaligned
 * accesses and being little endian, and those where unaligned accesses are
 * known to be fast (almost as fast as aligned ones).
 */
#if defined(__x86_64__)
#define HA_UNALIGNED
#define HA_UNALIGNED_LE
#define HA_UNALIGNED_LE64
#define HA_UNALIGNED_FAST
#define HA_UNALIGNED_ATOMIC
#define HA_HAVE_CAS_DW
#define HA_CAS_IS_8B
#elif defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__)
#define HA_UNALIGNED
#define HA_UNALIGNED_LE
#define HA_UNALIGNED_ATOMIC
#elif defined (__aarch64__) || defined(__ARM_ARCH_8A)
#define HA_UNALIGNED
#define HA_UNALIGNED_LE
#define HA_UNALIGNED_LE64
#define HA_UNALIGNED_FAST
#define HA_HAVE_CAS_DW
#define HA_CAS_IS_8B
#elif defined(__arm__) && (defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__))
#define HA_UNALIGNED
#define HA_UNALIGNED_LE
#define HA_UNALIGNED_FAST
#define HA_HAVE_CAS_DW
#endif


/* sets alignment for current field or variable */
#ifndef ALIGNED
#define ALIGNED(x) __attribute__((aligned(x)))
#endif

/* sets alignment only on architectures preventing unaligned atomic accesses */
#ifndef MAYBE_ALIGNED
#ifndef HA_UNALIGNED
#define MAYBE_ALIGNED(x)  ALIGNED(x)
#else
#define MAYBE_ALIGNED(x)
#endif
#endif

/* sets alignment only on architectures preventing unaligned atomic accesses */
#ifndef ATOMIC_ALIGNED
#ifndef HA_UNALIGNED_ATOMIC
#define ATOMIC_ALIGNED(x)  ALIGNED(x)
#else
#define ATOMIC_ALIGNED(x)
#endif
#endif

/* add a mandatory alignment for next fields in a structure */
#ifndef ALWAYS_ALIGN
#define ALWAYS_ALIGN(x)  union { } ALIGNED(x)
#endif

/* add an optional alignment for next fields in a structure, only for archs
 * which do not support unaligned accesses.
 */
#ifndef MAYBE_ALIGN
#ifndef HA_UNALIGNED
#define MAYBE_ALIGN(x)  union { } ALIGNED(x)
#else
#define MAYBE_ALIGN(x)
#endif
#endif

/* add an optional alignment for next fields in a structure, only for archs
 * which do not support unaligned accesses for atomic operations.
 */
#ifndef ATOMIC_ALIGN
#ifndef HA_UNALIGNED_ATOMIC
#define ATOMIC_ALIGN(x)  union { } ALIGNED(x)
#else
#define ATOMIC_ALIGN(x)
#endif
#endif

#endif /* _COMMON_COMPILER_H */
