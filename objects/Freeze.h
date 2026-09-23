#pragma once

#include <od/objects/Object.h>

class Freeze : public od::Object {
public:
  Freeze();
  ~Freeze();

#ifndef SWIGLUA
  virtual void process();

  od::Inlet mInput{"In"};
  od::Outlet mOutput{"Out"};
  od::Inlet mFreeze{"Freeze"};
  od::Inlet mSize{"Size"};

private:
  float* mBuffer = nullptr;
  int mBufferSize = 0;
  int mWriteIndex = 0;

  bool mFrozen = false;
  int mFreezeIndex = 0;
  float mLoopPhase = 0.0f;
  float mCrossfade = 0.0f;
  float mCrossfadeCoeff = 0.0f;
#endif
};
