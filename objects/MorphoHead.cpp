#include "MorphoHead.h"
#include <od/config.h>
#include <hal/ops.h>
#include <math.h>
#include <string.h>

namespace morpho {

MorphoHead::MorphoHead(int channelCount) : mOutputChannelCount(channelCount) {
  addInput(mSpeed);
  addInput(mTrigger);
  addOutput(mLeftOutput);
  addOutput(mRightOutput);
  addParameter(mGeneSize);
  addParameter(mSlide);
  addParameter(mOrganize);
  addParameter(mMorph);
  addParameter(mGain);
  addParameter(mSquash);
  addParameter(mSplices);

  mFreeStereoCount = 0;
  mFreeMonoCount = 0;
  for (int i = kMaxGrains - 1; i >= 0; i--) {
    mFreeStereo[mFreeStereoCount++] = &mStereoGrains[i];
    mFreeMono[mFreeMonoCount++] = &mMonoGrains[i];
  }

}

MorphoHead::~MorphoHead() {
  setSample(0);
}

void MorphoHead::stopAllGrains() {
  mFreeMonoCount = 0;
  for (int i = kMaxGrains - 1; i >= 0; i--) {
    if (mMonoGrains[i].mActive) {
      mMonoGrains[i].stop();
    }
    mFreeMono[mFreeMonoCount++] = &mMonoGrains[i];
  }

  mFreeStereoCount = 0;
  for (int i = kMaxGrains - 1; i >= 0; i--) {
    if (mStereoGrains[i].mActive) {
      mStereoGrains[i].stop();
    }
    mFreeStereo[mFreeStereoCount++] = &mStereoGrains[i];
  }
}

void MorphoHead::setSample(od::Sample* sample) {
  mEnabled = false;
  stopAllGrains();

  Base::setSample(sample);

  if (mpSample) {
    if (mOutputChannelCount == 2 && mpSample->mChannelCount == 2) {
      for (int i = 0; i < kMaxGrains; i++) {
        mMonoGrains[i].setSample(0);
        mStereoGrains[i].setSample(mpSample);
      }
    } else {
      for (int i = 0; i < kMaxGrains; i++) {
        mMonoGrains[i].setSample(mpSample);
        mStereoGrains[i].setSample(0);
      }
    }

    mCountdown = 0;
    mLastTriggerHigh = false;
    mShadowPanFlip = false;
    mEnabled = true;
  } else {
    mCountdown = 0;
    mLastTriggerHigh = false;
    mShadowPanFlip = false;
    for (int i = 0; i < kMaxGrains; i++) {
      mMonoGrains[i].setSample(0);
      mStereoGrains[i].setSample(0);
    }
  }
}

StereoGrain* MorphoHead::getNextFreeStereoGrain() {
  if (mEnabled && mFreeStereoCount > 0) {
    return mFreeStereo[--mFreeStereoCount];
  }
  return 0;
}

MonoGrain* MorphoHead::getNextFreeMonoGrain() {
  if (mEnabled && mFreeMonoCount > 0) {
    return mFreeMono[--mFreeMonoCount];
  }
  return 0;
}

inline float MorphoHead::nextRandom() {

  mSeed ^= mSeed << 13;
  mSeed ^= mSeed >> 17;
  mSeed ^= mSeed << 5;
  return (mSeed >> 8) * (1.0f / 16777216.0f);
}

void MorphoHead::spawnGeneSet(int delay, float speed, int start,
                              int duration, int fade, float morph,
                              int regionStart, int regionLength,
                              bool useStereo) {
  const float squash = mSquash.value();
  const float firstShadow = CLAMP(0.0f, 1.0f, (morph - 0.45f) * 1.9f);
  const float secondShadow = CLAMP(0.0f, 1.0f, (morph - 0.8f) * 5.0f);

  const float firstWeight = 0.6f * firstShadow;
  const float secondWeight = 0.4f * secondShadow;
  const float layerNorm =
      1.0f / sqrtf(1.0f + firstWeight * firstWeight +
                   secondWeight * secondWeight);
  const float mainGain = mGain.value() * layerNorm;

  if (useStereo) {
    StereoGrain* grain = getNextFreeStereoGrain();
    if (grain) {
      grain->setRegion(regionStart, regionLength);
      grain->init(start, duration, speed, mainGain, 0.0f);
      grain->setEnvelope(Grain::kTrapezoidWindow);
      grain->setFade(fade);
      grain->setSquash(squash);
      grain->setDelay(delay);
    }
  } else {
    MonoGrain* grain = getNextFreeMonoGrain();
    if (grain) {
      grain->setRegion(regionStart, regionLength);
      grain->init(start, duration, speed, mainGain, 0.0f);
      grain->setEnvelope(Grain::kTrapezoidWindow);
      grain->setFade(fade);
      grain->setSquash(squash);
      grain->setDelay(delay);
    }
  }

  if (firstShadow > 0.001f) {
    int scatter = (int)(nextRandom() * 0.25f * morph * duration);
    float pan = mShadowPanFlip ? (0.4f * morph) : (-0.4f * morph);
    if (useStereo) {
      StereoGrain* grain = getNextFreeStereoGrain();
      if (grain) {
        grain->setRegion(regionStart, regionLength);
        grain->init(start + scatter, duration, 2.0f * speed,
                    mainGain * firstWeight, pan);
        grain->setEnvelope(Grain::kSineWindow);
        grain->setSquash(squash);
        grain->setDelay(delay);
      }
    } else {
      MonoGrain* grain = getNextFreeMonoGrain();
      if (grain) {
        grain->setRegion(regionStart, regionLength);
        grain->init(start + scatter, duration, 2.0f * speed,
                    mainGain * firstWeight, pan);
        grain->setEnvelope(Grain::kSineWindow);
        grain->setSquash(squash);
        grain->setDelay(delay);
      }
    }
  }

  if (secondShadow > 0.001f) {
    int scatter = (int)(nextRandom() * 0.5f * duration);
    float pan = mShadowPanFlip ? -0.5f : 0.5f;
    if (useStereo) {
      StereoGrain* grain = getNextFreeStereoGrain();
      if (grain) {
        grain->setRegion(regionStart, regionLength);
        grain->init(start + scatter, duration, 4.0f * speed,
                    mainGain * secondWeight, pan);
        grain->setEnvelope(Grain::kSineWindow);
        grain->setSquash(squash);
        grain->setDelay(delay);
      }
    } else {
      MonoGrain* grain = getNextFreeMonoGrain();
      if (grain) {
        grain->setRegion(regionStart, regionLength);
        grain->init(start + scatter, duration, 4.0f * speed,
                    mainGain * secondWeight, pan);
        grain->setEnvelope(Grain::kSineWindow);
        grain->setSquash(squash);
        grain->setDelay(delay);
      }
    }
  }

  mShadowPanFlip = !mShadowPanFlip;
}

void MorphoHead::process() {
  float* left = mLeftOutput.buffer();
  float* right = mRightOutput.buffer();
  memset(left, 0, globalConfig.frameLength * sizeof(float));
  if (mOutputChannelCount > 1) {
    memset(right, 0, globalConfig.frameLength * sizeof(float));
  }

  if (mpSample == 0 || !mEnabled || mpSample->mSampleCount < 64) {
    return;
  }

  bool useStereo =
      (mOutputChannelCount == 2 && mpSample->mChannelCount == 2);

  const int sampleCount = (int)mpSample->mSampleCount;
  int spliceCount = (int)(mSplices.value() + 0.5f);
  spliceCount = CLAMP(1, 32, spliceCount);
  spliceCount = MIN(spliceCount, MAX(1, sampleCount / 64));

  const float organize = CLAMP(0.0f, 1.0f, mOrganize.value());
  int spliceIndex = (int)(organize * spliceCount);
  spliceIndex = MIN(spliceCount - 1, spliceIndex);

  const int spliceStart = (spliceIndex * sampleCount) / spliceCount;
  const int spliceEnd = ((spliceIndex + 1) * sampleCount) / spliceCount;
  const int spliceLength = spliceEnd - spliceStart;

  const float slide = CLAMP(0.0f, 1.0f, mSlide.value());
  const int geneStart =
      spliceStart + (int)(slide * (float)(spliceLength - 1));

  mCurrentIndex = geneStart;

  float geneSize = mGeneSize.value();
  int duration;
  if (geneSize < 0.001f) {

    duration = spliceLength;
  } else {
    duration = (int)(geneSize * globalConfig.sampleRate);
    const int minimumDuration = MIN(256, spliceLength);
    duration = CLAMP(minimumDuration, spliceLength, duration);
  }

  float morph = CLAMP(0.0f, 1.0f, mMorph.value());

  int fade = (int)((0.05f + 0.4f * morph) * duration);
  fade = MAX(32, fade);
  int period = duration - fade;
  period = MAX(64, period);

  const float* trigger = mTrigger.buffer();
  const float* speed = mSpeed.buffer();

  for (int i = 0; i < FRAMELENGTH; i++) {
    const bool high = trigger[i] > 0.0f;
    if (high && !mLastTriggerHigh) {

      mCountdown = 0;
    }
    mLastTriggerHigh = high;

    if (mCountdown <= 0) {
      spawnGeneSet(i, speed[i], geneStart, duration, fade, morph,
                   spliceStart, spliceLength, useStereo);
      mCountdown = period;
    }
    mCountdown--;
  }

  if (useStereo) {
    renderStereoGrains();
  } else {
    renderMonoGrains();
  }
}

void MorphoHead::renderMonoGrains() {

  int activeCount = 0;
  for (int i = 0; i < kMaxGrains; i++) {
    MonoGrain* grain = &mMonoGrains[i];
    if (grain->mActive) {
      int j = activeCount++;
      while (j > 0 && mActiveMono[j - 1]->mCurrentIndex > grain->mCurrentIndex) {
        mActiveMono[j] = mActiveMono[j - 1];
        j--;
      }
      mActiveMono[j] = grain;
    }
  }

  float* left = mLeftOutput.buffer();
  switch (mpSample->mChannelCount) {
  case 1:
    if (mOutputChannelCount == 2) {
      float* right = mRightOutput.buffer();
      for (int i = 0; i < activeCount; i++) {
        MonoGrain* grain = mActiveMono[i];
        grain->synthesizeFromMonoToStereo(left, right);
        if (!grain->mActive) {
          mFreeMono[mFreeMonoCount++] = grain;
        }
      }
    } else {
      for (int i = 0; i < activeCount; i++) {
        MonoGrain* grain = mActiveMono[i];
        grain->synthesizeFromMonoToMono(left);
        if (!grain->mActive) {
          mFreeMono[mFreeMonoCount++] = grain;
        }
      }
    }
    break;
  case 2:
    for (int i = 0; i < activeCount; i++) {
      MonoGrain* grain = mActiveMono[i];
      grain->synthesizeFromStereo(left);
      if (!grain->mActive) {
        mFreeMono[mFreeMonoCount++] = grain;
      }
    }
    break;
  }
}

void MorphoHead::renderStereoGrains() {

  int activeCount = 0;
  for (int i = 0; i < kMaxGrains; i++) {
    StereoGrain* grain = &mStereoGrains[i];
    if (grain->mActive) {
      int j = activeCount++;
      while (j > 0 &&
             mActiveStereo[j - 1]->mCurrentIndex > grain->mCurrentIndex) {
        mActiveStereo[j] = mActiveStereo[j - 1];
        j--;
      }
      mActiveStereo[j] = grain;
    }
  }

  float* left = mLeftOutput.buffer();
  float* right = mRightOutput.buffer();
  for (int i = 0; i < activeCount; i++) {
    StereoGrain* grain = mActiveStereo[i];
    grain->synthesize(left, right);
    if (!grain->mActive) {
      mFreeStereo[mFreeStereoCount++] = grain;
    }
  }
}

}
