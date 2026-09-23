#pragma once

#include <od/objects/Object.h>

class BandSpreader : public od::Object {
public:
  BandSpreader();
  ~BandSpreader();

#ifndef SWIGLUA
  virtual void process();

  od::Inlet mInputL{"InL"};
  od::Inlet mInputR{"InR"};
  od::Outlet mOutputL{"OutL"};
  od::Outlet mOutputR{"OutR"};

  od::Inlet mPan1{"Pan1"};
  od::Inlet mPan2{"Pan2"};
  od::Inlet mPan3{"Pan3"};
  od::Inlet mPan4{"Pan4"};
  od::Inlet mPan5{"Pan5"};
  od::Inlet mPan6{"Pan6"};

  od::Inlet mLevel1{"Level1"};
  od::Inlet mLevel2{"Level2"};
  od::Inlet mLevel3{"Level3"};
  od::Inlet mLevel4{"Level4"};
  od::Inlet mLevel5{"Level5"};
  od::Inlet mLevel6{"Level6"};
#endif

private:
  enum {
    kBandCount = 6,
    kCrossoverCount = 5
  };

  float mLowpassState[kCrossoverCount];
  float mLowpassCoeff[kCrossoverCount];

  float mLeftGain[kBandCount];
  float mRightGain[kBandCount];
  float mControlSmoothingCoeff = 0.0f;
};
