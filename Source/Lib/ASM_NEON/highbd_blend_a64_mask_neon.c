/*
 * Copyright (c) 2023, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <arm_neon.h>
#include <assert.h>

#include "blend_a64_mask_neon.h"
#include "common_dsp_rtcd.h"
#include "definitions.h"
#include "mem_neon.h"

static INLINE uint16x8_t alpha_blend_a64_u16x8(uint16x8_t m, uint16x8_t a, uint16x8_t b) {
    uint16x8_t m_inv = vsubq_u16(vdupq_n_u16(AOM_BLEND_A64_MAX_ALPHA), m);

    uint32x4_t blend_u32_lo = vmull_u16(vget_low_u16(a), vget_low_u16(m));
    uint32x4_t blend_u32_hi = vmull_u16(vget_high_u16(a), vget_high_u16(m));

    blend_u32_lo = vmlal_u16(blend_u32_lo, vget_low_u16(b), vget_low_u16(m_inv));
    blend_u32_hi = vmlal_u16(blend_u32_hi, vget_high_u16(b), vget_high_u16(m_inv));

    uint16x4_t blend_u16_lo = vrshrn_n_u32(blend_u32_lo, AOM_BLEND_A64_ROUND_BITS);
    uint16x4_t blend_u16_hi = vrshrn_n_u32(blend_u32_hi, AOM_BLEND_A64_ROUND_BITS);

    return vcombine_u16(blend_u16_lo, blend_u16_hi);
}

void svt_aom_highbd_blend_a64_mask_neon(uint8_t *dst_8, uint32_t dst_stride, const uint8_t *src0_8,
                                        uint32_t src0_stride, const uint8_t *src1_8, uint32_t src1_stride,
                                        const uint8_t *mask, uint32_t mask_stride, int w, int h, int subw, int subh,
                                        int bd) {
    (void)bd;

    const uint16_t *src0 = (uint16_t *)src0_8;
    const uint16_t *src1 = (uint16_t *)src1_8;
    uint16_t       *dst  = (uint16_t *)dst_8;

    assert(IMPLIES(src0 == dst, src0_stride == dst_stride));
    assert(IMPLIES(src1 == dst, src1_stride == dst_stride));

    assert(h >= 1);
    assert(w >= 1);
    assert(IS_POWER_OF_TWO(h));
    assert(IS_POWER_OF_TWO(w));

    assert(bd == 8 || bd == 10);

    if ((subw | subh) == 0) {
        if (w >= 8) {
            do {
                int i = 0;
                do {
                    uint16x8_t m0 = vmovl_u8(vld1_u8(mask + i));
                    uint16x8_t s0 = vld1q_u16(src0 + i);
                    uint16x8_t s1 = vld1q_u16(src1 + i);

                    uint16x8_t blend = alpha_blend_a64_u16x8(m0, s0, s1);

                    vst1q_u16(dst + i, blend);
                    i += 8;
                } while (i < w);

                mask += mask_stride;
                src0 += src0_stride;
                src1 += src1_stride;
                dst += dst_stride;
            } while (--h != 0);
        } else {
            do {
                uint16x8_t m0 = vmovl_u8(load_unaligned_u8_4x2(mask, mask_stride));
                uint16x8_t s0 = load_unaligned_u16_4x2(src0, src0_stride);
                uint16x8_t s1 = load_unaligned_u16_4x2(src1, src1_stride);

                uint16x8_t blend = alpha_blend_a64_u16x8(m0, s0, s1);

                store_u16x4_strided_x2(dst, dst_stride, blend);

                mask += 2 * mask_stride;
                src0 += 2 * src0_stride;
                src1 += 2 * src1_stride;
                dst += 2 * dst_stride;
                h -= 2;
            } while (h != 0);
        }
    } else if ((subw & subh) == 1) {
        if (w >= 8) {
            do {
                int i = 0;
                do {
                    uint8x8_t  m0 = vld1_u8(mask + 0 * mask_stride + 2 * i);
                    uint8x8_t  m1 = vld1_u8(mask + 1 * mask_stride + 2 * i);
                    uint8x8_t  m2 = vld1_u8(mask + 0 * mask_stride + 2 * i + 8);
                    uint8x8_t  m3 = vld1_u8(mask + 1 * mask_stride + 2 * i + 8);
                    uint16x8_t s0 = vld1q_u16(src0 + i);
                    uint16x8_t s1 = vld1q_u16(src1 + i);

                    uint16x8_t m_avg = vmovl_u8(avg_blend_pairwise_u8x8_4(m0, m1, m2, m3));

                    uint16x8_t blend = alpha_blend_a64_u16x8(m_avg, s0, s1);

                    vst1q_u16(dst + i, blend);

                    i += 8;
                } while (i < w);

                mask += 2 * mask_stride;
                src0 += src0_stride;
                src1 += src1_stride;
                dst += dst_stride;
            } while (--h != 0);
        } else {
            do {
                uint8x8_t  m0 = vld1_u8(mask + 0 * mask_stride);
                uint8x8_t  m1 = vld1_u8(mask + 1 * mask_stride);
                uint8x8_t  m2 = vld1_u8(mask + 2 * mask_stride);
                uint8x8_t  m3 = vld1_u8(mask + 3 * mask_stride);
                uint16x8_t s0 = load_unaligned_u16_4x2(src0, src0_stride);
                uint16x8_t s1 = load_unaligned_u16_4x2(src1, src1_stride);

                uint16x8_t m_avg = vmovl_u8(avg_blend_pairwise_u8x8_4(m0, m1, m2, m3));
                uint16x8_t blend = alpha_blend_a64_u16x8(m_avg, s0, s1);

                store_u16x4_strided_x2(dst, dst_stride, blend);

                mask += 4 * mask_stride;
                src0 += 2 * src0_stride;
                src1 += 2 * src1_stride;
                dst += 2 * dst_stride;
                h -= 2;
            } while (h != 0);
        }
    } else if (subw == 1 && subh == 0) {
        if (w >= 8) {
            do {
                int i = 0;

                do {
                    uint8x8_t  m0 = vld1_u8(mask + 2 * i);
                    uint8x8_t  m1 = vld1_u8(mask + 2 * i + 8);
                    uint16x8_t s0 = vld1q_u16(src0 + i);
                    uint16x8_t s1 = vld1q_u16(src1 + i);

                    uint16x8_t m_avg = vmovl_u8(avg_blend_pairwise_u8x8(m0, m1));
                    uint16x8_t blend = alpha_blend_a64_u16x8(m_avg, s0, s1);

                    vst1q_u16(dst + i, blend);

                    i += 8;
                } while (i < w);

                mask += mask_stride;
                src0 += src0_stride;
                src1 += src1_stride;
                dst += dst_stride;
            } while (--h != 0);
        } else {
            do {
                uint8x8_t  m0 = vld1_u8(mask + 0 * mask_stride);
                uint8x8_t  m1 = vld1_u8(mask + 1 * mask_stride);
                uint16x8_t s0 = load_unaligned_u16_4x2(src0, src0_stride);
                uint16x8_t s1 = load_unaligned_u16_4x2(src1, src1_stride);

                uint16x8_t m_avg = vmovl_u8(avg_blend_pairwise_u8x8(m0, m1));
                uint16x8_t blend = alpha_blend_a64_u16x8(m_avg, s0, s1);

                store_u16x4_strided_x2(dst, dst_stride, blend);

                mask += 2 * mask_stride;
                src0 += 2 * src0_stride;
                src1 += 2 * src1_stride;
                dst += 2 * dst_stride;
                h -= 2;
            } while (h != 0);
        }
    } else {
        if (w >= 8) {
            do {
                int i = 0;
                do {
                    uint8x8_t  m0 = vld1_u8(mask + 0 * mask_stride + i);
                    uint8x8_t  m1 = vld1_u8(mask + 1 * mask_stride + i);
                    uint16x8_t s0 = vld1q_u16(src0 + i);
                    uint16x8_t s1 = vld1q_u16(src1 + i);

                    uint16x8_t m_avg = vmovl_u8(avg_blend_u8x8(m0, m1));
                    uint16x8_t blend = alpha_blend_a64_u16x8(m_avg, s0, s1);

                    vst1q_u16(dst + i, blend);

                    i += 8;
                } while (i < w);

                mask += 2 * mask_stride;
                src0 += src0_stride;
                src1 += src1_stride;
                dst += dst_stride;
            } while (--h != 0);
        } else {
            do {
                uint8x8_t  m0_2 = load_unaligned_u8_4x2(mask + 0 * mask_stride, 2 * mask_stride);
                uint8x8_t  m1_3 = load_unaligned_u8_4x2(mask + 1 * mask_stride, 2 * mask_stride);
                uint16x8_t s0   = load_unaligned_u16_4x2(src0, src0_stride);
                uint16x8_t s1   = load_unaligned_u16_4x2(src1, src1_stride);

                uint16x8_t m_avg = vmovl_u8(avg_blend_u8x8(m0_2, m1_3));
                uint16x8_t blend = alpha_blend_a64_u16x8(m_avg, s0, s1);

                store_u16x4_strided_x2(dst, dst_stride, blend);

                mask += 4 * mask_stride;
                src0 += 2 * src0_stride;
                src1 += 2 * src1_stride;
                dst += 2 * dst_stride;
                h -= 2;
            } while (h != 0);
        }
    }
}
