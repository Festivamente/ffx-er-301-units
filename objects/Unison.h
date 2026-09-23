#pragma once

#include <od/objects/Object.h>

class Unison : public od::Object {
public:
  Unison();
  ~Unison();

#ifndef SWIGLUA
  virtual void process();

  od::Inlet mInput{"In"};
  od::Outlet mOutputL{"OutL"};
  od::Outlet mOutputR{"OutR"};
  od::Outlet mOutputMono{"OutMono"};
  od::Inlet mDetune{"Detune"};
  od::Inlet mBlend{"Blend"};
  od::Inlet mSpread{"Spread"};

private:
  enum { kVoiceCount = 4 };

  float* mBuffer = nullptr;
  int mBufferSize = 0;
  int mWriteIndex = 0;

  float mPhase[kVoiceCount] = {0.0f, 0.25f, 0.5f, 0.75f};
  float mWindow = 0.0f;
  float mMaxSpreadDelay = 0.0f;
  float mSmoothedDetune = 0.0f;
  float mSmoothedSpread = 0.0f;
  float mDetuneCoeff = 0.0f;
  float mSpreadCoeff = 0.0f;
#endif
};
