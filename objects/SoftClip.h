#pragma once

#include <od/objects/Object.h>

#define SC_TANH 4
#define SC_SIN 5
#define SC_EXP 6
#define SC_2STG 7
#define SC_CUBIC 8
#define SC_RECIP 9

namespace od {

class SoftClip : public Object {
public:
  SoftClip();
  ~SoftClip();

#ifndef SWIGLUA
  virtual void process();
  Inlet mInput{"In"};
  Outlet mOutput{"Out"};
  Inlet mGain{"Gain"};
  Option mAlgo{"Algo", SC_TANH};
#endif
};

}
