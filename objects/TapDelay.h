#pragma once

#include <od/objects/Object.h>

class TapDelay : public od::Object {
public:
  TapDelay();
  ~TapDelay();

#ifndef SWIGLUA
  virtual void process();

  od::Inlet mInput{"In"};
  od::Outlet mOutput{"Out"};
  od::Outlet mTap1{"Tap1"};
  od::Outlet mTap2{"Tap2"};
  od::Outlet mTap3{"Tap3"};
  od::Outlet mTap4{"Tap4"};
  od::Inlet mTime{"Time"};
  od::Inlet mFeedback{"Feedback"};
  od::Inlet mReverse{"Reverse"};
  od::Inlet mBlend{"Blend"};

  od::Inlet mPitch1{"Pitch1"};
  od::Inlet mPitch2{"Pitch2"};
  od::Inlet mPitch3{"Pitch3"};
  od::Inlet mPitch4{"Pitch4"};

  od::Inlet mLevel1{"Level1"};
  od::Inlet mLevel2{"Level2"};
  od::Inlet mLevel3{"Level3"};
  od::Inlet mLevel4{"Level4"};

private:
  enum { kTapCount = 4 };

  float* mBuffer = nullptr;
  int mBufferSize = 0;
  int mWriteIndex = 0;

  float mPhase[kTapCount] = {0.0f, 0.25f, 0.5f, 0.75f};
  float mSmoothedRatio[kTapCount] = {1.0f, 1.0f, 1.0f, 1.0f};
  float mSpacing = 0.0f;

  float mWindow = 0.0f;
  float mSpacingCoeff = 0.0f;
  float mRatioCoeff = 0.0f;
#endif
};

class TapDelayStereoMixer : public od::Object {
public:
  TapDelayStereoMixer();

#ifndef SWIGLUA
  virtual void process();

  od::Inlet mDryL{"DryL"};
  od::Inlet mDryR{"DryR"};

  od::Inlet mTap1L{"Tap1L"};
  od::Inlet mTap1R{"Tap1R"};
  od::Inlet mTap2L{"Tap2L"};
  od::Inlet mTap2R{"Tap2R"};
  od::Inlet mTap3L{"Tap3L"};
  od::Inlet mTap3R{"Tap3R"};
  od::Inlet mTap4L{"Tap4L"};
  od::Inlet mTap4R{"Tap4R"};

  od::Inlet mLevel1{"Level1"};
  od::Inlet mLevel2{"Level2"};
  od::Inlet mLevel3{"Level3"};
  od::Inlet mLevel4{"Level4"};

  od::Inlet mPan1{"Pan1"};
  od::Inlet mPan2{"Pan2"};
  od::Inlet mPan3{"Pan3"};
  od::Inlet mPan4{"Pan4"};

  od::Inlet mBlend{"Blend"};
  od::Outlet mOutL{"OutL"};
  od::Outlet mOutR{"OutR"};
#endif
};
