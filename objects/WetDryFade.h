#pragma once

#include <od/objects/Object.h>

class WetDryFade : public od::Object {
public:
  WetDryFade();
  ~WetDryFade();

#ifndef SWIGLUA
  virtual void process();

  od::Inlet mInputA{"A"};
  od::Inlet mInputB{"B"};
  od::Inlet mFade{"Fade"};
  od::Outlet mOutput{"Out"};

private:
  float mSmoothedFade = 1.0f;
  float mFadeCoeff = 0.0f;
#endif
};
