#include <objects/Cymbal.h>
#include <objects/drum_common.h>
#include <hal/ops.h>
#include <cmath>

#define CYM_OSCS 6

static const float kCymBaseFrequency = 373.0f;
static const float kCymFreqRatios[CYM_OSCS] =
    {1.0f,
     571.0f / 373.0f,
     613.0f / 373.0f,
     691.0f / 373.0f,
     829.0f / 373.0f,
     1187.0f / 373.0f};

static const float kCymPhases[CYM_OSCS] =
    {0.05f, 0.22f, 0.39f, 0.51f, 0.68f, 0.9f};

namespace
{
  inline float finiteOr(float x, float fallback)
  {
    return (x == x) ? x : fallback;
  }

  inline float safeState(float x)
  {
    x = finiteOr(x, 0.0f);
    return (x > -20.0f && x < 20.0f) ? x : 0.0f;
  }
}

Cymbal::Cymbal()
{
  addInput(mTrig);
  addOutput(mOutput);
  addInput(mTune);
  addInput(mDecay);
  addInput(mTone);
  addInput(mSizzle);

  for (int k = 0; k < CYM_OSCS; k++)
  {
    mPhase[k] = kCymPhases[k];
  }
  mStrikeEnv = 0.0f;
  mBodyEnv = 0.0f;
  mHp1 = 0.0f;
  mHp2 = 0.0f;
  mRes1 = 0.0f;
  mRes2 = 0.0f;
  mNoiseHpLp = 0.0f;
  mPrevTrig = false;
  mSeed = 0xC1CADA5u;

  mStrikeCoeff = decayCoeff(0.08f);
}

Cymbal::~Cymbal()
{
}

void Cymbal::process()
{
  float *trig = mTrig.buffer();
  float *frequency = mTune.buffer();
  float *out = mOutput.buffer();

  const float sampleRate = clampf(finiteOr(globalConfig.sampleRate, 48000.0f),
                                  8000.0f, 384000.0f);
  const float invSr = 1.0f / sampleRate;
  const float maxFrequency = 0.45f * sampleRate;

  float decay = clampf(finiteOr(mDecay.buffer()[0], 1.5f), 0.2f, 5.0f);
  float tone = clampf(finiteOr(mTone.buffer()[0], 5000.0f), 1500.0f, 8000.0f);
  float sizzle = clampf(finiteOr(mSizzle.buffer()[0], 0.4f), 0.0f, 1.0f);

  float bodyCoeff = finiteOr(decayCoeff(decay), 0.0f);
  bodyCoeff = clampf(bodyCoeff, 0.0f, 1.0f);

  float strikeCutoff = clampf(6500.0f, 0.0f, maxFrequency);
  float hpCoeff = 1.0f - expf(-6.2831853f * strikeCutoff / sampleRate);
  hpCoeff = clampf(finiteOr(hpCoeff, 0.0f), 0.0f, 1.0f);

  tone = clampf(tone, 0.0f, maxFrequency);
  const float r = 0.99f;
  const float r2 = r * r;
  float theta = 6.2831853f * tone * invSr;
  float c = 2.0f * r * cosf(theta);
  c = clampf(finiteOr(c, 0.0f), -2.0f * r, 2.0f * r);
  const float norm = (1.0f - r) * 1.5f;

  mStrikeEnv = clampf(finiteOr(mStrikeEnv, 0.0f), 0.0f, 1.0f);
  mBodyEnv = clampf(finiteOr(mBodyEnv, 0.0f), 0.0f, 1.0f);
  mHp1 = safeState(mHp1);
  mHp2 = safeState(mHp2);
  mRes1 = safeState(mRes1);
  mRes2 = safeState(mRes2);
  mNoiseHpLp = safeState(mNoiseHpLp);
  mStrikeCoeff = clampf(finiteOr(mStrikeCoeff, decayCoeff(0.08f)), 0.0f, 1.0f);
  for (int k = 0; k < CYM_OSCS; k++)
  {
    mPhase[k] = finiteOr(mPhase[k], kCymPhases[k]);
  }

  for (int i = 0; i < FRAMELENGTH; i++)
  {
    bool high = trig[i] > 0.5f;
    if (high && !mPrevTrig)
    {
      mStrikeEnv = 1.0f;
      mBodyEnv = 1.0f;
    }
    mPrevTrig = high;

    mStrikeEnv *= mStrikeCoeff;
    mBodyEnv *= bodyCoeff;

    float baseFrequency = finiteOr(frequency[i], kCymBaseFrequency);
    baseFrequency = clampf(baseFrequency, 0.0f, maxFrequency);

    float sq[CYM_OSCS];
    for (int k = 0; k < CYM_OSCS; k++)
    {
      float f = clampf(baseFrequency * kCymFreqRatios[k],
                       0.0f, maxFrequency);
      mPhase[k] += f * invSr;
      if (mPhase[k] >= 1.0f)
      {
        mPhase[k] -= floorf(mPhase[k]);
      }
      sq[k] = (mPhase[k] < 0.5f) ? 1.0f : -1.0f;
    }

    float ring = (sq[0] * sq[3] + sq[1] * sq[4] + sq[2] * sq[5]) * 0.3333f;
    float sum = (sq[0] + sq[1] + sq[2] + sq[3] + sq[4] + sq[5]) * 0.1667f;
    float metal = 0.65f * ring + 0.35f * sum;

    mHp1 += hpCoeff * (metal - mHp1);
    float h = metal - mHp1;
    mHp2 += hpCoeff * (h - mHp2);
    float strike = (h - mHp2) * mStrikeEnv;

    float y = metal + c * mRes1 - r2 * mRes2;
    y = safeState(y);
    mRes2 = mRes1;
    mRes1 = y;
    float bodyBand = norm * y * mBodyEnv;

    float n = lcgNoise(mSeed);
    mNoiseHpLp += hpCoeff * (n - mNoiseHpLp);
    float wash = (n - mNoiseHpLp) * mBodyEnv;

    float mix = 1.1f * strike + 1.2f * bodyBand + 0.4f * sizzle * wash;
    out[i] = softclip(finiteOr(mix, 0.0f));
  }
}
