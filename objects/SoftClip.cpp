#include <SoftClip.h>
#include <od/config.h>
#include <hal/ops.h>
#include <hal/simd.h>

namespace od {

inline float32x4_t sgnf_f32x4(float32x4_t x) {
  float32x4_t zero = vdupq_n_f32(0.0f);
  float32x4_t one  = vdupq_n_f32(1.0f);
  float32x4_t neg1 = vdupq_n_f32(-1.0f);
  uint32x4_t gt = vcgtq_f32(x, zero);
  uint32x4_t lt = vcltq_f32(x, zero);
  float32x4_t y = vbslq_f32(gt, one, zero);
  y = vbslq_f32(lt, neg1, y);
  return y;
}

SoftClip::SoftClip() {
  addInput(mInput);
  addOutput(mOutput);
  addInput(mGain);
  addOption(mAlgo);
}

SoftClip::~SoftClip() {}

void SoftClip::process() {
  float* in  = mInput.buffer();
  float* out = mOutput.buffer();
  float* g   = mGain.buffer();
  int n      = FRAMELENGTH;
  int algo   = mAlgo.value();

  const float32x4_t vPreDrive  = vdupq_n_f32(1.5f);
  const float32x4_t vTwoThirds = vdupq_n_f32(2.0f / 3.0f);
  const float32x4_t vOneThird  = vdupq_n_f32(1.0f / 3.0f);
  const float32x4_t vOne       = vdupq_n_f32(1.0f);
  const float32x4_t vTwo       = vdupq_n_f32(2.0f);
  const float32x4_t vThree     = vdupq_n_f32(3.0f);
  const float32x4_t vZero      = vdupq_n_f32(0.0f);
  const float32x4_t vSinScale  = vdupq_n_f32(0.75f * 3.14159265359f);

  for (int i = 0; i < n; i += 4) {
    float32x4_t x = vld1q_f32(in + i);
    float32x4_t gain = vld1q_f32(g + i);
    x = vmulq_f32(x, gain);
    x = vmulq_f32(x, vPreDrive);

    float32x4_t absx = vabsq_f32(x);
    float32x4_t sign = sgnf_f32x4(x);
    float32x4_t y = vZero;

    switch (algo) {
      case SC_TANH: {
        float32x4_t z = vmulq_n_f32(x, 5.0f);
        float32x4_t absz = vabsq_f32(z);
        absz = vminq_f32(absz, vdupq_n_f32(10.0f));
        float32x4_t e2z = simd_exp(vmulq_n_f32(absz, 2.0f));
        float32x4_t invDen = simd_invert(vaddq_f32(e2z, vOne));
        y = vmulq_f32(sign, vsubq_f32(vOne, vmulq_f32(vTwo, invDen)));
      } break;

      case SC_SIN: {
        uint32x4_t mask = vcgtq_f32(absx, vTwoThirds);
        float32x4_t sinv = simd_sin(vmulq_f32(x, vSinScale));
        y = vbslq_f32(mask, sign, sinv);
      } break;

      case SC_EXP: {
        uint32x4_t mask = vcgtq_f32(absx, vTwoThirds);
        float32x4_t t = vabsq_f32(vsubq_f32(vmulq_n_f32(x, 1.5f), sign));
        float32x4_t val = vmulq_f32(sign, vsubq_f32(vOne, vmulq_f32(t, t)));
        y = vbslq_f32(mask, sign, val);
      } break;

      case SC_2STG: {
        uint32x4_t maskHigh = vcgtq_f32(absx, vTwoThirds);
        uint32x4_t maskMid = vandq_u32(vmvnq_u32(maskHigh), vcgeq_f32(absx, vOneThird));
        uint32x4_t maskLow = vmvnq_u32(vorrq_u32(maskHigh, maskMid));

        float32x4_t inner = vsubq_f32(vTwo, vabsq_f32(vmulq_n_f32(x, 3.0f)));
        float32x4_t mid = vmulq_f32(sign,
          vmulq_n_f32(vsubq_f32(vThree, vmulq_f32(inner, inner)), 1.0f / 3.0f));
        float32x4_t low = vmulq_n_f32(x, 2.0f);

        float32x4_t sel = vbslq_f32(maskLow, low, vZero);
        sel = vbslq_f32(maskMid, mid, sel);
        sel = vbslq_f32(maskHigh, sign, sel);
        y = sel;
      } break;

      case SC_CUBIC: {
        uint32x4_t mask = vcgtq_f32(absx, vTwoThirds);
        float32x4_t x3 = vmulq_f32(x, vmulq_f32(x, x));
        float32x4_t poly = vsubq_f32(vmulq_n_f32(x, 9.0f / 4.0f),
                                     vmulq_n_f32(x3, 27.0f / 16.0f));
        y = vbslq_f32(mask, sign, poly);
      } break;

      case SC_RECIP: {
        float32x4_t den = vaddq_f32(vmulq_n_f32(absx, 30.0f), vOne);
        float32x4_t curve = vsubq_f32(vOne, simd_invert(den));
        y = vmulq_f32(sign, curve);
      } break;

      default: y = x; break;
    }

    vst1q_f32(out + i, y);
  }
}

}
