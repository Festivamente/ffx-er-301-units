#include <objects/Tom.h>
#include <objects/drum_common.h>
#include <hal/ops.h>

namespace
{

  inline float finiteOr(float x, float fallback)
  {
    return (x == x) ? x : fallback;
  }
}

Tom::Tom()
{
  addInput(mTrig);
  addOutput(mOutput);
  addInput(mTune);
  addInput(mDecay);
  addInput(mBend);

  mPitchCoeff = decayCoeff(0.12f);
  mStrikeCoeff = decayCoeff(0.008f);
}

Tom::~Tom()
{
}

void Tom::process()
{
  float *trig = mTrig.buffer();
  float *frequency = mTune.buffer();
  float *out = mOutput.buffer();

  float decay = clampf(finiteOr(mDecay.buffer()[0], 0.35f), 0.05f, 2.0f);
  float bend = clampf(finiteOr(mBend.buffer()[0], 0.4f), 0.0f, 1.0f);

  float ampCoeff = decayCoeff(decay);
  const float sampleRate = globalConfig.sampleRate;
  const float invSr = 1.0f / sampleRate;
  const float maxFrequency = 0.45f * sampleRate;

  for (int i = 0; i < FRAMELENGTH; i++)
  {
    bool high = trig[i] > 0.5f;
    if (high && !mPrevTrig)
    {
      mAmpEnv = 1.0f;
      mPitchEnv = 1.0f;
      mStrikeEnv = 1.0f;
      mPhase = 0.0f;
    }
    mPrevTrig = high;

    mAmpEnv *= ampCoeff;
    mPitchEnv *= mPitchCoeff;
    mStrikeEnv *= mStrikeCoeff;

    float baseFrequency = finiteOr(frequency[i], 120.0f);
    baseFrequency = clampf(baseFrequency, 0.0f, maxFrequency);

    float f = baseFrequency * (1.0f + 1.4f * bend * mPitchEnv);
    f = clampf(f, 0.0f, maxFrequency);

    mPhase += f * invSr;
    if (mPhase >= 1.0f)
    {
      mPhase -= floorf(mPhase);
    }

    float body = sineLfo(mPhase) * mAmpEnv;
    float skin = 0.25f * lcgNoise(mSeed) * mStrikeEnv;

    out[i] = softclip(body + skin);
  }
}
