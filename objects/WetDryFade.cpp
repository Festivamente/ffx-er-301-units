#include <WetDryFade.h>
#include <od/config.h>
#include <cmath>

namespace {
inline float clampUnit(float x) {
  if (x < 0.0f)
    return 0.0f;
  if (x > 1.0f)
    return 1.0f;
  return x;
}
}

WetDryFade::WetDryFade() {
  addInput(mInputA);
  addInput(mInputB);
  addInput(mFade);
  addOutput(mOutput);

  mFadeCoeff = 1.0f - std::exp(-1.0f / (0.005f * globalConfig.sampleRate));
}

WetDryFade::~WetDryFade() {}

void WetDryFade::process() {
  const float* dry = mInputA.buffer();
  const float* wet = mInputB.buffer();
  const float* fadeBuffer = mFade.buffer();
  float* out = mOutput.buffer();

  float fade = mSmoothedFade;

  for (int i = 0; i < FRAMELENGTH; i++) {
    const float target = clampUnit(fadeBuffer[i]);
    fade += mFadeCoeff * (target - fade);

    out[i] = dry[i] + fade * (wet[i] - dry[i]);
  }

  mSmoothedFade = fade;
}
