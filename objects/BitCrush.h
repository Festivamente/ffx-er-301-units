#pragma once

#include <od/objects/Object.h>

namespace od {

class BitCrush : public Object {
public:
  BitCrush();
  ~BitCrush();

#ifndef SWIGLUA
  virtual void process();
  Inlet mInput{"In"};
  Outlet mOutput{"Out"};
  Inlet mBitDepth{"BitDepth"};
  Inlet mSampleRate{"SR"};
  float mLastSample;
  int mCounter;
#endif
};

}
