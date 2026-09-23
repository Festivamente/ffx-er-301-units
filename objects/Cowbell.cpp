#include <objects/Cowbell.h>
#include <objects/drum_common.h>
#include <hal/ops.h>

namespace
{

  inline float finiteOr(float x, float fallback)
  {
    return (x == x) ? x : fallback;
  }
}

Cowbell::Cowbell()
{
  addInput(mTrig);
  addOutput(mOutput);
  addInput(mTune);
  addInput(mDecay);
  addInput(mClank);

  mClankCoeff = decayCoeff(0.008f);
}

Cowbell::~Cowbell()
{
}

void Cowbell::process()
{
  float *trig = mTrig.buffer();
  float *frequency = mTune.buffer();
  float *out = mOutput.buffer();

  float decay = clampf(finiteOr(mDecay.buffer()[0], 0.3f), 0.03f, 1.0f);
  float clank = clampf(finiteOr(mClank.buffer()[0], 0.35f), 0.0f, 1.0f);

  float bodyCoeff = decayCoeff(decay);
  const float sr = globalConfig.sampleRate;
  const float invSr = 1.0f / sr;
  const float maxFrequency = 0.45f * sr;

  const float upperRatio = 800.0f / 540.0f;

  float filterBase = finiteOr(frequency[0], 540.0f);
  filterBase = clampf(filterBase, 0.0f, maxFrequency);
  float tuneScale = filterBase / 540.0f;
  float hpFrequency = clampf(350.0f * tuneScale, 0.0f, maxFrequency);
  float lpFrequency = clampf(1900.0f * tuneScale, 0.0f, maxFrequency);
  float hpCoeff = 1.0f - expf(-6.2831853f * hpFrequency / sr);
  float lpCoeff = 1.0f - expf(-6.2831853f * lpFrequency / sr);

  for (int i = 0; i < FRAMELENGTH; i++)
  {
    bool high = trig[i] > 0.5f;
    if (high && !mPrevTrig)
    {
      mBodyEnv = 1.0f;
      mClankEnv = 1.0f;
      mPhase1 = 0.0f;
      mPhase2 = 0.0f;
    }
    mPrevTrig = high;

    mBodyEnv *= bodyCoeff;
    mClankEnv *= mClankCoeff;

    float f1 = finiteOr(frequency[i], 540.0f);
    f1 = clampf(f1, 0.0f, maxFrequency);
    float f2 = clampf(f1 * upperRatio, 0.0f, maxFrequency);

    mPhase1 += f1 * invSr;
    if (mPhase1 >= 1.0f)
    {
      mPhase1 -= floorf(mPhase1);
    }
    mPhase2 += f2 * invSr;
    if (mPhase2 >= 1.0f)
    {
      mPhase2 -= floorf(mPhase2);
    }

    float sum = ((mPhase1 < 0.5f) ? 1.0f : -1.0f)
              + ((mPhase2 < 0.5f) ? 1.0f : -1.0f);
    sum *= 0.5f;

    mHpLp += hpCoeff * (sum - mHpLp);
    float band = sum - mHpLp;
    mLp += lpCoeff * (band - mLp);
    band = mLp;

    float env = 0.9f * mBodyEnv + 1.8f * clank * mClankEnv;

    out[i] = softclip(1.6f * band * env);
  }
}
