#pragma once

#include <od/objects/Object.h>

class Grotesque : public od::Object {
public:
  Grotesque();
  ~Grotesque();

#ifndef SWIGLUA
  virtual void process();

  od::Inlet mInput{"In"};
  od::Inlet mHigh{"High"};
  od::Inlet mLow{"Low"};
  od::Inlet mGain{"Gain"};
  od::Inlet mInputLevel{"Input"};
  od::Outlet mOutput{"Out"};
#endif

private:
  float mHighPassState1 = 0.0f;
  float mHighPassState2 = 0.0f;
  float mLowPassState1 = 0.0f;
  float mLowPassState2 = 0.0f;
  float mFeedbackState = 0.0f;
  float mDcInputState = 0.0f;
  float mDcOutputState = 0.0f;
};
