#pragma once

#include <od/objects/Object.h>

class Pluck : public od::Object {
public:
  Pluck(float seconds);
  virtual ~Pluck();

  float allocateTimeUpTo(float seconds);
  void deallocate();
  void zero();

#ifndef SWIGLUA
  virtual void process();

  od::Inlet mInput{"In"};
  od::Inlet mTrigger{"Trigger"};
  od::Inlet mFrequency{"f0"};
  od::Inlet mDamp{"Damp"};
  od::Inlet mDecay{"Decay"};
  od::Outlet mOutput{"Out"};
#endif

private:
  enum { kVoices = 4 };

  float* mpBlock = 0;
  float* mpBuffer[kVoices] = {0, 0, 0, 0};
  int mMaxDelayInSamples = 0;
  int mWriteIndex = 0;

  float mFreq[kVoices] = {110.0f, 110.0f, 110.0f, 110.0f};
  float mLp[kVoices] = {0, 0, 0, 0};
  float mDcX[kVoices] = {0, 0, 0, 0};
  float mDcY[kVoices] = {0, 0, 0, 0};

  int mCurrent = 0;
  int mTrackRemaining = 0;
  float mPreviousTrigger = 0.0f;

  inline float tap(int voice, int samplesAgo) {
    int index = mWriteIndex - samplesAgo;
    if (index < 0)
      index += mMaxDelayInSamples;
    return mpBuffer[voice][index];
  }
};
