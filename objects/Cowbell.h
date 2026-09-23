#pragma once

#include <od/objects/Object.h>

class Cowbell : public od::Object
{
public:
  Cowbell();
  ~Cowbell();

#ifndef SWIGLUA
  virtual void process();
  od::Inlet mTrig{"Trig"};
  od::Outlet mOutput{"Out"};
  od::Inlet mTune{"Tune"};
  od::Inlet mDecay{"Decay"};
  od::Inlet mClank{"Clank"};

private:
  float mPhase1 = 0.0f;
  float mPhase2 = 0.0f;
  float mBodyEnv = 0.0f;
  float mClankEnv = 0.0f;
  float mHpLp = 0.0f;
  float mLp = 0.0f;
  bool mPrevTrig = false;
  float mClankCoeff = 0.0f;
#endif
};
