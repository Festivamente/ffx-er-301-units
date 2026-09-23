#pragma once

#include <od/objects/Object.h>

class Cymbal : public od::Object
{
public:
  Cymbal();
  ~Cymbal();

#ifndef SWIGLUA
  virtual void process();
  od::Inlet mTrig{"Trig"};
  od::Outlet mOutput{"Out"};
  od::Inlet mTune{"Tune"};
  od::Inlet mDecay{"Decay"};
  od::Inlet mTone{"Tone"};
  od::Inlet mSizzle{"Sizzle"};

private:
  float mPhase[6];
  float mStrikeEnv;
  float mBodyEnv;

  float mHp1;
  float mHp2;

  float mRes1;
  float mRes2;

  float mNoiseHpLp;
  bool mPrevTrig;
  unsigned int mSeed;
  float mStrikeCoeff;
#endif
};
