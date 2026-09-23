#include <CompressorObject.h>
#include <od/config.h>
#include <hal/ops.h>
#include <cmath>

CompressorObject::CompressorObject() {
  addInput(mInput);
  addOutput(mOutput);
  addInput(mSideChain);
  addInput(mThreshold);
  addInput(mRatio);

  constexpr float smoothingTime = 0.001f;
  mGainSmoothingCoeff = 1.0f - expf(-1.0f / (smoothingTime * globalConfig.sampleRate));
}

CompressorObject::~CompressorObject() {}

void CompressorObject::process() {
  const float* in = mInput.buffer();
  float* out = mOutput.buffer();
  const float* threshold = mThreshold.buffer();
  const float* ratio = mRatio.buffer();
  const float* envelope = mSideChain.buffer();

  float gain = mSmoothedGain;

  for (int i = 0; i < FRAMELENGTH; i++) {
    float env = envelope[i];
    if (env < 0.0f) env = 0.0f;

    float thr = threshold[i];
    if (thr < 1e-5f) thr = 1e-5f;

    float rat = ratio[i];
    if (rat < 1.0f) rat = 1.0f;

    float targetGain = 1.0f;
    if (env > thr) {
      targetGain = powf(env / thr, (1.0f / rat) - 1.0f);
    }

    gain += mGainSmoothingCoeff * (targetGain - gain);
    out[i] = in[i] * gain;
  }

  mSmoothedGain = gain;
}
