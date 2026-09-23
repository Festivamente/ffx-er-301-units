#include <FuzzBox.h>
#include <od/config.h>
#include <hal/ops.h>
#include <math.h>

namespace {
constexpr float twoPi = 6.28318530718f;
constexpr float dcBlockerHz = 38.293f;

inline float softSaturate(float x) {
  if (x > 3.0f)
    return 1.0f;
  if (x < -3.0f)
    return -1.0f;

  const float x2 = x * x;
  return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}
}

FuzzBox::FuzzBox() {
  addInput(mInput);
  addInput(mDrive);
  addInput(mGrit);
  addInput(mTone);
  addOutput(mOutput);
  addParameter(mShape);
  mShape.enableSerialization();

  mDcCoeff = expf(-twoPi * dcBlockerHz * globalConfig.samplePeriod);
}

FuzzBox::~FuzzBox() {}

void FuzzBox::process() {
  const float* input = mInput.buffer();
  float* output = mOutput.buffer();
  const float* drive = mDrive.buffer();
  const float* grit = mGrit.buffer();
  const float* tone = mTone.buffer();

  int shape = (int)mShape.roundTarget();
  shape = CLAMP(SHAPE_SOFT, SHAPE_FUZZ, shape);

  const float toneAmount = CLAMP(0.0f, 1.0f, tone[0]);
  const float cutoffHz = 250.0f * powf(48.0f, toneAmount);
  const float toneCoeff =
      1.0f - expf(-twoPi * cutoffHz * globalConfig.samplePeriod);

  for (int i = 0; i < FRAMELENGTH; i++) {
    const float driveAmount = CLAMP(0.0f, 1.0f, drive[i]);
    const float driveGain = 1.0f + driveAmount * driveAmount * 99.0f;
    const float gritBias = CLAMP(-1.0f, 1.0f, grit[i]) * 0.5f;

    float driven = input[i] * driveGain + gritBias;
    float shaped;

    switch (shape) {
    default:
    case SHAPE_SOFT:
      shaped = softSaturate(driven);
      break;

    case SHAPE_HARD:
      shaped = CLAMP(-1.0f, 1.0f, driven);
      break;

    case SHAPE_FUZZ:
      driven *= 4.0f;
      shaped = softSaturate(driven);
      shaped = CLAMP(-1.0f, 0.7f, shaped) * 1.42857f;
      break;
    }

    const float dcBlocked =
        shaped - mDcInputState + mDcCoeff * mDcOutputState;
    mDcInputState = shaped;
    mDcOutputState = dcBlocked;

    mToneState += toneCoeff * (dcBlocked - mToneState);
    output[i] = mToneState;
  }
}
