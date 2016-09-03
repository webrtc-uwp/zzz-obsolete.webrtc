/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/audio_processing/aecm/aecm_core.h"

#include <arm_neon.h>
#include <assert.h>

#include "webrtc/common_audio/signal_processing/include/real_fft.h"

#if defined(_MSC_VER)
  #define INLINE_KEYWORD __inline
#else
  #define INLINE_KEYWORD inline
#endif

// TODO(kma): Re-write the corresponding assembly file, the offset
// generating script and makefile, to replace these C functions.

// Square root of Hanning window in Q14.
const ALIGN8_BEG int16_t WebRtcAecm_kSqrtHanning[] ALIGN8_END = {
  0,
  399, 798, 1196, 1594, 1990, 2386, 2780, 3172,
  3562, 3951, 4337, 4720, 5101, 5478, 5853, 6224,
  6591, 6954, 7313, 7668, 8019, 8364, 8705, 9040,
  9370, 9695, 10013, 10326, 10633, 10933, 11227, 11514,
  11795, 12068, 12335, 12594, 12845, 13089, 13325, 13553,
  13773, 13985, 14189, 14384, 14571, 14749, 14918, 15079,
  15231, 15373, 15506, 15631, 15746, 15851, 15947, 16034,
  16111, 16179, 16237, 16286, 16325, 16354, 16373, 16384
};

static INLINE_KEYWORD void AddLanes(uint32_t* ptr, uint32x4_t v) {
#if defined(WEBRTC_ARCH_ARM64)
  *(ptr) = vaddvq_u32(v);
#else
  uint32x2_t tmp_v;
  tmp_v = vadd_u32(vget_low_u32(v), vget_high_u32(v));
  tmp_v = vpadd_u32(tmp_v, tmp_v);
  *(ptr) = vget_lane_u32(tmp_v, 0);
#endif
}

void WebRtcAecm_CalcLinearEnergiesNeon(AecmCore* aecm,
                                       const uint16_t* far_spectrum,
                                       int32_t* echo_est,
                                       uint32_t* far_energy,
                                       uint32_t* echo_energy_adapt,
                                       uint32_t* echo_energy_stored) {
  int16_t* start_stored_p = aecm->channelStored;
  int16_t* start_adapt_p = aecm->channelAdapt16;
  int32_t* echo_est_p = echo_est;
  const int16_t* end_stored_p = aecm->channelStored + PART_LEN;
  const uint16_t* far_spectrum_p = far_spectrum;
  int16x8_t store_v, adapt_v;
  uint16x8_t spectrum_v;
  uint32x4_t echo_est_v_low, echo_est_v_high;
  uint32x4_t far_energy_v, echo_stored_v, echo_adapt_v;

  far_energy_v = vdupq_n_u32(0);
  echo_adapt_v = vdupq_n_u32(0);
  echo_stored_v = vdupq_n_u32(0);

  // Get energy for the delayed far end signal and estimated
  // echo using both stored and adapted channels.
  // The C code:
  //  for (i = 0; i < PART_LEN1; i++) {
  //      echo_est[i] = WEBRTC_SPL_MUL_16_U16(aecm->channelStored[i],
  //                                         far_spectrum[i]);
  //      (*far_energy) += (uint32_t)(far_spectrum[i]);
  //      *echo_energy_adapt += aecm->channelAdapt16[i] * far_spectrum[i];
  //      (*echo_energy_stored) += (uint32_t)echo_est[i];
  //  }
  while (start_stored_p < end_stored_p) {
    spectrum_v = vld1q_u16(far_spectrum_p);
    adapt_v = vld1q_s16(start_adapt_p);
    store_v = vld1q_s16(start_stored_p);

    far_energy_v = vaddw_u16(far_energy_v, vget_low_u16(spectrum_v));
    far_energy_v = vaddw_u16(far_energy_v, vget_high_u16(spectrum_v));

    echo_est_v_low = vmull_u16(vreinterpret_u16_s16(vget_low_s16(store_v)),
                               vget_low_u16(spectrum_v));
    echo_est_v_high = vmull_u16(vreinterpret_u16_s16(vget_high_s16(store_v)),
                                vget_high_u16(spectrum_v));
    vst1q_s32(echo_est_p, vreinterpretq_s32_u32(echo_est_v_low));
    vst1q_s32(echo_est_p + 4, vreinterpretq_s32_u32(echo_est_v_high));

    echo_stored_v = vaddq_u32(echo_est_v_low, echo_stored_v);
    echo_stored_v = vaddq_u32(echo_est_v_high, echo_stored_v);

    echo_adapt_v = vmlal_u16(echo_adapt_v,
                             vreinterpret_u16_s16(vget_low_s16(adapt_v)),
                             vget_low_u16(spectrum_v));
    echo_adapt_v = vmlal_u16(echo_adapt_v,
                             vreinterpret_u16_s16(vget_high_s16(adapt_v)),
                             vget_high_u16(spectrum_v));

    start_stored_p += 8;
    start_adapt_p += 8;
    far_spectrum_p += 8;
    echo_est_p += 8;
  }

  AddLanes(far_energy, far_energy_v);
  AddLanes(echo_energy_stored, echo_stored_v);
  AddLanes(echo_energy_adapt, echo_adapt_v);

  echo_est[PART_LEN] = WEBRTC_SPL_MUL_16_U16(aecm->channelStored[PART_LEN],
                                             far_spectrum[PART_LEN]);
  *echo_energy_stored += (uint32_t)echo_est[PART_LEN];
  *far_energy += (uint32_t)far_spectrum[PART_LEN];
  *echo_energy_adapt += aecm->channelAdapt16[PART_LEN] * far_spectrum[PART_LEN];
}

void WebRtcAecm_StoreAdaptiveChannelNeon(AecmCore* aecm,
                                         const uint16_t* far_spectrum,
                                         int32_t* echo_est) {
  assert((uintptr_t)echo_est % 32 == 0);
  assert((uintptr_t)(aecm->channelStored) % 16 == 0);
  assert((uintptr_t)(aecm->channelAdapt16) % 16 == 0);

  // This is C code of following optimized code.
  // During startup we store the channel every block.
  //  memcpy(aecm->channelStored,
  //         aecm->channelAdapt16,
  //         sizeof(int16_t) * PART_LEN1);
  // Recalculate echo estimate
  //  for (i = 0; i < PART_LEN; i += 4) {
  //    echo_est[i] = WEBRTC_SPL_MUL_16_U16(aecm->channelStored[i],
  //                                        far_spectrum[i]);
  //    echo_est[i + 1] = WEBRTC_SPL_MUL_16_U16(aecm->channelStored[i + 1],
  //                                            far_spectrum[i + 1]);
  //    echo_est[i + 2] = WEBRTC_SPL_MUL_16_U16(aecm->channelStored[i + 2],
  //                                            far_spectrum[i + 2]);
  //    echo_est[i + 3] = WEBRTC_SPL_MUL_16_U16(aecm->channelStored[i + 3],
  //                                            far_spectrum[i + 3]);
  //  }
  //  echo_est[i] = WEBRTC_SPL_MUL_16_U16(aecm->channelStored[i],
  //                                     far_spectrum[i]);
  const uint16_t* far_spectrum_p = far_spectrum;
  int16_t* start_adapt_p = aecm->channelAdapt16;
  int16_t* start_stored_p = aecm->channelStored;
  const int16_t* end_stored_p = aecm->channelStored + PART_LEN;
  int32_t* echo_est_p = echo_est;

  uint16x8_t far_spectrum_v;
  int16x8_t adapt_v;
  uint32x4_t echo_est_v_low, echo_est_v_high;

  while (start_stored_p < end_stored_p) {
    far_spectrum_v = vld1q_u16(far_spectrum_p);
    adapt_v = vld1q_s16(start_adapt_p);

    vst1q_s16(start_stored_p, adapt_v);

    echo_est_v_low = vmull_u16(vget_low_u16(far_spectrum_v),
                               vget_low_u16(vreinterpretq_u16_s16(adapt_v)));
    echo_est_v_high = vmull_u16(vget_high_u16(far_spectrum_v),
                                vget_high_u16(vreinterpretq_u16_s16(adapt_v)));

    vst1q_s32(echo_est_p, vreinterpretq_s32_u32(echo_est_v_low));
    vst1q_s32(echo_est_p + 4, vreinterpretq_s32_u32(echo_est_v_high));

    far_spectrum_p += 8;
    start_adapt_p += 8;
    start_stored_p += 8;
    echo_est_p += 8;
  }
  aecm->channelStored[PART_LEN] = aecm->channelAdapt16[PART_LEN];
  echo_est[PART_LEN] = WEBRTC_SPL_MUL_16_U16(aecm->channelStored[PART_LEN],
                                             far_spectrum[PART_LEN]);
}

void WebRtcAecm_ResetAdaptiveChannelNeon(AecmCore* aecm) {
  assert((uintptr_t)(aecm->channelStored) % 16 == 0);
  assert((uintptr_t)(aecm->channelAdapt16) % 16 == 0);
  assert((uintptr_t)(aecm->channelAdapt32) % 32 == 0);

  // The C code of following optimized code.
  // for (i = 0; i < PART_LEN1; i++) {
  //   aecm->channelAdapt16[i] = aecm->channelStored[i];
  //   aecm->channelAdapt32[i] = WEBRTC_SPL_LSHIFT_W32(
  //              (int32_t)aecm->channelStored[i], 16);
  // }

  int16_t* start_stored_p = aecm->channelStored;
  int16_t* start_adapt16_p = aecm->channelAdapt16;
  int32_t* start_adapt32_p = aecm->channelAdapt32;
  const int16_t* end_stored_p = start_stored_p + PART_LEN;

  int16x8_t stored_v;
  int32x4_t adapt32_v_low, adapt32_v_high;

  while (start_stored_p < end_stored_p) {
    stored_v = vld1q_s16(start_stored_p);
    vst1q_s16(start_adapt16_p, stored_v);

    adapt32_v_low = vshll_n_s16(vget_low_s16(stored_v), 16);
    adapt32_v_high = vshll_n_s16(vget_high_s16(stored_v), 16);

    vst1q_s32(start_adapt32_p, adapt32_v_low);
    vst1q_s32(start_adapt32_p + 4, adapt32_v_high);

    start_stored_p += 8;
    start_adapt16_p += 8;
    start_adapt32_p += 8;
  }
  aecm->channelAdapt16[PART_LEN] = aecm->channelStored[PART_LEN];
  aecm->channelAdapt32[PART_LEN] = (int32_t)aecm->channelStored[PART_LEN] << 16;
}
