#pragma once

#include <od/objects/Object.h>

class Tom : public od::Object
{
public:
  Tom();
  ~Tom();

#ifndef SWIGLUA
  virtual void process();
  od::Inlet mTrig{"Trig"};
  od::Outlet mOutput{"Out"};
  od::Inlet mTune{"Tune"};
  od::Inlet mDecay{"Decay"};
  od::Inlet mBend{"Bend"};

private:
  float mPhase = 0.0f;
  float mAmpEnv = 0.0f;
  float mPitchEnv = 0.0f;
  float mStrikeEnv = 0.0f;
  bool mPrevTrig = false;
  unsigned int mSeed = 0xD00D5EEDu;
  float mPitchCoeff = 0.0f;
  float mStrikeCoeff = 0.0f;
#endif
};
