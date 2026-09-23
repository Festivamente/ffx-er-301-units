#include <Leslie.h>
#include <dsp/CircularBuffer.h>
#include <dsp/DspMath.h>
#include <od/config.h>
#include <od/extras/BigHeap.h>
#include <cmath>
#include <cstring>

namespace {

constexpr float kHornSlow = 0.80f;
constexpr float kHornFast = 6.80f;
constexpr float kDrumSlow = 0.66f;
constexpr float kDrumFast = 5.90f;

inline float slewCoeff(float seconds) {
  return 1.0f - expf(-1.0f / (seconds * globalConfig.sampleRate));
}

}

Leslie::Leslie() {
  addInput(mInput);
  addOutput(mOutputL);
  addOutput(mOutputR);
  addInput(mFast);
  addInput(mDepth);
  addInput(mDrive);
  addInput(mBlend);

  mBufferSize = (int)(0.004f * globalConfig.sampleRate) + 4;
  float* block = (float*)od::BigHeap::allocateZeroed(
      2 * mBufferSize * sizeof(float));
  if (block) {
    mHornBuffer = block;
    mDrumBuffer = block + mBufferSize;
  }

  mCrossoverCoeff =
      1.0f - expf(-6.2831853f * 800.0f / globalConfig.sampleRate);

  mHornAccelCoeff = slewCoeff(0.8f);
  mHornDecelCoeff = slewCoeff(1.1f);
  mDrumAccelCoeff = slewCoeff(4.0f);
  mDrumDecelCoeff = slewCoeff(5.0f);
}

Leslie::~Leslie() {
  if (mHornBuffer) {
    od::BigHeap::free((char*)mHornBuffer);
    mHornBuffer = nullptr;
    mDrumBuffer = nullptr;
  }
}

void Leslie::process() {
  const float* in = mInput.buffer();
  float* outL = mOutputL.buffer();
  float* outR = mOutputR.buffer();
  const float* fast = mFast.buffer();
  const float* depth = mDepth.buffer();
  const float* drive = mDrive.buffer();
  const float* blend = mBlend.buffer();

  if (!mHornBuffer) {
    std::memcpy(outL, in, sizeof(float) * FRAMELENGTH);
    std::memcpy(outR, in, sizeof(float) * FRAMELENGTH);
    return;
  }

  const float sampleRate = globalConfig.sampleRate;
  const float inverseSampleRate = 1.0f / sampleRate;

  const float hornBaseDelay = 0.0012f * sampleRate;
  const float hornDoppler = 0.00085f * sampleRate;
  const float drumBaseDelay = 0.0012f * sampleRate;
  const float drumDoppler = 0.00035f * sampleRate;

  int writeIndex = mWriteIndex;
  float crossoverLowpass = mCrossoverLowpass;
  float hornPhase = mHornPhase;
  float drumPhase = mDrumPhase;
  float hornSpeed = mHornSpeed;
  float drumSpeed = mDrumSpeed;

  for (int i = 0; i < FRAMELENGTH; i++) {
    const float dry = in[i];

    const float driveAmount = timefx::clamp(drive[i], 0.0f, 1.0f);
    const float preDrive = 1.0f + 3.0f * driveAmount;
    float driven = dry * preDrive;
    driven = driven * (27.0f + driven * driven) /
             (27.0f + 9.0f * driven * driven);
    driven *= 1.0f / (1.0f + 0.7f * driveAmount);

    crossoverLowpass +=
        mCrossoverCoeff * (driven - crossoverLowpass);
    const float drum = crossoverLowpass;
    const float horn = driven - crossoverLowpass;

    mHornBuffer[writeIndex] = horn;
    mDrumBuffer[writeIndex] = drum;

    const bool fastMode = fast[i] > 0.5f;
    const float hornTarget = fastMode ? kHornFast : kHornSlow;
    const float drumTarget = fastMode ? kDrumFast : kDrumSlow;
    hornSpeed +=
        (hornTarget > hornSpeed ? mHornAccelCoeff : mHornDecelCoeff) *
        (hornTarget - hornSpeed);
    drumSpeed +=
        (drumTarget > drumSpeed ? mDrumAccelCoeff : mDrumDecelCoeff) *
        (drumTarget - drumSpeed);

    hornPhase =
        timefx::wrapUnit(hornPhase + hornSpeed * inverseSampleRate);
    drumPhase =
        timefx::wrapUnit(drumPhase + drumSpeed * inverseSampleRate);

    const float depthAmount = timefx::clamp(depth[i], 0.0f, 1.0f);

    const float hornMotion = timefx::sineCycle(hornPhase);
    const float hornPan =
        timefx::sineCycle(timefx::wrapUnit(hornPhase + 0.25f));
    float hornSignal = timefx::readLinearDelay(
        mHornBuffer, mBufferSize, writeIndex,
        hornBaseDelay + depthAmount * hornDoppler * hornMotion);
    hornSignal *=
        1.0f - (0.45f * depthAmount) * 0.5f * (1.0f + hornMotion);
    const float hornLeft =
        hornSignal * 0.5f * (1.0f + 0.8f * depthAmount * hornPan);
    const float hornRight =
        hornSignal * 0.5f * (1.0f - 0.8f * depthAmount * hornPan);

    const float drumMotion = timefx::sineCycle(drumPhase);
    const float drumPan =
        timefx::sineCycle(timefx::wrapUnit(drumPhase + 0.25f));
    float drumSignal = timefx::readLinearDelay(
        mDrumBuffer, mBufferSize, writeIndex,
        drumBaseDelay + depthAmount * drumDoppler * drumMotion);
    drumSignal *=
        1.0f - (0.25f * depthAmount) * 0.5f * (1.0f + drumMotion);
    const float drumLeft =
        drumSignal * 0.5f * (1.0f + 0.5f * depthAmount * drumPan);
    const float drumRight =
        drumSignal * 0.5f * (1.0f - 0.5f * depthAmount * drumPan);

    const float wetL = 2.0f * (hornLeft + drumLeft);
    const float wetR = 2.0f * (hornRight + drumRight);

    const float mix = timefx::clamp(blend[i], 0.0f, 1.0f);
    outL[i] = dry + mix * (wetL - dry);
    outR[i] = dry + mix * (wetR - dry);

    writeIndex++;
    if (writeIndex >= mBufferSize)
      writeIndex = 0;
  }

  mWriteIndex = writeIndex;
  mCrossoverLowpass = crossoverLowpass;
  mHornPhase = hornPhase;
  mDrumPhase = drumPhase;
  mHornSpeed = hornSpeed;
  mDrumSpeed = drumSpeed;
}
