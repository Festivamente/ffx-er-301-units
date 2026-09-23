#pragma once

#include <od/objects/Object.h>

class Exciter : public od::Object {
public:
  Exciter();
  ~Exciter();

#ifndef SWIGLUA
  virtual void process();
  od::Inlet mInput{"In"};
  od::Outlet mOutput{"Out"};
  od::Inlet mAxMix{"AxMix"};
  od::Option mMode{"Mode", 4};

private:
  float mHpLp = 0.0f;
  float mDcLp = 0.0f;
  float mHpCoeff = 0.0f;
  float mDcCoeff = 0.0f;
#endif
};
