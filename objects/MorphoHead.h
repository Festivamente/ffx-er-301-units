#pragma once

#include <od/objects/heads/Head.h>
#include <atomic>

#ifndef SWIGLUA
#include "Grain.h"
#include "MonoGrain.h"
#include "StereoGrain.h"
#endif

namespace morpho {

class MorphoHead : public od::Head {
public:
  MorphoHead(int channelCount);
  virtual ~MorphoHead();

  virtual void setSample(od::Sample* sample);

#ifndef SWIGLUA
  virtual void process();

  od::Outlet mLeftOutput{"Left Out"};
  od::Outlet mRightOutput{"Right Out"};
  od::Inlet mTrigger{"Trigger"};
  od::Inlet mSpeed{"Speed"};

  od::Parameter mGeneSize{"Gene Size", 0.0f};
  od::Parameter mSlide{"Slide", 0.0f};
  od::Parameter mOrganize{"Organize", 0.0f};
  od::Parameter mMorph{"Morph", 0.0f};
  od::Parameter mGain{"Gain", 1.0f};
  od::Parameter mSquash{"Squash", 1.0f};
  od::Parameter mSplices{"Splices", 8.0f};

protected:
  enum { kMaxGrains = 16 };

  int mOutputChannelCount;

  StereoGrain mStereoGrains[kMaxGrains];
  MonoGrain mMonoGrains[kMaxGrains];

  StereoGrain* mFreeStereo[kMaxGrains];
  MonoGrain* mFreeMono[kMaxGrains];
  int mFreeStereoCount = 0;
  int mFreeMonoCount = 0;

  StereoGrain* mActiveStereo[kMaxGrains];
  MonoGrain* mActiveMono[kMaxGrains];
  int mCountdown = 0;
  bool mLastTriggerHigh = false;
  bool mShadowPanFlip = false;
  uint32_t mSeed = 0x9E3779B9u;

  StereoGrain* getNextFreeStereoGrain();
  MonoGrain* getNextFreeMonoGrain();
  void stopAllGrains();
  inline float nextRandom();

  void spawnGeneSet(int delay, float speed, int start, int duration,
                    int fade, float morph, int regionStart, int regionLength,
                    bool useStereo);
  void renderMonoGrains();
  void renderStereoGrains();

private:
  typedef od::Head Base;
  std::atomic<bool> mEnabled{false};
#endif
};

}
