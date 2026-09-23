#include <Unison.h>
#include <dsp/DspMath.h>
#include <dsp/DualGrainPitch.h>
#include <od/config.h>
#include <od/extras/BigHeap.h>
#include <cmath>

Unison::Unison() {
  addInput(mInput);
  addOutput(mOutputL);
  addOutput(mOutputR);
  addOutput(mOutputMono);
  addInput(mDetune);
  addInput(mBlend);
  addInput(mSpread);

  mWindow = 0.025f * globalConfig.sampleRate;

  mMaxSpreadDelay = 0.010f * globalConfig.sampleRate;
  mBufferSize = (int)(mWindow + mMaxSpreadDelay) + 8;
  mBuffer = (float*)od::BigHeap::allocateZeroed(
      mBufferSize * sizeof(float));

  mDetuneCoeff = 1.0f - expf(-1.0f / (0.020f * globalConfig.sampleRate));
  mSpreadCoeff = 1.0f - expf(-1.0f / (0.020f * globalConfig.sampleRate));
}

Unison::~Unison() {
  if (mBuffer) {
    od::BigHeap::free((char*)mBuffer);
    mBuffer = nullptr;
  }
}

void Unison::process() {
  const float* in = mInput.buffer();
  float* outL = mOutputL.buffer();
  float* outR = mOutputR.buffer();
  float* outMono = mOutputMono.buffer();
  const float* detuneInput = mDetune.buffer();
  const float* blend = mBlend.buffer();
  const float* spreadInput = mSpread.buffer();

  if (!mBuffer) {
    for (int i = 0; i < FRAMELENGTH; i++) {
      outL[i] = in[i];
      outR[i] = in[i];
      outMono[i] = in[i];
    }
    return;
  }

  const float window = mWindow;
  const int bufferSize = mBufferSize;

  const float centScale[kVoiceCount] = {1.0f, -1.0f, -0.4f, 0.4f};
  const float panSide[kVoiceCount] = {1.0f, -1.0f, -1.0f, 1.0f};

  const float delayScale[kVoiceCount] = {0.0f, 1.0f, 0.0f, 1.0f};

  const float centsToRatio = 0.00057762265f;

  int writeIndex = mWriteIndex;
  float detune = mSmoothedDetune;
  float smoothedSpread = mSmoothedSpread;

  for (int i = 0; i < FRAMELENGTH; i++) {
    const float dry = in[i];
    mBuffer[writeIndex] = dry;

    const float targetDetune =
        timefx::clamp(detuneInput[i], 0.0f, 100.0f);
    detune += mDetuneCoeff * (targetDetune - detune);

    const float targetSpread =
        timefx::clamp(spreadInput[i], 0.0f, 1.0f);
    smoothedSpread +=
        mSpreadCoeff * (targetSpread - smoothedSpread);

    float wetL = 0.0f;
    float wetR = 0.0f;
    float wetMono = 0.0f;
    const float spread = smoothedSpread;

    for (int voice = 0; voice < kVoiceCount; voice++) {
      const float cents = detune * centScale[voice];
      const float scaledCents = centsToRatio * cents;
      const float ratio =
          1.0f + scaledCents + 0.5f * scaledCents * scaledCents;

      float phase = mPhase[voice];
      const float baseDelay =
          1.0f + spread * delayScale[voice] * mMaxSpreadDelay;
      const float voiceSample = timefx::readDualGrainPitch(
          mBuffer, bufferSize, writeIndex, baseDelay, window, ratio, phase);
      mPhase[voice] = phase;

      const float pan = panSide[voice] * spread;
      const float gainL = 0.25f * (1.0f - pan);
      const float gainR = 0.25f * (1.0f + pan);
      wetL += gainL * voiceSample;
      wetR += gainR * voiceSample;
      wetMono += 0.25f * voiceSample;
    }

    const float mix = timefx::clamp(blend[i], 0.0f, 1.0f);
    const float left = dry + mix * (wetL - dry);
    const float right = dry + mix * (wetR - dry);

    outL[i] = left;
    outR[i] = right;
    outMono[i] = dry + mix * (wetMono - dry);

    writeIndex++;
    if (writeIndex >= bufferSize)
      writeIndex = 0;
  }

  mWriteIndex = writeIndex;
  mSmoothedDetune = detune;
  mSmoothedSpread = smoothedSpread;
}
