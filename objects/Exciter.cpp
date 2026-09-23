#include <Exciter.h>
#include <od/config.h>
#include <hal/ops.h>
#include <cmath>

static inline float cutoffToCoeff(float hz) {
  return 1.0f - expf(-6.2831853f * hz / globalConfig.sampleRate);
}

Exciter::Exciter() {
  addInput(mInput);
  addOutput(mOutput);
  addOption(mMode);
  addInput(mAxMix);

  mHpCoeff = cutoffToCoeff(2000.0f);
  mDcCoeff = cutoffToCoeff(10.0f);
}

Exciter::~Exciter() {}

void Exciter::process() {
  const float* input = mInput.buffer();
  float* output = mOutput.buffer();
  const float* axMix = mAxMix.buffer();
  const int mode = mMode.value();

  float hpLp = mHpLp;
  float dcLp = mDcLp;

  for (int i = 0; i < FRAMELENGTH; i++) {
    const float x = input[i];

    hpLp += mHpCoeff * (x - hpLp);
    float high = x - hpLp;

    if (high > 1.5f)
      high = 1.5f;
    else if (high < -1.5f)
      high = -1.5f;

    float excited;
    switch (mode) {
      case 4:
        excited = high * (27.0f + high * high) /
                  (27.0f + 9.0f * high * high);
        break;

      case 5:
        excited = high + 0.6f * high * high;
        break;

      case 6: {
        const float clipped = high > 1.0f ? 1.0f :
                              (high < -1.0f ? -1.0f : high);
        excited = 1.5f * clipped -
                  0.5f * clipped * clipped * clipped;
        break;
      }

      case 7:
        excited = fabsf(high);
        break;

      case 8:
        excited = high + 0.4f * high * high -
                  0.3f * high * high * high;
        break;

      case 9:
        excited = sinf(3.0f * high);
        break;

      default:
        excited = high;
        break;
    }

    dcLp += mDcCoeff * (excited - dcLp);
    excited -= dcLp;

    output[i] = x + axMix[i] * excited;
  }

  mHpLp = hpLp;
  mDcLp = dcLp;
}
