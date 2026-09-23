#include <Pluck.h>
#include <od/extras/BigHeap.h>
#include <od/config.h>
#include <hal/ops.h>
#include <math.h>
#include <string.h>

namespace {
constexpr float kDcBlockerPole = 0.9995f;
constexpr float kPitchSettleSeconds = 0.005f;

constexpr float kDecayReferenceSeconds = 0.0763f;
}

Pluck::Pluck(float seconds) {
  addInput(mInput);
  addInput(mTrigger);
  addInput(mFrequency);
  addInput(mDamp);
  addInput(mDecay);
  addOutput(mOutput);
  allocateTimeUpTo(seconds);
}

Pluck::~Pluck() {
  deallocate();
}

float Pluck::allocateTimeUpTo(float seconds) {
  int sampleCount = (int)(globalConfig.sampleRate * MAX(0.005f, seconds));
  int frameCount = sampleCount / FRAMELENGTH + 1;
  sampleCount = frameCount * FRAMELENGTH;

  if (sampleCount == mMaxDelayInSamples)
    return sampleCount * globalConfig.samplePeriod;

  deallocate();

  while (frameCount > 1) {
    const int samplesPerVoice = frameCount * FRAMELENGTH;
    float* block = (float*)od::BigHeap::allocateZeroed(
        kVoices * samplesPerVoice * sizeof(float));
    if (block) {
      mpBlock = block;
      mMaxDelayInSamples = samplesPerVoice;
      for (int voice = 0; voice < kVoices; voice++)
        mpBuffer[voice] = block + voice * samplesPerVoice;
      mWriteIndex = 0;
      return samplesPerVoice * globalConfig.samplePeriod;
    }
    frameCount /= 2;
  }

  return 0.0f;
}

void Pluck::deallocate() {
  if (mpBlock) {
    od::BigHeap::free((char*)mpBlock);
    mpBlock = 0;
  }

  for (int voice = 0; voice < kVoices; voice++) {
    mpBuffer[voice] = 0;
    mLp[voice] = 0.0f;
    mDcX[voice] = 0.0f;
    mDcY[voice] = 0.0f;
  }

  mMaxDelayInSamples = 0;
  mWriteIndex = 0;
  mTrackRemaining = 0;
  mPreviousTrigger = 0.0f;
}

void Pluck::zero() {
  if (mpBlock)
    bzero(mpBlock, kVoices * mMaxDelayInSamples * sizeof(float));

  for (int voice = 0; voice < kVoices; voice++) {
    mLp[voice] = 0.0f;
    mDcX[voice] = 0.0f;
    mDcY[voice] = 0.0f;
  }
}

void Pluck::process() {
  const float* in = mInput.buffer();
  const float* trigger = mTrigger.buffer();
  const float* frequency = mFrequency.buffer();
  const float* damp = mDamp.buffer();
  const float* decay = mDecay.buffer();
  float* out = mOutput.buffer();

  if (mMaxDelayInSamples == 0) {

    memset(out, 0, sizeof(float) * FRAMELENGTH);
    return;
  }

  const float sampleRate = (float)globalConfig.sampleRate;
  const int settleWindow = (int)(kPitchSettleSeconds * sampleRate);

  float cutoff = damp[0];
  if (cutoff < 20.0f)
    cutoff = 20.0f;
  else if (cutoff > 0.45f * sampleRate)
    cutoff = 0.45f * sampleRate;

  const float dampingPole =
      expf(-2.0f * (float)M_PI * cutoff / sampleRate);
  const float dampingGain = 1.0f - dampingPole;

  float decayAmount = decay[0];
  if (decayAmount < 1e-6f)
    decayAmount = 1e-6f;
  else if (decayAmount > 0.999995f)
    decayAmount = 0.999995f;

  const float logDecayOverReference =
      logf(decayAmount) / (kDecayReferenceSeconds * sampleRate);
  const float maxDelay = (float)(mMaxDelayInSamples - 8);
  const float minFrequency = sampleRate / maxDelay;
  const float maxFrequency = 0.25f * sampleRate;

  float voiceDelay[kVoices];
  float voiceFeedback[kVoices];

  auto setupVoice = [&](float frequencyHz, float& delayOut,
                        float& feedbackOut) {
    const float period = sampleRate / frequencyHz;
    const float omega = 2.0f * (float)M_PI * frequencyHz / sampleRate;
    const float cosine = cosf(omega);
    const float sine = sinf(omega);

    const float lpReal = 1.0f - dampingPole * cosine;
    const float lpImag = dampingPole * sine;
    const float lpMagnitude =
        dampingGain / sqrtf(lpReal * lpReal + lpImag * lpImag);
    const float lpDelay = atan2f(lpImag, lpReal) / omega;

    const float dcReal = 1.0f - kDcBlockerPole * cosine;
    const float dcImag = kDcBlockerPole * sine;
    const float dcMagnitude =
        (2.0f * sinf(0.5f * omega)) /
        sqrtf(dcReal * dcReal + dcImag * dcImag);
    const float dcLead =
        (0.5f * ((float)M_PI - omega) - atan2f(dcImag, dcReal)) / omega;

    float delay = period - lpDelay + dcLead;
    if (delay < 4.0f)
      delay = 4.0f;
    else if (delay > maxDelay)
      delay = maxDelay;
    delayOut = delay;

    const float targetGain = expf(logDecayOverReference * period);
    float feedbackGain = targetGain / (lpMagnitude * dcMagnitude);
    if (feedbackGain > 4.0f)

      feedbackGain = 4.0f;
    feedbackOut = feedbackGain;
  };

  for (int voice = 0; voice < kVoices; voice++)
    setupVoice(mFreq[voice], voiceDelay[voice], voiceFeedback[voice]);

  float previousTrigger = mPreviousTrigger;

  for (int i = 0; i < FRAMELENGTH; i++) {
    const float triggerValue = trigger[i];
    if (triggerValue > 0.5f && previousTrigger <= 0.5f) {
      mCurrent++;
      if (mCurrent == kVoices)
        mCurrent = 0;
      mTrackRemaining = settleWindow;
    }
    previousTrigger = triggerValue;

    if (mTrackRemaining > 0) {
      float frequencyHz = frequency[i];
      if (frequencyHz < minFrequency)
        frequencyHz = minFrequency;
      else if (frequencyHz > maxFrequency)
        frequencyHz = maxFrequency;

      mFreq[mCurrent] = frequencyHz;
      mTrackRemaining--;
      setupVoice(frequencyHz, voiceDelay[mCurrent], voiceFeedback[mCurrent]);
    }

    float sum = 0.0f;

    for (int voice = 0; voice < kVoices; voice++) {
      const float delay = voiceDelay[voice];

      const int base = (int)delay - 1;
      const float fraction = delay - (float)base;
      const float u1 = fraction - 1.0f;
      const float u2 = fraction - 2.0f;
      const float u3 = fraction - 3.0f;
      const float h0 = -u1 * u2 * u3 * (1.0f / 6.0f);
      const float h1 = fraction * u2 * u3 * 0.5f;
      const float h2 = -fraction * u1 * u3 * 0.5f;
      const float h3 = fraction * u1 * u2 * (1.0f / 6.0f);

      const float stringSample =
          h0 * tap(voice, base) + h1 * tap(voice, base + 1) +
          h2 * tap(voice, base + 2) + h3 * tap(voice, base + 3);

      mLp[voice] =
          dampingGain * stringSample + dampingPole * mLp[voice];

      const float dc =
          mLp[voice] - mDcX[voice] + kDcBlockerPole * mDcY[voice];
      mDcX[voice] = mLp[voice];
      mDcY[voice] = dc;

      float y = voiceFeedback[voice] * dc;
      if (voice == mCurrent)
        y += in[i];

      if (y > 2.0f)
        y = 2.0f;
      else if (y < -2.0f)
        y = -2.0f;

      mpBuffer[voice][mWriteIndex] = y;
      sum += y;
    }

    mWriteIndex++;
    if (mWriteIndex == mMaxDelayInSamples)
      mWriteIndex = 0;

    out[i] = sum;
  }

  mPreviousTrigger = previousTrigger;
}
