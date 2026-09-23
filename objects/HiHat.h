#pragma once

#include <od/objects/Object.h>

class HiHat : public od::Object
{
public:
  HiHat();
  ~HiHat();

#ifndef SWIGLUA
  virtual void process();
  od::Inlet mTrigC{"TrigC"};
  od::Inlet mTrigO{"TrigO"};
  od::Outlet mOutput{"Out"};
  od::Inlet mTune{"Tune"};
  od::Inlet mDecayC{"DecayC"};
  od::Inlet mDecayO{"DecayO"};
  od::Inlet mTone{"Tone"};

private:
  float mPhase[6];
  float mEnv;
  float mEnvCoeff;
  float mHp1;
  float mHp2;
  bool mPrevC;
  bool mPrevO;
#endif
};
