#include <objects/Snare.h>
#include <objects/drum_common.h>
#include <hal/ops.h>

namespace
{

  inline float finiteOr(float x, float fallback)
  {
    return (x == x) ? x : fallback;
  }
}

Snare::Snare()
{
  addInput(mTrig);
  addOutput(mOutput);
  addInput(mTune);
  addInput(mDecay);
  addInput(mSnap);
  addInput(mTone);

  mPhase1 = 0.0f;
  mPhase2 = 0.0f;
  mBodyEnv = 0.0f;
  mNoiseEnv = 0.0f;
  mHpLp = 0.0f;
  mPrevTrig = false;
  mSeed = 0xBEEF001u;
}

Snare::~Snare()
{
}

void Snare::process()
{
  float *trig = mTrig.buffer();
  float *frequency = mTune.buffer();
  float *out = mOutput.buffer();

  float decay = clampf(finiteOr(mDecay.buffer()[0], 0.22f), 0.03f, 1.5f);
  float snap = clampf(finiteOr(mSnap.buffer()[0], 0.6f), 0.0f, 1.0f);
  float tone = clampf(finiteOr(mTone.buffer()[0], 2500.0f), 500.0f, 9000.0f);

  float bodyCoeff = decayCoeff(decay);
  float noiseCoeff = decayCoeff(0.05f + 0.45f * decay);

  const float sampleRate = globalConfig.sampleRate;
  const float invSr = 1.0f / sampleRate;
  const float maxFrequency = 0.45f * sampleRate;

  tone = clampf(tone, 0.0f, maxFrequency);
  float hpCoeff = 1.0f - expf(-6.2831853f * tone / sampleRate);

  for (int i = 0; i < FRAMELENGTH; i++)
  {
    bool high = trig[i] > 0.5f;
    if (high && !mPrevTrig)
    {
      mBodyEnv = 1.0f;
      mNoiseEnv = 1.0f;
      mPhase1 = 0.0f;
      mPhase2 = 0.0f;
    }
    mPrevTrig = high;

    mBodyEnv *= bodyCoeff;
    mNoiseEnv *= noiseCoeff;

    float f1 = finiteOr(frequency[i], 185.0f);
    f1 = clampf(f1, 0.0f, maxFrequency);
    float f2 = clampf(1.83f * f1, 0.0f, maxFrequency);

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

    float body = (sineLfo(mPhase1) + 0.55f * sineLfo(mPhase2)) * mBodyEnv * 0.6f;

    float n = lcgNoise(mSeed);
    mHpLp += hpCoeff * (n - mHpLp);
    float wires = (n - mHpLp) * mNoiseEnv;

    out[i] = softclip(body + 1.6f * snap * wires);
  }
}
