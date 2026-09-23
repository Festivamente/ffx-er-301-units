#pragma once

#include <od/objects/Object.h>

class Shaper : public od::Object {
public:
  Shaper();
  ~Shaper();

#ifndef SWIGLUA
  virtual void process();

  od::Inlet mInput{"In"};
  od::Outlet mOutput{"Out"};
  od::Inlet mAttack{"Atk"};
  od::Inlet mSustain{"Sus"};

private:
  float mFastEnv = 0.0f;
  float mSlowEnv = 0.0f;
  float mSmoothedGain = 1.0f;

  float mFastAttackCoeff = 0.0f;
  float mFastReleaseCoeff = 0.0f;
  float mSlowAttackCoeff = 0.0f;
  float mSlowReleaseCoeff = 0.0f;
  float mGainSmoothingCoeff = 0.0f;
#endif
};
