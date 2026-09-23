#include <Flanger.h>
#include <dsp/CircularBuffer.h>
#include <dsp/DspMath.h>
#include <od/config.h>
#include <od/extras/BigHeap.h>
#include <cmath>
#include <cstring>

Flanger::Flanger() {
  addInput(mInput);
  addInput(mInputR);
  addOutput(mOutput);
  addOutput(mOutputR);
  addInput(mRate);
  addInput(mDepth);
  addInput(mFeedback);
  addInput(mBlend);

  mBufferSize = (int)(0.010f * globalConfig.sampleRate) + 4;
  float* block = (float*)od::BigHeap::allocateZeroed(
      2 * mBufferSize * sizeof(float));
  if (block) {
    mBuffer = block;
    mBufferR = block + mBufferSize;
  }

  mSmoothedDelay = 0.002f * globalConfig.sampleRate;
  mDelayCoeff = 1.0f - expf(-1.0f / (0.002f * globalConfig.sampleRate));
}

Flanger::~Flanger() {
  if (mBuffer) {
    od::BigHeap::free((char*)mBuffer);
    mBuffer = nullptr;
    mBufferR = nullptr;
  }
}

void Flanger::process() {
  const float* in = mInput.buffer();
  const float* inR = mInputR.buffer();
  float* out = mOutput.buffer();
  float* outR = mOutputR.buffer();
  const float* rate = mRate.buffer();
  const float* depth = mDepth.buffer();
  const float* feedback = mFeedback.buffer();
  const float* blend = mBlend.buffer();

  if (!mBuffer) {
    std::memcpy(out, in, sizeof(float) * FRAMELENGTH);
    std::memcpy(outR, inR, sizeof(float) * FRAMELENGTH);
    return;
  }

  const float sampleRate = globalConfig.sampleRate;
  const float inverseSampleRate = 1.0f / sampleRate;
  const float minDelay = 0.0005f * sampleRate;
  const float maxSweep = 0.0075f * sampleRate;

  int writeIndex = mWriteIndex;
  float phase = mPhase;
  float delay = mSmoothedDelay;

  for (int i = 0; i < FRAMELENGTH; i++) {
    const float dry = in[i];
    const float dryR = inR[i];

    const float rateHz = timefx::clamp(rate[i], 0.01f, 10.0f);
    phase += rateHz * inverseSampleRate;
    if (phase >= 1.0f)
      phase -= 1.0f;

    const float lfo = 0.5f * (1.0f + timefx::sineCycle(phase));
    const float depthAmount = timefx::clamp(depth[i], 0.0f, 1.0f);
    const float targetDelay = minDelay + depthAmount * maxSweep * lfo;
    delay += mDelayCoeff * (targetDelay - delay);

    const float delayed =
        timefx::readLinearDelay(mBuffer, mBufferSize, writeIndex, delay);
    const float delayedR =
        timefx::readLinearDelay(mBufferR, mBufferSize, writeIndex, delay);
    const float feedbackAmount = timefx::clamp(feedback[i], -0.95f, 0.95f);
    mBuffer[writeIndex] = dry + feedbackAmount * delayed;
    mBufferR[writeIndex] = dryR + feedbackAmount * delayedR;

    const float wet = 0.7071f * (dry + delayed);
    const float wetR = 0.7071f * (dryR + delayedR);
    const float mix = timefx::clamp(blend[i], 0.0f, 1.0f);
    out[i] = dry + mix * (wet - dry);
    outR[i] = dryR + mix * (wetR - dryR);

    writeIndex++;
    if (writeIndex >= mBufferSize)
      writeIndex = 0;
  }

  mWriteIndex = writeIndex;
  mPhase = phase;
  mSmoothedDelay = delay;
}
