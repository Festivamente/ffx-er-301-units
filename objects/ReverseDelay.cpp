#include <ReverseDelay.h>
#include <od/extras/BigHeap.h>
#include <od/config.h>
#include <hal/ops.h>
#include <string.h>

namespace {
constexpr float kMinimumAllocationSeconds = 0.001f;
constexpr float kSegmentFadeSeconds = 0.005f;
}

ReverseDelay::ReverseDelay(int channelCount) : mChannelCount(channelCount) {
  addInput(mLeftInput);
  addInput(mFeedback);
  addParameter(mDelay);
  mDelay.enableSerialization();
  addOutput(mLeftOutput);

  if (channelCount > 1) {
    addInput(mRightInput);
    addOutput(mRightOutput);
  }
}

ReverseDelay::~ReverseDelay() {
  deallocate();
}

void ReverseDelay::resetLine(Line& line) {
  line.mWriteIndex = 0;
  line.mAnchor = 0;
  line.mCounter = 0;
  line.mSegment = 0;
}

void ReverseDelay::zero() {
  if (mLeft.mpBuffer)
    memset(mLeft.mpBuffer, 0, mMaxSamples * sizeof(float));
  if (mRight.mpBuffer)
    memset(mRight.mpBuffer, 0, mMaxSamples * sizeof(float));

  resetLine(mLeft);
  resetLine(mRight);
}

bool ReverseDelay::allocate(int sampleCount) {
  deallocate();
  const int byteCount = sampleCount * sizeof(float);

  mLeft.mpBuffer = od::BigHeap::allocateZeroed(byteCount);
  if (mLeft.mpBuffer == 0)
    return false;

  if (mChannelCount > 1) {
    mRight.mpBuffer = od::BigHeap::allocateZeroed(byteCount);
    if (mRight.mpBuffer == 0) {
      deallocate();
      return false;
    }
  }

  return true;
}

void ReverseDelay::deallocate() {
  if (mLeft.mpBuffer) {
    od::BigHeap::free(mLeft.mpBuffer);
    mLeft.mpBuffer = 0;
  }
  if (mRight.mpBuffer) {
    od::BigHeap::free(mRight.mpBuffer);
    mRight.mpBuffer = 0;
  }

  mMaxSamples = 0;
  mMaxDelaySamples = 0;
  resetLine(mLeft);
  resetLine(mRight);
}

float ReverseDelay::allocateTimeUpTo(float seconds) {

  const int delaySamples =
      (int)(globalConfig.sampleRate * MAX(kMinimumAllocationSeconds, seconds));
  int frameCount = (2 * delaySamples) / FRAMELENGTH + 1;
  int sampleCount = frameCount * FRAMELENGTH;

  if (sampleCount == mMaxSamples)
    return mMaxDelaySamples * globalConfig.samplePeriod;

  while (frameCount > 1) {
    if (allocate(frameCount * FRAMELENGTH)) {
      mMaxSamples = frameCount * FRAMELENGTH;
      mMaxDelaySamples = mMaxSamples / 2;
      return mMaxDelaySamples * globalConfig.samplePeriod;
    }
    frameCount /= 2;
  }

  return 0;
}

float ReverseDelay::minimumDelayTime() {
  return FRAMELENGTH * globalConfig.samplePeriod;
}

float ReverseDelay::maximumDelayTime() {
  return mMaxDelaySamples * globalConfig.samplePeriod;
}

void ReverseDelay::process() {
  if (mMaxSamples == 0 || mLeft.mpBuffer == 0 ||
      (mChannelCount > 1 && mRight.mpBuffer == 0)) {
    memcpy(mLeftOutput.buffer(), mLeftInput.buffer(),
           sizeof(float) * FRAMELENGTH);
    if (mChannelCount > 1) {
      memcpy(mRightOutput.buffer(), mRightInput.buffer(),
             sizeof(float) * FRAMELENGTH);
    }
    return;
  }

  int delaySamples = (int)(mDelay.target() * globalConfig.sampleRate);
  delaySamples = CLAMP(FRAMELENGTH, mMaxDelaySamples, delaySamples);

  const int fadeSamples = (int)(kSegmentFadeSeconds * globalConfig.sampleRate);

  pushSamples(mLeft, mLeftInput.buffer(), mLeftOutput.buffer(),
              mFeedback.buffer(), delaySamples, fadeSamples);
  if (mChannelCount > 1) {
    pushSamples(mRight, mRightInput.buffer(), mRightOutput.buffer(),
                mFeedback.buffer(), delaySamples, fadeSamples);
  }
}

void ReverseDelay::pushSamples(Line& line, const float* in, float* out,
                               const float* feedback, int delaySamples,
                               int fadeSamples) {
  float* buffer = (float*)line.mpBuffer;

  for (int i = 0; i < FRAMELENGTH; i++) {
    if (line.mCounter >= line.mSegment) {

      line.mSegment = delaySamples;
      line.mCounter = 0;
      line.mAnchor = line.mWriteIndex;
    }

    int fade = MIN(fadeSamples, line.mSegment / 4);
    if (fade < 1)
      fade = 1;

    float segmentGain = 1.0f;
    if (line.mCounter < fade) {
      segmentGain = line.mCounter / (float)fade;
    } else {
      const int remaining = line.mSegment - line.mCounter;
      if (remaining < fade)
        segmentGain = remaining / (float)fade;
    }

    int readIndex = line.mAnchor - 1 - line.mCounter;
    while (readIndex < 0)
      readIndex += mMaxSamples;

    const float reversed = buffer[readIndex] * segmentGain;
    const float feedbackAmount = CLAMP(0.0f, 1.0f, feedback[i]);

    buffer[line.mWriteIndex] = in[i] + feedbackAmount * reversed;
    out[i] = reversed;

    line.mWriteIndex++;
    if (line.mWriteIndex >= mMaxSamples)
      line.mWriteIndex = 0;
    line.mCounter++;
  }
}
