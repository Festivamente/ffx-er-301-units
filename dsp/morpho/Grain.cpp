#include "Grain.h"
#include <od/extras/LookupTables.h>
#include <hal/simd.h>
#include <od/config.h>
#include <hal/ops.h>

namespace morpho {
using namespace od;

Grain::Grain() {}

Grain::~Grain() {
  setSample(0);
}

void Grain::setSample(Sample* sample) {
  stop();

  if (mpSample)
    mpSample->release();
  mpSample = sample;

  if (mpSample) {
    mpSample->attach();
    mSpeedAdjustment = sample->mSampleRate * globalConfig.samplePeriod;
    setRegion(0, (int)sample->mSampleCount);
  } else {
    mRegionStart = 0;
    mRegionLength = 0;
  }
}

void Grain::setRegion(int start, int length) {
  if (mpSample == 0 || mpSample->mSampleCount == 0) {
    mRegionStart = 0;
    mRegionLength = 0;
    return;
  }

  const int sampleCount = (int)mpSample->mSampleCount;
  start = CLAMP(0, sampleCount - 1, start);
  length = CLAMP(1, sampleCount - start, length);
  mRegionStart = start;
  mRegionLength = length;
}

int Grain::wrapIndex(int index) const {
  if (mRegionLength <= 0) {
    return 0;
  }

  const int regionEnd = mRegionStart + mRegionLength;
  if (index >= mRegionStart && index < regionEnd) {
    return index;
  }

  int offset = (index - mRegionStart) % mRegionLength;
  if (offset < 0) {
    offset += mRegionLength;
  }
  return mRegionStart + offset;
}

void Grain::init(int start, int duration, float speed, float gain, float pan) {
  mCurrentIndex = wrapIndex(start);
  mDelayInSamples = 0;
  duration = MAX(64, duration);
  mDuration = mRemaining = 4 * (duration / 4);
  mPhase = 0.0f;
  mPhaseDelta = speed * mSpeedAdjustment;

  if (pan < -1e-5f) {
    mLeftBalance = gain;
    mRightBalance = gain * (1.0f + pan);
  } else if (pan > 1e-5f) {
    mLeftBalance = gain * (1.0f - pan);
    mRightBalance = gain;
  } else {
    mLeftBalance = gain;
    mRightBalance = gain;
  }

  mEnvelopePhase = 0.0f;
  mEnvelopePhaseDelta = 0.5f / mDuration;
  mActive = mpSample != 0;
}

void Grain::snapToZeroCrossing(int window) {
  int end;
  float current;
  float previous;
  int forward;
  int backward;
  bool forwardFound = false;
  bool backwardFound = false;

  end = MIN(mpSample->mSampleCount, (uint32_t)(mCurrentIndex + window));
  previous = mpSample->get(mCurrentIndex, 0);
  for (int i = mCurrentIndex; i < end; i++) {
    current = mpSample->get(i, 0);
    if (previous < 0 && current >= 0) {
      forward = i;
      forwardFound = true;
      break;
    }
  }

  end = MAX(0, mCurrentIndex - window);
  previous = mpSample->get(mCurrentIndex, 0);
  for (int i = mCurrentIndex; i >= end; i--) {
    current = mpSample->get(i, 0);
    if (current <= 0 && previous > 0) {
      backward = i;
      backwardFound = true;
      break;
    }
  }

  if (forwardFound && backwardFound) {
    if (forward - mCurrentIndex < mCurrentIndex - backward)
      mCurrentIndex = forward;
    else
      mCurrentIndex = backward;
  } else if (backwardFound) {
    mCurrentIndex = backward;
  } else if (forwardFound) {
    mCurrentIndex = forward;
  }
}

void Grain::stop() {
  mRemaining = 0;
  mActive = false;
}

void Grain::generateSineWindow(float* out, int n) {
  float phase[4] = {0.0f, 0.0f, 0.0f, 0.0f};

  for (int i = 0; i < n; i += 4) {
    mEnvelopePhase += mEnvelopePhaseDelta;
    phase[0] = mEnvelopePhase;
    mEnvelopePhase += mEnvelopePhaseDelta;
    phase[1] = mEnvelopePhase;
    mEnvelopePhase += mEnvelopePhaseDelta;
    phase[2] = mEnvelopePhase;
    mEnvelopePhase += mEnvelopePhaseDelta;
    phase[3] = mEnvelopePhase;

    vst1q_f32(out + i, simd_sine_env(phase));
  }
}

void Grain::generateHanningWindow(float* out, int n) {
  float phase[4] = {0.0f, 0.0f, 0.0f, 0.0f};

  for (int i = 0; i < n; i += 4) {
    mEnvelopePhase += mEnvelopePhaseDelta;
    phase[0] = mEnvelopePhase;
    mEnvelopePhase += mEnvelopePhaseDelta;
    phase[1] = mEnvelopePhase;
    mEnvelopePhase += mEnvelopePhaseDelta;
    phase[2] = mEnvelopePhase;
    mEnvelopePhase += mEnvelopePhaseDelta;
    phase[3] = mEnvelopePhase;

    vst1q_f32(out + i, simd_hanning(phase));
  }
}

void Grain::generateTrapezoidWindow(float* out, int n) {
  const float step = 1.0f / mFade;

  for (int i = 0, remaining = mRemaining; i < n; i++, remaining--) {
    if (remaining < 0)
      out[i] = 0.0f;
    else if (remaining < mFade)
      out[i] = remaining * step;
    else if (remaining < mDuration - mFade)
      out[i] = 1.0f;
    else
      out[i] = (mDuration - remaining) * step;
  }
}

void Grain::generateEnvelope(float* out, int n) {
  switch (mEnvelopeType) {
  case kSineWindow:
    generateSineWindow(out, n);
    break;
  case kHanningWindow:
    generateHanningWindow(out, n);
    break;
  case kTrapezoidWindow:
    generateTrapezoidWindow(out, n);
    break;
  }

  if (mSquash > 1.0f)
    squashEnvelope(out, n);
}

void Grain::squashEnvelope(float* out, int n) {
  const float32x4_t cubicCoeff = vdupq_n_f32(-1.0f / 6.75f);
  const float32x4_t maximum = vdupq_n_f32(1.5);
  const float32x4_t minimum = vdupq_n_f32(-1.5);
  const float32x4_t gain = vdupq_n_f32(mSquash);

  for (int i = 0; i < n; i += 4) {
    float32x4_t x = gain * vld1q_f32(out + i);
    x = vminq_f32(x, maximum);
    x = vmaxq_f32(x, minimum);
    const float32x4_t x3 = vmulq_f32(x, vmulq_f32(x, x));
    vst1q_f32(out + i, vmlaq_f32(x, cubicCoeff, x3));
  }
}

void Grain::setEnvelope(int type) {
  mEnvelopeType = type;
  mFade = 0;
}

void Grain::setFade(int fade) {
  mFade = fade;
}

void Grain::setSquash(float squash) {
  mSquash = squash;
}

void Grain::setDelay(int samples) {
  mDelayInSamples = CLAMP(0, mDuration, samples);

  mDelayInSamples /= 4;
  mDelayInSamples *= 4;
}

int Grain::samplesRequired() {
  return mPhaseDelta * mRemaining;
}

}
