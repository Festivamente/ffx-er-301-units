#pragma once

#include <od/audio/Sample.h>

namespace morpho {
using namespace od;

class Grain {
public:
  Grain();
  virtual ~Grain();

  void setSample(Sample* sample);
  void setRegion(int start, int length);
  void init(int start, int duration, float speed, float gain, float pan);
  void snapToZeroCrossing(int window);
  void stop();

  enum {
    kSineWindow = 0,
    kHanningWindow = 1,
    kTrapezoidWindow = 2
  };

  void setEnvelope(int type);
  void setFade(int fade);
  void setSquash(float squash);
  void setDelay(int samples);
  int samplesRequired();

  inline bool operator<(const Grain& other) const {
    return mCurrentIndex < other.mCurrentIndex;
  }

  Sample* mpSample = 0;
  float mEnvelopePhase = 0.0f;
  float mEnvelopePhaseDelta = 0.0f;
  int mEnvelopeType = kSineWindow;
  int mFade = 0;
  int mCurrentIndex = 0;
  int mDuration = 0;
  int mRemaining = 0;
  int mDelayInSamples = 0;
  float mSquash = 0.0f;

  float mSpeedAdjustment = 1.0f;
  float mPhaseDelta = 0.0f;
  float mPhase = 0.0f;
  float mLeftBalance = 1.0f;
  float mRightBalance = 1.0f;
  float mLastEnvelopeValue = 0.0f;

  bool mActive = false;

protected:
  int wrapIndex(int index) const;

  int mRegionStart = 0;
  int mRegionLength = 0;

  void generateSineWindow(float* out, int n);
  void generateHanningWindow(float* out, int n);
  void generateTrapezoidWindow(float* out, int n);
  void generateEnvelope(float* out, int n);
  void squashEnvelope(float* out, int n);
};

}
