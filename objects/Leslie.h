#pragma once

#include <od/objects/Object.h>

class Leslie : public od::Object {
public:
  Leslie();
  ~Leslie();

#ifndef SWIGLUA
  virtual void process();

  od::Inlet mInput{"In"};
  od::Outlet mOutputL{"OutL"};
  od::Outlet mOutputR{"OutR"};
  od::Inlet mFast{"Fast"};
  od::Inlet mDepth{"Depth"};
  od::Inlet mDrive{"Drive"};
  od::Inlet mBlend{"Blend"};

private:
  float* mHornBuffer = nullptr;
  float* mDrumBuffer = nullptr;
  int mBufferSize = 0;
  int mWriteIndex = 0;

  float mCrossoverLowpass = 0.0f;
  float mCrossoverCoeff = 0.0f;

  float mHornPhase = 0.0f;
  float mDrumPhase = 0.3f;
  float mHornSpeed = 0.8f;
  float mDrumSpeed = 0.66f;

  float mHornAccelCoeff = 0.0f;
  float mHornDecelCoeff = 0.0f;
  float mDrumAccelCoeff = 0.0f;
  float mDrumDecelCoeff = 0.0f;
#endif
};
