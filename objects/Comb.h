#pragma once

#include <od/objects/Object.h>

#define POS 1
#define NEG 3

class Comb : public od::Object {
public:
  Comb();
  ~Comb();

#ifndef SWIGLUA
  virtual void process();

  od::Inlet mInput{"In"};
  od::Inlet mInputR{"InR"};
  od::Outlet mOutput{"Out"};
  od::Outlet mOutputR{"OutR"};
  od::Inlet mFrequency{"f0"};
  od::Inlet mResonance{"Q"};
  od::Option mType{"Type", POS};

private:
  float* mBuffer = nullptr;
  float* mBufferR = nullptr;
  int mBufferSize = 0;
  int mWriteIndex = 0;
  float mMinFrequency = 20.0f;
#endif
};
