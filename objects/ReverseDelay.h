#pragma once

#include <od/objects/Object.h>

class ReverseDelay : public od::Object {
public:
  ReverseDelay(int channelCount);
  virtual ~ReverseDelay();

#ifndef SWIGLUA
  virtual void process();

  od::Inlet mLeftInput{"Left In"};
  od::Inlet mRightInput{"Right In"};
  od::Inlet mFeedback{"Feedback"};
  od::Parameter mDelay{"Delay", 1.0f};
  od::Outlet mLeftOutput{"Left Out"};
  od::Outlet mRightOutput{"Right Out"};
#endif

  float allocateTimeUpTo(float seconds);
  void deallocate();
  void zero();
  float minimumDelayTime();
  float maximumDelayTime();

private:
  struct Line {
    char* mpBuffer = 0;
    int mWriteIndex = 0;
    int mAnchor = 0;
    int mCounter = 0;
    int mSegment = 0;
  };

  Line mLeft;
  Line mRight;

  int mChannelCount;
  int mMaxSamples = 0;
  int mMaxDelaySamples = 0;

  bool allocate(int sampleCount);
  void resetLine(Line& line);
  void pushSamples(Line& line, const float* in, float* out,
                   const float* feedback,
                   int delaySamples, int fadeSamples);
};
