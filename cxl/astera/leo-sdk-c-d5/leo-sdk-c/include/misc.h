/*
 * Copyright 2022 Astera Labs, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may not
 * use this file except in compliance with the License. You may obtain a copy
 * of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file misc.h
 * @brief Header file for miscellaneous defines used in the SDK.
 */
#ifndef _MISC_H_
#define _MISC_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../include/astera_log.h"

#define MIN(a, b) (((a) <= (b)) ? (a) : (b))
#define MAX(a, b) (((a) >= (b)) ? (a) : (b))

#define SIZE_1K (1 << 10)
#define SIZE_4K (SIZE_1K << 2)
#define SIZE_8K (SIZE_1K << 3)
#define SIZE_16K (SIZE_1K << 4)
#define SIZE_32K (SIZE_1K << 5)
#define SIZE_64K (SIZE_1K << 6)

#define GET_U64_HIGH(x) ((x) >> 32)
#define GET_U64_LOW(x) ((x) & (0xFFFFFFFF))
#define GET_U32_HIGH(x) (((x) & (0xFFFF0000)) >> 16)
#define GET_U32_LOW(x) ((x) & (0xFFFF))
#define GET_U16_HIGH(x) (((x) & (0xFF00)) >> 8)
#define GET_U16_LOW(x) ((x) & (0xFF))
#define GET_U8_HIGH(x) (((x) & (0xF0)) >> 4)
#define GET_U8_LOW(x) ((x) & (0xF))

#define SWAP_U16(x) ((((x) << 8) & 0xFF00) | (((x) >> 8) & 0xFF))
#define SWAP_U32(x)                                                            \
  ((SWAP_U16((x & 0xFFFF)) << 16) | SWAP_U16(((x >> 16) & 0xFFFF)))
#define DWORD_OFFSET(x) ((x) >> 2)
#define DWORD_SIZE(x) ((x) >> 2)

static __inline__ uint32_t align_down(uint32_t val, uint32_t align) {
  return (val & (~(align - 1)));
}

static __inline__ uint32_t align_up(uint32_t val, uint32_t align) {
  return ((val + align) & (~(align - 1)));
}

// Each word is 32 bits.
#define BITMAP_WORD_ARRAY_WORD_SIZE_BITS_SHIFT (5)
// Each word is 4 bytes.
#define BITMAP_WORD_ARRAY_WORD_SIZE_BYTE_SHIFT (2)
// Each word is 8 bits.
#define BITMAP_WORD_ARRAY_CHAR_SIZE_BITS_SHIFT (3)

#define BITMAP_WORD_ARRAY_SIZE_WORDS(x)                                        \
  ((x + (1 << BITMAP_WORD_ARRAY_WORD_SIZE_BITS_SHIFT)) >>                      \
   (BITMAP_WORD_ARRAY_WORD_SIZE_BITS_SHIFT))
#define BITMAP_WORD_ARRAY_SIZE_BYTES(x)                                        \
  (((x + (1 << BITMAP_WORD_ARRAY_WORD_SIZE_BITS_SHIFT)) >>                     \
    (BITMAP_WORD_ARRAY_WORD_SIZE_BITS_SHIFT))                                  \
   << BITMAP_WORD_ARRAY_WORD_SIZE_BYTE_SHIFT)
#define BITMAP_CHAR_ARRAY_SIZE_BYTES(x)                                        \
  ((x + (1 << BITMAP_WORD_ARRAY_CHAR_SIZE_BITS_SHIFT)) >>                      \
   (BITMAP_WORD_ARRAY_CHAR_SIZE_BITS_SHIFT))

/**
 * @brief This function is used to check if a particular bit is set in a word
 * array bitmap.
 * @param p_bmp Pointer to the bitmap array.
 * @param bit_num Bit number to be checked.
 * @return true if the bit is set, false otherwise.
 *
 */
static __inline__ bool bitmap_word_array_is_bitset(uint32_t *p_bmp,
                                                   uint32_t bit_num) {
  uint32_t bmp_ar_idx = bit_num >> BITMAP_WORD_ARRAY_WORD_SIZE_BITS_SHIFT;
  uint32_t bmp_bit_idx =
      bit_num - (bmp_ar_idx << BITMAP_WORD_ARRAY_WORD_SIZE_BITS_SHIFT);

  return ((p_bmp[bmp_ar_idx] & (1 << bmp_bit_idx)) >> bmp_bit_idx);
}

/**
 * @brief This function is used to set a particular bit in a word array
 * bitmap.
 * @param p_bmp Pointer to the bitmap array.
 * @param bit_num Bit number to be set.
 * @return None.
 */
static __inline__ void bitmap_word_array_bit_set(uint32_t *p_bmp,
                                                 uint32_t bit_num) {
  uint32_t bmp_ar_idx = bit_num >> BITMAP_WORD_ARRAY_WORD_SIZE_BITS_SHIFT;
  uint32_t bmp_bit_idx =
      bit_num - (bmp_ar_idx << BITMAP_WORD_ARRAY_WORD_SIZE_BITS_SHIFT);

  p_bmp[bmp_ar_idx] |= (1 << bmp_bit_idx);
}

/**
 * @brief This function is used to clear a particular bit in a word array
 * bitmap.
 * @param p_bmp Pointer to the bitmap array.
 * @param bit_num Bit number to be cleared.
 * @return None.
 */
static __inline__ void bitmap_word_array_bit_clear(uint32_t *p_bmp,
                                                   uint32_t bit_num) {
  uint32_t bmp_ar_idx = bit_num >> BITMAP_WORD_ARRAY_WORD_SIZE_BITS_SHIFT;
  uint32_t bmp_bit_idx =
      bit_num - (bmp_ar_idx << BITMAP_WORD_ARRAY_WORD_SIZE_BITS_SHIFT);

  p_bmp[bmp_ar_idx] &= (~(1 << bmp_bit_idx));
}

#define RET_OK 0
#define RET_FAIL 1

/**
 * @brief Macro to supress unferenced function argument.
 */
#define UNUSED_FUNC_ARG(X) (void)X

// Debug printing for simulations
#define FW_NONE 0x0
#define FW_LOW 0x1
#define FW_MEDIUM 0x2
#define FW_HIGH 0x3

#ifndef FW_VERBOSITY
#define FW_VERBOSITY FW_NONE
#endif
#if FW_VERBOSITY == FW_HIGH
#define al_print_high(...) al_print(__VA_ARGS__)
#define al_print_medium(...) al_print(__VA_ARGS__)
#define al_print_low(...) al_print(__VA_ARGS__)
#elif FW_VERBOSITY == FW_MEDIUM
#define al_print_high(...)
#define al_print_medium(...) al_print(__VA_ARGS__)
#define al_print_low(...) al_print(__VA_ARGS__)
#elif FW_VERBOSITY == FW_LOW
#define al_print_high(...)
#define al_print_medium(...)
#define al_print_low(...) al_print(__VA_ARGS__)
#elif FW_VERBOSITY == FW_NONE
#define al_print_high(...)
#define al_print_medium(...)
#define al_print_low(...)
#endif

#define TRUE 1
#define FALSE 0

#define ENABLE 1
#define DISABLE 0

#define MSK_BIT0 0x01
#define MSK_BIT1 0x02
#define MSK_BIT2 0x04
#define MSK_BIT3 0x08
#define MSK_BIT4 0x10
#define MSK_BIT5 0x20
#define MSK_BIT6 0x40
#define MSK_BIT7 0x80

#define LANE0 0x0
#define LANE1 0x1
#define LANEX 0xF

typedef union qword {
  uint8_t byte[8];
  uint16_t word[4];
  uint32_t dw[2];
  uint64_t qw;
} qword_t;

/**
 * @brief Generic scheduler function prototype
 * @param[in] ctx - context to be passed to the scheduler
 */
typedef void (*p_generic_scheduler)(void *ctx);

#endif

#ifdef CHIP_D5
#include "d5/leo_top_csr_pub_d5.h"
#elif CHIP_A0
#include "a0/leo_top_csr_pub_a0.h"
#else
#error Unknown CHIP verison
#endif 
