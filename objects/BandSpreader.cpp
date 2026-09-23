#include <BandSpreader.h>
#include <od/config.h>
#include <cmath>

namespace {
constexpr float kTwoPi = 6.28318530717958647692f;
constexpr float kCenterPanGain = 0.70710678118654752440f;

const float kCrossovers[5] = {
    120.0f, 350.0f, 1000.0f, 2800.0f, 8000.0f};

inline float clamp(float value, float minimum, float maximum) {
  return value < minimum ? minimum : (value > maximum ? maximum : value);
}
}

BandSpreader::BandSpreader() {
  addInput(mInputL);
  addInput(mInputR);
  addOutput(mOutputL);
  addOutput(mOutputR);

  addInput(mPan1);
  addInput(mPan2);
  addInput(mPan3);
  addInput(mPan4);
  addInput(mPan5);
  addInput(mPan6);

  addInput(mLevel1);
  addInput(mLevel2);
  addInput(mLevel3);
  addInput(mLevel4);
  addInput(mLevel5);
  addInput(mLevel6);

  const float sampleRate = static_cast<float>(globalConfig.sampleRate);

  for (int i = 0; i < kCrossoverCount; i++) {
    mLowpassState[i] = 0.0f;
    mLowpassCoeff[i] = 1.0f -
                       std::exp(-kTwoPi * kCrossovers[i] / sampleRate);
  }

  for (int i = 0; i < kBandCount; i++) {
    mLeftGain[i] = kCenterPanGain;
    mRightGain[i] = kCenterPanGain;
  }

  mControlSmoothingCoeff =
      1.0f - std::exp(-1.0f / (0.003f * sampleRate));
}

BandSpreader::~BandSpreader() {}

void BandSpreader::process() {
  const float* inputL = mInputL.buffer();
  const float* inputR = mInputR.buffer();
  float* outputL = mOutputL.buffer();
  float* outputR = mOutputR.buffer();

  const float* pan[kBandCount] = {
      mPan1.buffer(), mPan2.buffer(), mPan3.buffer(),
      mPan4.buffer(), mPan5.buffer(), mPan6.buffer()};

  const float* level[kBandCount] = {
      mLevel1.buffer(), mLevel2.buffer(), mLevel3.buffer(),
      mLevel4.buffer(), mLevel5.buffer(), mLevel6.buffer()};

  float targetLeft[kBandCount];
  float targetRight[kBandCount];

  for (int band = 0; band < kBandCount; band++) {
    const float panValue = clamp(pan[band][0], -1.0f, 1.0f);
    const float levelValue = clamp(level[band][0], 0.0f, 1.0f);

    targetLeft[band] =
        levelValue * std::sqrt(0.5f * (1.0f - panValue));
    targetRight[band] =
        levelValue * std::sqrt(0.5f * (1.0f + panValue));
  }

  float lowpass[kCrossoverCount] = {
      mLowpassState[0], mLowpassState[1], mLowpassState[2],
      mLowpassState[3], mLowpassState[4]};

  const float smoothing = mControlSmoothingCoeff;

  for (int i = 0; i < FRAMELENGTH; i++) {
    const float input = 0.5f * (inputL[i] + inputR[i]);

    for (int split = 0; split < kCrossoverCount; split++) {
      lowpass[split] +=
          mLowpassCoeff[split] * (input - lowpass[split]);
    }

    const float bands[kBandCount] = {
        lowpass[0],
        lowpass[1] - lowpass[0],
        lowpass[2] - lowpass[1],
        lowpass[3] - lowpass[2],
        lowpass[4] - lowpass[3],
        input - lowpass[4]};

    float left = 0.0f;
    float right = 0.0f;

    for (int band = 0; band < kBandCount; band++) {
      mLeftGain[band] +=
          smoothing * (targetLeft[band] - mLeftGain[band]);
      mRightGain[band] +=
          smoothing * (targetRight[band] - mRightGain[band]);

      left += bands[band] * mLeftGain[band];
      right += bands[band] * mRightGain[band];
    }

    outputL[i] = left;
    outputR[i] = right;
  }

  for (int split = 0; split < kCrossoverCount; split++) {
    mLowpassState[split] = lowpass[split];
  }
}
