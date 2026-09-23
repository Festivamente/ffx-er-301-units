#pragma once

#include <od/objects/Object.h>

class CompressorObject : public od::Object {
public:
  CompressorObject();
  ~CompressorObject();

#ifndef SWIGLUA
  virtual void process();

  od::Inlet mInput{"In"};
  od::Outlet mOutput{"Out"};
  od::Inlet mSideChain{"SCIn"};
  od::Inlet mThreshold{"Threshold"};
  od::Inlet mRatio{"Ratio"};

private:
  float mSmoothedGain = 1.0f;
  float mGainSmoothingCoeff = 0.0f;
#endif
};
