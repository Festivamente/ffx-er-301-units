#pragma once

#include <od/objects/Object.h>

class Lofi : public od::Object {
public:
  Lofi();
  ~Lofi();

#ifndef SWIGLUA
  virtual void process();

  od::Inlet mInput{"In"};
  od::Inlet mDepth{"Depth"};
  od::Inlet mSpeed{"Speed"};
  od::Inlet mBlend{"Blend"};
  od::Outlet mOutput{"Out"};
  od::Option mWaveform{"Waveform", 5};

private:
  float* mBuffer = nullptr;
  int mSize = 0;
  int mWrite = 0;

  float mPhase = 0.0f;

  float mSquareState = -1.0f;
  float mSquareRingPhase = 0.0f;
  float mSquareRingEnvelope = 0.0f;
  float mSquarePreviousTarget = -1.0f;
  float mTriangleState = -1.0f;
  int mPreviousWaveform = 5;

  float mEnv = 0.0f;
  float mCompGain = 1.0f;
  float mAttackCoeff = 0.0f;
  float mReleaseCoeff = 0.0f;
  float mGainSmoothingCoeff = 0.0f;

  float mLoFiLp = 0.0f;
#endif
};
