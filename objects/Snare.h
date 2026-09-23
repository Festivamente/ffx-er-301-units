#pragma once

#include <od/objects/Object.h>

class Snare : public od::Object
{
public:
  Snare();
  ~Snare();

#ifndef SWIGLUA
  virtual void process();
  od::Inlet mTrig{"Trig"};
  od::Outlet mOutput{"Out"};
  od::Inlet mTune{"Tune"};
  od::Inlet mDecay{"Decay"};
  od::Inlet mSnap{"Snap"};
  od::Inlet mTone{"Tone"};

private:
  float mPhase1;
  float mPhase2;
  float mBodyEnv;
  float mNoiseEnv;
  float mHpLp;
  bool mPrevTrig;
  unsigned int mSeed;
#endif
};
