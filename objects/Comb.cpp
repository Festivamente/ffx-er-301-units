#include <Comb.h>
#include <dsp/CircularBuffer.h>
#include <dsp/DspMath.h>
#include <od/config.h>
#include <od/extras/BigHeap.h>
#include <cstring>

Comb::Comb() {
  addInput(mInput);
  addInput(mInputR);
  addOutput(mOutput);
  addOutput(mOutputR);
  addInput(mFrequency);
  addInput(mResonance);
  addOption(mType);

  mBufferSize = (int)(globalConfig.sampleRate / 20.0f) + 4;
  float* block = (float*)od::BigHeap::allocateZeroed(
      2 * mBufferSize * sizeof(float));
  if (block) {
    mBuffer = block;
    mBufferR = block + mBufferSize;
  }

  mMinFrequency = globalConfig.sampleRate / (float)(mBufferSize - 3);
}

Comb::~Comb() {
  if (mBuffer) {
    od::BigHeap::free((char*)mBuffer);
    mBuffer = nullptr;
    mBufferR = nullptr;
  }
}

void Comb::process() {
  const float* in = mInput.buffer();
  const float* inR = mInputR.buffer();
  float* out = mOutput.buffer();
  float* outR = mOutputR.buffer();
  const float* frequency = mFrequency.buffer();
  const float* resonance = mResonance.buffer();

  if (!mBuffer) {
    std::memcpy(out, in, sizeof(float) * FRAMELENGTH);
    std::memcpy(outR, inR, sizeof(float) * FRAMELENGTH);
    return;
  }

  const float sampleRate = globalConfig.sampleRate;
  const float maxDelay = (float)(mBufferSize - 3);
  const float maxFrequency = 0.45f * sampleRate;
  const float feedbackSign = mType.value() == NEG ? -1.0f : 1.0f;

  int writeIndex = mWriteIndex;

  for (int i = 0; i < FRAMELENGTH; i++) {
    const float frequencyHz =
        timefx::clamp(frequency[i], mMinFrequency, maxFrequency);
    const float delay =
        timefx::clamp(sampleRate / frequencyHz, 2.0f, maxDelay);
    const float feedback =
        feedbackSign * timefx::clamp(resonance[i], 0.0f, 0.98f);

    const float delayed =
        timefx::readLinearDelay(mBuffer, mBufferSize, writeIndex, delay);
    const float delayedR =
        timefx::readLinearDelay(mBufferR, mBufferSize, writeIndex, delay);

    float y = in[i] + feedback * delayed;
    float yR = inR[i] + feedback * delayedR;
    y = timefx::clamp(y, -4.0f, 4.0f);
    yR = timefx::clamp(yR, -4.0f, 4.0f);

    mBuffer[writeIndex] = y;
    mBufferR[writeIndex] = yR;
    out[i] = y;
    outR[i] = yR;

    writeIndex++;
    if (writeIndex >= mBufferSize)
      writeIndex = 0;
  }

  mWriteIndex = writeIndex;
}
