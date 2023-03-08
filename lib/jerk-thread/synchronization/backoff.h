/* Copyright (C) 2023 Gleb Bezborodov - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the MIT license.
 *
 * You should have received a copy of the MIT license with
 * this file. If not, please write to: bezborodoff.gleb@gmail.com, or visit : https://github.com/glensand/jerk-thread
 */

#pragma once

#undef BACK_OFF
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#   include <immintrin.h>
#   define BACK_OFF _mm_pause()
#elif defined(__clang__)
#	include <xmmintrin.h>
#	define BACK_OFF _mm_pause()
#else
#   define BACK_OFF
#endif