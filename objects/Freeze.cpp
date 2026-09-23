#include <Freeze.h>
#include <dsp/CircularBuffer.h>
#include <dsp/DspMath.h>
#include <od/config.h>
#include <od/extras/BigHeap.h>
#include <cmath>

Freeze::Freeze() {
  addInput(mInput);
  addOutput(mOutput);
  addInput(mFreeze);
  addInput(mSize);

  mBufferSize = (int)globalConfig.sampleRate + 8;
  mBuffer = (float*)od::BigHeap::allocateZeroed(
      mBufferSize * sizeof(float));

  mCrossfadeCoeff =
      1.0f - expf(-1.0f / (0.008f * globalConfig.sampleRate));
}

Freeze::~Freeze() {
  if (mBuffer) {
    od::BigHeap::free((char*)mBuffer);
    mBuffer = nullptr;
  }
}

void Freeze::process() {
  const float* in = mInput.buffer();
  float* out = mOutput.buffer();
  const float* freeze = mFreeze.buffer();
  const float* loopSize = mSize.buffer();

  if (!mBuffer) {
    for (int i = 0; i < FRAMELENGTH; i++)
      out[i] = in[i];
    return;
  }

  const float sampleRate = globalConfig.sampleRate;
  const int bufferSize = mBufferSize;
  const float maxLoop = (float)(bufferSize - 8);

  int writeIndex = mWriteIndex;
  float loopPhase = mLoopPhase;
  float crossfade = mCrossfade;

  for (int i = 0; i < FRAMELENGTH; i++) {
    const float dry = in[i];
    const bool gateHigh = freeze[i] > 0.5f;

    if (gateHigh && !mFrozen) {
      mFrozen = true;
      mFreezeIndex = writeIndex;
      loopPhase = 0.0f;
    } else if (!gateHigh && mFrozen) {
      mFrozen = false;
    }

    if (!mFrozen) {
      mBuffer[writeIndex] = dry;
      writeIndex++;
      if (writeIndex >= bufferSize)
        writeIndex = 0;
    }

    const float loopLength =
        timefx::clamp(loopSize[i] * sampleRate, 128.0f, maxLoop);
    const float phaseIncrement = 1.0f / loopLength;

    loopPhase += phaseIncrement;
    loopPhase -= floorf(loopPhase);

    float secondPhase = loopPhase + 0.5f;
    if (secondPhase >= 1.0f)
      secondPhase -= 1.0f;

    const float readPosition1 =
        (float)mFreezeIndex - loopLength + loopPhase * loopLength;
    const float readPosition2 =
        (float)mFreezeIndex - loopLength + secondPhase * loopLength;

    const float sample1 =
        timefx::readLinearAt(mBuffer, bufferSize, readPosition1);
    const float sample2 =
        timefx::readLinearAt(mBuffer, bufferSize, readPosition2);
    const float frozen = timefx::sinPi01(loopPhase) * sample1 +
                         timefx::sinPi01(secondPhase) * sample2;

    const float targetCrossfade = mFrozen ? 1.0f : 0.0f;
    crossfade += mCrossfadeCoeff * (targetCrossfade - crossfade);
    out[i] = dry + crossfade * (frozen - dry);
  }

  mWriteIndex = writeIndex;
  mLoopPhase = loopPhase;
  mCrossfade = crossfade;
}
