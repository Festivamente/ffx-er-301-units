#pragma once

#include <od/objects/Object.h>

class Flanger : public od::Object {
public:
  Flanger();
  ~Flanger();

#ifndef SWIGLUA
  virtual void process();

  od::Inlet mInput{"In"};
  od::Inlet mInputR{"InR"};
  od::Outlet mOutput{"Out"};
  od::Outlet mOutputR{"OutR"};
  od::Inlet mRate{"Rate"};
  od::Inlet mDepth{"Depth"};
  od::Inlet mFeedback{"Feedback"};
  od::Inlet mBlend{"Blend"};

private:
  float* mBuffer = nullptr;
  float* mBufferR = nullptr;
  int mBufferSize = 0;
  int mWriteIndex = 0;
  float mPhase = 0.0f;
  float mSmoothedDelay = 0.0f;
  float mDelayCoeff = 0.0f;
#endif
};
