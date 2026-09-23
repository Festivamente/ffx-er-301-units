#pragma once

#include <od/objects/Object.h>

class PitchShifter : public od::Object {
public:
  PitchShifter();
  ~PitchShifter();

  void setVoiceCount(int voiceCount);
  int getVoiceCount() const;

#ifndef SWIGLUA
  virtual void process();

  od::Inlet mInput{"In"};
  od::Outlet mOutput{"Out"};
  od::Inlet mBlend{"Blend"};

  od::Inlet mSpeed1{"Speed1"};
  od::Inlet mSpeed2{"Speed2"};
  od::Inlet mSpeed3{"Speed3"};

private:
  static constexpr int kMaxVoices = 3;

  int mVoiceCount = 1;
  float mVoiceGain = 1.0f;

  float* mBuffer = nullptr;
  int mBufferSize = 0;
  int mWriteIndex = 0;
  int mHistorySamples = 0;
  int mSearchSamples = 0;

  float mPhase0[kMaxVoices];
  float mPhase1[kMaxVoices];
  float mCorrection0[kMaxVoices];
  float mCorrection1[kMaxVoices];
  float mSmoothedRatio[kMaxVoices];
  float mRatioCoeff = 0.0f;
  float mWindow = 0.0f;
#endif
};
