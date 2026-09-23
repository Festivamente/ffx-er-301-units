#include <PitchShifter.h>
#include <dsp/DspMath.h>
#include <dsp/DualGrainPitch.h>
#include <od/config.h>
#include <od/extras/BigHeap.h>
#include <cmath>

PitchShifter::PitchShifter() {
  addInput(mInput);
  addOutput(mOutput);
  addInput(mBlend);
  addInput(mSpeed1);
  addInput(mSpeed2);
  addInput(mSpeed3);

  const float sampleRate = globalConfig.sampleRate;
  mWindow = 0.040f * sampleRate;

  mSearchSamples = (int)(0.040f * sampleRate);
  mBufferSize = (int)mWindow + mSearchSamples + 8;
  mBuffer = (float*)od::BigHeap::allocateZeroed(
      mBufferSize * sizeof(float));

  mRatioCoeff = 1.0f - expf(-1.0f / (0.003f * sampleRate));

  for (int voice = 0; voice < kMaxVoices; voice++) {
    mPhase0[voice] = 0.0f;
    mPhase1[voice] = 0.5f;
    mCorrection0[voice] = 0.0f;
    mCorrection1[voice] = 0.0f;
    mSmoothedRatio[voice] = 1.0f;
  }
}

PitchShifter::~PitchShifter() {
  if (mBuffer) {
    od::BigHeap::free((char*)mBuffer);
    mBuffer = nullptr;
  }
}

void PitchShifter::setVoiceCount(int voiceCount) {
  if (voiceCount < 1)
    voiceCount = 1;
  else if (voiceCount > kMaxVoices)
    voiceCount = kMaxVoices;

  mVoiceCount = voiceCount;
  mVoiceGain = 1.0f / (float)mVoiceCount;

  for (int voice = 0; voice < kMaxVoices; voice++) {
    mPhase0[voice] = 0.0f;
    mPhase1[voice] = 0.5f;
    mCorrection0[voice] = 0.0f;
    mCorrection1[voice] = 0.0f;
    mSmoothedRatio[voice] = 1.0f;
  }
}

int PitchShifter::getVoiceCount() const {
  return mVoiceCount;
}

void PitchShifter::process() {
  const float* in = mInput.buffer();
  float* out = mOutput.buffer();
  const float* blend = mBlend.buffer();

  const float* speed[kMaxVoices] = {
      mSpeed1.buffer(), mSpeed2.buffer(), mSpeed3.buffer()};

  if (!mBuffer) {
    for (int i = 0; i < FRAMELENGTH; i++)
      out[i] = in[i];
    return;
  }

  const float window = mWindow;
  const int bufferSize = mBufferSize;
  const int voiceCount = mVoiceCount;
  const float voiceGain = mVoiceGain;
  const int searchSamples = mSearchSamples;

  int writeIndex = mWriteIndex;
  int historySamples = mHistorySamples;

  for (int i = 0; i < FRAMELENGTH; i++) {
    const float dry = in[i];
    mBuffer[writeIndex] = dry;

    if (historySamples < bufferSize)
      historySamples++;
    const bool historyReady = historySamples >= bufferSize;

    float wet = 0.0f;

    for (int voice = 0; voice < voiceCount; voice++) {
      const float targetRatio =
          timefx::clamp(speed[voice][i], -4.0f, 4.0f);
      float ratio = mSmoothedRatio[voice];
      ratio += mRatioCoeff * (targetRatio - ratio);
      mSmoothedRatio[voice] = ratio;

      wet += timefx::readPhaseAlignedPitch(
          mBuffer, bufferSize, writeIndex, 1.0f, window,
          searchSamples, historyReady, ratio,
          mPhase0[voice], mPhase1[voice],
          mCorrection0[voice], mCorrection1[voice]);
    }

    wet *= voiceGain;

    const float mix = timefx::clamp(blend[i], 0.0f, 1.0f);
    out[i] = dry + mix * (wet - dry);

    writeIndex++;
    if (writeIndex >= bufferSize)
      writeIndex = 0;
  }

  mWriteIndex = writeIndex;
  mHistorySamples = historySamples;
}
