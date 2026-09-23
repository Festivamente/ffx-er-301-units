#pragma once

#include <od/objects/Object.h>

class Kick : public od::Object
{
public:
  Kick();
  ~Kick();

#ifndef SWIGLUA
  virtual void process();
  od::Inlet mTrig{"Trig"};
  od::Outlet mOutput{"Out"};
  od::Inlet mTune{"Tune"};
  od::Inlet mDecay{"Decay"};
  od::Inlet mPunch{"Punch"};
  od::Inlet mClick{"Click"};
  od::Inlet mDrive{"Drive"};

private:
  float mPhase = 0.0f;
  float mAmpEnv = 0.0f;
  float mPitchEnv = 0.0f;
  float mClickEnv = 0.0f;
  bool mPrevTrig = false;
  unsigned int mSeed = 0x1234567u;
  float mPitchCoeff = 0.0f;
  float mClickCoeff = 0.0f;
#endif
};
