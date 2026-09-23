#include <objects/HiHat.h>
#include <objects/drum_common.h>
#include <hal/ops.h>

#define HAT_OSCS 6

static const float kHatBaseFrequency = 263.0f;
static const float kHatFreqRatios[HAT_OSCS] =
    {1.0f,
     400.0f / 263.0f,
     421.0f / 263.0f,
     474.0f / 263.0f,
     587.0f / 263.0f,
     845.0f / 263.0f};

static const float kHatPhases[HAT_OSCS] =
    {0.0f, 0.17f, 0.31f, 0.47f, 0.61f, 0.83f};

namespace
{

  inline float finiteOr(float x, float fallback)
  {
    return (x == x) ? x : fallback;
  }
}

HiHat::HiHat()
{
  addInput(mTrigC);
  addInput(mTrigO);
  addOutput(mOutput);
  addInput(mTune);
  addInput(mDecayC);
  addInput(mDecayO);
  addInput(mTone);

  for (int k = 0; k < HAT_OSCS; k++)
  {
    mPhase[k] = kHatPhases[k];
  }
  mEnv = 0.0f;
  mEnvCoeff = 0.99f;
  mHp1 = 0.0f;
  mHp2 = 0.0f;
  mPrevC = false;
  mPrevO = false;
}

HiHat::~HiHat()
{
}

void HiHat::process()
{
  float *trigC = mTrigC.buffer();
  float *trigO = mTrigO.buffer();
  float *frequency = mTune.buffer();
  float *out = mOutput.buffer();

  float decC = clampf(finiteOr(mDecayC.buffer()[0], 0.04f), 0.005f, 0.3f);
  float decO = clampf(finiteOr(mDecayO.buffer()[0], 0.4f), 0.05f, 2.0f);
  float tone = clampf(finiteOr(mTone.buffer()[0], 7000.0f), 2000.0f, 12000.0f);

  const float sampleRate = globalConfig.sampleRate;
  const float invSr = 1.0f / sampleRate;
  const float maxFrequency = 0.45f * sampleRate;

  float coeffC = decayCoeff(decC);
  float coeffO = decayCoeff(decO);

  tone = clampf(tone, 0.0f, maxFrequency);
  float hpCoeff = 1.0f - expf(-6.2831853f * tone / sampleRate);
  hpCoeff = clampf(finiteOr(hpCoeff, 0.0f), 0.0f, 1.0f);

  mEnv = finiteOr(mEnv, 0.0f);
  mEnvCoeff = finiteOr(mEnvCoeff, coeffC);
  mHp1 = finiteOr(mHp1, 0.0f);
  mHp2 = finiteOr(mHp2, 0.0f);
  for (int k = 0; k < HAT_OSCS; k++)
  {
    mPhase[k] = finiteOr(mPhase[k], kHatPhases[k]);
  }

  for (int i = 0; i < FRAMELENGTH; i++)
  {

    bool c = trigC[i] > 0.5f;
    bool o = trigO[i] > 0.5f;
    if (c && !mPrevC)
    {
      mEnv = 1.0f;
      mEnvCoeff = coeffC;
    }
    if (o && !mPrevO)
    {
      mEnv = 1.0f;
      mEnvCoeff = coeffO;
    }
    mPrevC = c;
    mPrevO = o;

    mEnv *= mEnvCoeff;

    float baseFrequency = finiteOr(frequency[i], kHatBaseFrequency);
    baseFrequency = clampf(baseFrequency, 0.0f, maxFrequency);

    float sum = 0.0f;
    for (int k = 0; k < HAT_OSCS; k++)
    {
      float f = clampf(baseFrequency * kHatFreqRatios[k],
                       0.0f, maxFrequency);
      mPhase[k] += f * invSr;
      if (mPhase[k] >= 1.0f)
      {
        mPhase[k] -= floorf(mPhase[k]);
      }
      sum += (mPhase[k] < 0.5f) ? 1.0f : -1.0f;
    }
    sum *= 0.1667f;

    mHp1 += hpCoeff * (sum - mHp1);
    float h = sum - mHp1;
    mHp2 += hpCoeff * (h - mHp2);
    h = h - mHp2;

    out[i] = 1.5f * h * mEnv * mEnv;
  }
}
