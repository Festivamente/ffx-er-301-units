#pragma once

#include <od/objects/Object.h>

class FuzzBox : public od::Object {
public:
  FuzzBox();
  ~FuzzBox();

#ifndef SWIGLUA
  virtual void process();

  od::Inlet mInput{"In"};
  od::Inlet mDrive{"Drive"};
  od::Inlet mGrit{"Grit"};
  od::Inlet mTone{"Tone"};
  od::Outlet mOutput{"Out"};

  od::Parameter mShape{"Shape", 1.0f};
#endif

protected:
  static constexpr int SHAPE_SOFT = 1;
  static constexpr int SHAPE_HARD = 2;
  static constexpr int SHAPE_FUZZ = 3;

  float mToneState = 0.0f;
  float mDcInputState = 0.0f;
  float mDcOutputState = 0.0f;
  float mDcCoeff = 0.995f;
};
