#include <TapDelay.h>
#include <dsp/DspMath.h>
#include <dsp/DualGrainPitch.h>
#include <od/config.h>
#include <od/extras/BigHeap.h>
#include <cmath>

namespace {

constexpr float kMinSpacing = 0.020f;
constexpr float kMaxSpacing = 0.500f;

constexpr float kGrainWindow = 0.100f;
constexpr float kInitialSpacing = 0.150f;
constexpr float kSpacingSmoothing = 0.080f;
constexpr float kRatioSmoothing = 0.040f;
constexpr float kMaxFeedback = 0.95f;

inline float softclip(float x) {
  x = timefx::clamp(x, -1.7f, 1.7f);
  return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

}

TapDelay::TapDelay() {
  addInput(mInput);
  addOutput(mOutput);
  addOutput(mTap1);
  addOutput(mTap2);
  addOutput(mTap3);
  addOutput(mTap4);
  addInput(mTime);
  addInput(mFeedback);
  addInput(mReverse);
  addInput(mBlend);
  addInput(mPitch1);
  addInput(mPitch2);
  addInput(mPitch3);
  addInput(mPitch4);
  addInput(mLevel1);
  addInput(mLevel2);
  addInput(mLevel3);
  addInput(mLevel4);

  const float sampleRate = globalConfig.sampleRate;

  mWindow = kGrainWindow * sampleRate;
  mBufferSize = (int)(4.0f * kMaxSpacing * sampleRate + mWindow) + 8;
  mBuffer = (float*)od::BigHeap::allocateZeroed(
      mBufferSize * sizeof(float));

  mSpacing = kInitialSpacing * sampleRate;
  mSpacingCoeff =
      1.0f - expf(-1.0f / (kSpacingSmoothing * sampleRate));
  mRatioCoeff =
      1.0f - expf(-1.0f / (kRatioSmoothing * sampleRate));
}

TapDelay::~TapDelay() {
  if (mBuffer) {
    od::BigHeap::free((char*)mBuffer);
    mBuffer = nullptr;
  }
}

void TapDelay::process() {
  const float* in = mInput.buffer();
  float* out = mOutput.buffer();
  float* tapOut[kTapCount] = {
      mTap1.buffer(), mTap2.buffer(), mTap3.buffer(), mTap4.buffer()};
  const float* time = mTime.buffer();
  const float* feedback = mFeedback.buffer();
  const float* reverse = mReverse.buffer();
  const float* blend = mBlend.buffer();

  const float* pitch[kTapCount] = {
      mPitch1.buffer(), mPitch2.buffer(), mPitch3.buffer(), mPitch4.buffer()};
  const float* level[kTapCount] = {
      mLevel1.buffer(), mLevel2.buffer(), mLevel3.buffer(), mLevel4.buffer()};

  if (!mBuffer) {
    for (int i = 0; i < FRAMELENGTH; i++) {
      out[i] = in[i];
      for (int tap = 0; tap < kTapCount; tap++)
        tapOut[tap][i] = 0.0f;
    }
    return;
  }

  const float sampleRate = globalConfig.sampleRate;
  const float window = mWindow;
  const int bufferSize = mBufferSize;

  int writeIndex = mWriteIndex;
  float spacing = mSpacing;

  for (int i = 0; i < FRAMELENGTH; i++) {
    const float dry = in[i];

    const float targetSpacing =
        timefx::clamp(time[i], kMinSpacing, kMaxSpacing) * sampleRate;
    spacing += mSpacingCoeff * (targetSpacing - spacing);

    const bool reverseEnabled = reverse[i] > 0.5f;
    float wet = 0.0f;
    float finalTap = 0.0f;

    for (int tap = 0; tap < kTapCount; tap++) {
      float targetRatio = timefx::clamp(pitch[tap][i], -4.0f, 4.0f);
      if (reverseEnabled)
        targetRatio = -targetRatio;
      mSmoothedRatio[tap] +=
          mRatioCoeff * (targetRatio - mSmoothedRatio[tap]);

      const float tapDelay = spacing * (float)(tap + 1);
      float phase = mPhase[tap];
      const float tapOutput = timefx::readDualGrainPitch(
          mBuffer, bufferSize, writeIndex, tapDelay + 1.0f, window,
          mSmoothedRatio[tap], phase);
      mPhase[tap] = phase;

      tapOut[tap][i] = tapOutput;

      if (tap == kTapCount - 1)
        finalTap = tapOutput;

      wet += level[tap][i] * tapOutput;
    }

    const float feedbackAmount =
        timefx::clamp(feedback[i], 0.0f, kMaxFeedback);
    mBuffer[writeIndex] = dry + softclip(feedbackAmount * finalTap);

    const float mix = timefx::clamp(blend[i], 0.0f, 1.0f);
    out[i] = dry + mix * (wet - dry);

    writeIndex++;
    if (writeIndex >= bufferSize)
      writeIndex = 0;
  }

  mWriteIndex = writeIndex;
  mSpacing = spacing;
}

TapDelayStereoMixer::TapDelayStereoMixer() {
  addInput(mDryL);
  addInput(mDryR);
  addInput(mTap1L);
  addInput(mTap1R);
  addInput(mTap2L);
  addInput(mTap2R);
  addInput(mTap3L);
  addInput(mTap3R);
  addInput(mTap4L);
  addInput(mTap4R);
  addInput(mLevel1);
  addInput(mLevel2);
  addInput(mLevel3);
  addInput(mLevel4);
  addInput(mPan1);
  addInput(mPan2);
  addInput(mPan3);
  addInput(mPan4);
  addInput(mBlend);
  addOutput(mOutL);
  addOutput(mOutR);
}

void TapDelayStereoMixer::process() {
  const float* dryL = mDryL.buffer();
  const float* dryR = mDryR.buffer();
  float* outL = mOutL.buffer();
  float* outR = mOutR.buffer();

  const float* tapL[4] = {
      mTap1L.buffer(), mTap2L.buffer(), mTap3L.buffer(), mTap4L.buffer()};
  const float* tapR[4] = {
      mTap1R.buffer(), mTap2R.buffer(), mTap3R.buffer(), mTap4R.buffer()};
  const float* level[4] = {
      mLevel1.buffer(), mLevel2.buffer(), mLevel3.buffer(), mLevel4.buffer()};
  const float* pan[4] = {
      mPan1.buffer(), mPan2.buffer(), mPan3.buffer(), mPan4.buffer()};
  const float* blend = mBlend.buffer();

  for (int i = 0; i < FRAMELENGTH; i++) {
    float wetL = 0.0f;
    float wetR = 0.0f;

    for (int tap = 0; tap < 4; tap++) {
      const float levelAmount = level[tap][i];
      const float panAmount = timefx::clamp(pan[tap][i], -1.0f, 1.0f);

      const float leftGain = panAmount > 0.0f ? 1.0f - panAmount : 1.0f;
      const float rightGain = panAmount < 0.0f ? 1.0f + panAmount : 1.0f;

      wetL += levelAmount * leftGain * tapL[tap][i];
      wetR += levelAmount * rightGain * tapR[tap][i];
    }

    const float mix = timefx::clamp(blend[i], 0.0f, 1.0f);
    outL[i] = dryL[i] + mix * (wetL - dryL[i]);
    outR[i] = dryR[i] + mix * (wetR - dryR[i]);
  }
}
