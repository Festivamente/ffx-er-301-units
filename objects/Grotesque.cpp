#include <Grotesque.h>
#include <od/config.h>
#include <od/extras/Random.h>
#include <cmath>

namespace {
constexpr float kTwoPi = 6.28318530717958647692f;

inline float clamp01(float x) {
  if (x < 0.0f)
    return 0.0f;
  if (x > 1.0f)
    return 1.0f;
  return x;
}

inline float softLimit(float x) {
  if (x >= 3.0f)
    return 1.0f;
  if (x <= -3.0f)
    return -1.0f;

  const float x2 = x * x;
  return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

inline float exponentialMap(float minimum, float maximum, float position) {
  return minimum * std::pow(maximum / minimum, clamp01(position));
}

inline float onePoleCoefficient(float frequency) {
  const float maximum = 0.42f * globalConfig.sampleRate;
  if (frequency > maximum)
    frequency = maximum;
  return 1.0f - std::exp(-kTwoPi * frequency * globalConfig.samplePeriod);
}
}

Grotesque::Grotesque() {
  addInput(mInput);
  addInput(mHigh);
  addInput(mLow);
  addInput(mGain);
  addInput(mInputLevel);
  addOutput(mOutput);
}

Grotesque::~Grotesque() = default;

void Grotesque::process() {
  const float* input = mInput.buffer();
  const float* highControl = mHigh.buffer();
  const float* lowControl = mLow.buffer();
  const float* gainControl = mGain.buffer();
  const float* inputControl = mInputLevel.buffer();
  float* output = mOutput.buffer();

  float hpState1 = mHighPassState1;
  float hpState2 = mHighPassState2;
  float lpState1 = mLowPassState1;
  float lpState2 = mLowPassState2;
  float feedbackState = mFeedbackState;
  float dcInputState = mDcInputState;
  float dcOutputState = mDcOutputState;

  const float dcCoefficient =
      std::exp(-kTwoPi * 12.0f * globalConfig.samplePeriod);

  for (int i = 0; i < FRAMELENGTH; i++) {
    const float high = clamp01(highControl[i]);
    const float low = clamp01(lowControl[i]);
    const float gain = clamp01(gainControl[i]);
    const float inputAmount = clamp01(inputControl[i]);

    const float lowPassCutoff = exponentialMap(150.0f, 18000.0f, high);
    const float highPassCutoff = exponentialMap(7000.0f, 25.0f, low);
    const float lowPassCoeff = onePoleCoefficient(lowPassCutoff);
    const float highPassCoeff = onePoleCoefficient(highPassCutoff);

    const float poleMix = 0.15f + 0.85f * gain;

    const float feedbackAmount =
        0.02f + 0.80f * high + 0.40f * low + 0.65f * gain * gain;

    const float inputGain = inputAmount * inputAmount;

    const float stageGain = 0.75f + 4.5f * gain * gain;

    const float noiseAmount =
        (2.0e-7f + 3.0e-5f * gain * gain * gain) * (1.0f - inputAmount);
    const float noise = od::Random::generateFloat(-noiseAmount, noiseAmount);

    const float feedback = softLimit(feedbackState * feedbackAmount);
    const float driven = softLimit(input[i] * inputGain * stageGain +
                                   feedback + noise);

    hpState1 += highPassCoeff * (driven - hpState1);
    const float highPass1 = driven - hpState1;

    hpState2 += highPassCoeff * (highPass1 - hpState2);
    const float highPass2 = highPass1 - hpState2;
    const float highPassed =
        highPass1 + poleMix * (highPass2 - highPass1);

    lpState1 += lowPassCoeff * (highPassed - lpState1);
    lpState2 += lowPassCoeff * (lpState1 - lpState2);
    const float filtered =
        lpState1 + poleMix * (lpState2 - lpState1);

    feedbackState = filtered;

    const float limited = softLimit(filtered * 1.15f);

    const float dcBlocked =
        limited - dcInputState + dcCoefficient * dcOutputState;
    dcInputState = limited;
    dcOutputState = dcBlocked;

    output[i] = dcBlocked;
  }

  mHighPassState1 = hpState1;
  mHighPassState2 = hpState2;
  mLowPassState1 = lpState1;
  mLowPassState2 = lpState2;
  mFeedbackState = feedbackState;
  mDcInputState = dcInputState;
  mDcOutputState = dcOutputState;
}
