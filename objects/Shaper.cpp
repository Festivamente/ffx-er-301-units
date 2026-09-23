#include <Shaper.h>
#include <od/config.h>
#include <cmath>

namespace {

inline float timeToCoeff(float seconds) {
  return 1.0f - expf(-1.0f / (seconds * globalConfig.sampleRate));
}

}

Shaper::Shaper() {
  addInput(mInput);
  addOutput(mOutput);
  addInput(mAttack);
  addInput(mSustain);

  mFastAttackCoeff = timeToCoeff(0.001f);
  mFastReleaseCoeff = timeToCoeff(0.015f);
  mSlowAttackCoeff = timeToCoeff(0.020f);
  mSlowReleaseCoeff = timeToCoeff(0.150f);

  mGainSmoothingCoeff = timeToCoeff(0.002f);
}

Shaper::~Shaper() {}

void Shaper::process() {
  const float* input = mInput.buffer();
  float* output = mOutput.buffer();
  const float* attack = mAttack.buffer();
  const float* sustain = mSustain.buffer();

  constexpr float maxRangeDB = 15.0f;
  constexpr float dbToNatural = 0.11512925f;

  float fastEnv = mFastEnv;
  float slowEnv = mSlowEnv;
  float gain = mSmoothedGain;

  for (int i = 0; i < FRAMELENGTH; i++) {
    const float x = input[i];
    const float level = fabsf(x);

    const float fastCoeff = level > fastEnv ? mFastAttackCoeff : mFastReleaseCoeff;
    fastEnv += fastCoeff * (level - fastEnv);

    const float slowCoeff = level > slowEnv ? mSlowAttackCoeff : mSlowReleaseCoeff;
    slowEnv += slowCoeff * (level - slowEnv);

    float transient = (fastEnv - slowEnv) / (slowEnv + 1.0e-6f);
    if (transient > 1.0f) transient = 1.0f;
    else if (transient < -1.0f) transient = -1.0f;

    const float onset = transient > 0.0f ? transient : 0.0f;
    const float decay = transient < 0.0f ? -transient : 0.0f;

    float attackAmount = (attack[i] - 0.5f) * 2.0f;
    float sustainAmount = (sustain[i] - 0.5f) * 2.0f;

    if (attackAmount > 1.0f) attackAmount = 1.0f;
    else if (attackAmount < -1.0f) attackAmount = -1.0f;

    if (sustainAmount > 1.0f) sustainAmount = 1.0f;
    else if (sustainAmount < -1.0f) sustainAmount = -1.0f;

    const float gainDB = maxRangeDB *
        (attackAmount * onset + sustainAmount * decay);
    const float targetGain = expf(dbToNatural * gainDB);

    gain += mGainSmoothingCoeff * (targetGain - gain);
    output[i] = x * gain;
  }

  mFastEnv = fastEnv;
  mSlowEnv = slowEnv;
  mSmoothedGain = gain;
}
