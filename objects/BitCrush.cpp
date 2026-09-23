#include <BitCrush.h>
#include <od/config.h>
#include <cmath>

namespace od {

namespace {

inline float quantizeSample(float sample, int bitDepth) {
  if (bitDepth < 1) bitDepth = 1;
  if (bitDepth > 32) bitDepth = 32;

  const float scale = std::ldexp(1.0f, bitDepth - 1);
  return std::round(sample * scale) / scale;
}

}

BitCrush::BitCrush() {
  addInput(mInput);
  addOutput(mOutput);
  addInput(mBitDepth);
  addInput(mSampleRate);

  mLastSample = 0.0f;
  mCounter = 0;
}

BitCrush::~BitCrush() {}

void BitCrush::process() {
  float* in = mInput.buffer();
  float* out = mOutput.buffer();
  float* depth = mBitDepth.buffer();
  float* sr = mSampleRate.buffer();

  for (int i = 0; i < FRAMELENGTH; i++) {
    const int bitDepth = (int)depth[i];
    int holdSamples = (int)sr[i];

    if (holdSamples < 1) holdSamples = 1;

    if (mCounter == 0)
      mLastSample = quantizeSample(in[i], bitDepth);

    out[i] = mLastSample;

    mCounter++;
    if (mCounter >= holdSamples)
      mCounter = 0;
  }
}

}
