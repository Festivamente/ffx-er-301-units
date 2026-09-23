#include <objects/Kick.h>
#include <objects/drum_common.h>
#include <hal/ops.h>

namespace
{

  inline float finiteOr(float x, float fallback)
  {
    return (x == x) ? x : fallback;
  }
}

Kick::Kick()
{
  addInput(mTrig);
  addOutput(mOutput);
  addInput(mTune);
  addInput(mDecay);
  addInput(mPunch);
  addInput(mClick);
  addInput(mDrive);

  mPitchCoeff = decayCoeff(0.045f);
  mClickCoeff = decayCoeff(0.0012f);
}

Kick::~Kick()
{
}

void Kick::process()
{
  float *trig = mTrig.buffer();
  float *frequency = mTune.buffer();
  float *out = mOutput.buffer();

  float decay = clampf(finiteOr(mDecay.buffer()[0], 0.4f), 0.03f, 3.0f);
  float punch = clampf(finiteOr(mPunch.buffer()[0], 0.5f), 0.0f, 1.0f);
  float click = clampf(finiteOr(mClick.buffer()[0], 0.3f), 0.0f, 1.0f);
  float drive = clampf(finiteOr(mDrive.buffer()[0], 0.25f), 0.0f, 1.0f);

  float ampCoeff = decayCoeff(decay);
  float pre = 1.0f + 4.0f * drive;
  float makeup = 1.0f / (1.0f + drive);
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
      mClickEnv = 1.0f;
      mPhase = 0.0f;
    }
    mPrevTrig = high;

    mAmpEnv *= ampCoeff;
    mPitchEnv *= mPitchCoeff;
    mClickEnv *= mClickCoeff;

    float baseFrequency = finiteOr(frequency[i], 50.0f);
    baseFrequency = clampf(baseFrequency, 0.0f, maxFrequency);

    float pe = mPitchEnv * mPitchEnv;
    float f = baseFrequency * (1.0f + 9.0f * punch * pe);
    f = clampf(f, 0.0f, maxFrequency);

    mPhase += f * invSr;
    if (mPhase >= 1.0f)
    {

      mPhase -= floorf(mPhase);
    }

    float body = sineLfo(mPhase) * mAmpEnv;
    float snap = 0.7f * click * mClickEnv * lcgNoise(mSeed);

    out[i] = softclip((body + snap) * pre) * makeup;
  }
}
