#include <Lofi.h>
#include <od/config.h>
#include <cmath>

namespace {
constexpr float twoPi = 6.2831853071795864769f;

inline float clampf(float value, float lo, float hi) {
  if (value < lo) return lo;
  if (value > hi) return hi;
  return value;
}

inline float softSaturate(float x) {
  if (x >= 1.5f) return 1.0f;
  if (x <= -1.5f) return -1.0f;
  return x - (4.0f / 27.0f) * x * x * x;
}

inline float sineLfo(float phase) {
  return std::sin(twoPi * phase);
}

inline float triangleLfo(float phase) {
  return 1.0f - 4.0f * std::fabs(phase - 0.5f);
}

inline float speedToHertz(float normalizedSpeed) {
  constexpr float minimumHz = 0.15f;
  constexpr float maximumHz = 5.0f;
  return minimumHz * std::exp(std::log(maximumHz / minimumHz) * normalizedSpeed);
}

inline float smoothDepth(float normalizedDepth) {
  const float x = clampf(normalizedDepth, 0.0f, 1.0f);
  return x * x * (3.0f - 2.0f * x);
}

inline int wrapIndex(int index, int size) {
  while (index < 0) index += size;
  while (index >= size) index -= size;
  return index;
}

inline float readCubic(const float* buffer, int size, float position) {
  while (position < 0.0f) position += static_cast<float>(size);
  while (position >= static_cast<float>(size)) position -= static_cast<float>(size);

  const int i1 = static_cast<int>(position);
  const float t = position - static_cast<float>(i1);
  const int i0 = wrapIndex(i1 - 1, size);
  const int i2 = wrapIndex(i1 + 1, size);
  const int i3 = wrapIndex(i1 + 2, size);

  const float y0 = buffer[i0];
  const float y1 = buffer[i1];
  const float y2 = buffer[i2];
  const float y3 = buffer[i3];

  const float a = 0.5f * (-y0 + 3.0f * y1 - 3.0f * y2 + y3);
  const float b = 0.5f * (2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3);
  const float c = 0.5f * (-y0 + y2);
  return ((a * t + b) * t + c) * t + y1;
}
}

Lofi::Lofi() {
  addInput(mInput);
  addInput(mDepth);
  addInput(mSpeed);
  addInput(mBlend);
  addOutput(mOutput);
  addOption(mWaveform);

  const float sampleRate = globalConfig.sampleRate;

  mSize = static_cast<int>(sampleRate * 0.020f) + 8;
  mBuffer = new float[mSize];
  for (int i = 0; i < mSize; i++) {
    mBuffer[i] = 0.0f;
  }

  mAttackCoeff = 1.0f - std::exp(-1.0f / (0.004f * sampleRate));
  mReleaseCoeff = 1.0f - std::exp(-1.0f / (0.300f * sampleRate));
  mGainSmoothingCoeff = 1.0f - std::exp(-1.0f / (0.006f * sampleRate));
}

Lofi::~Lofi() {
  delete[] mBuffer;
}

void Lofi::process() {
  const float* input = mInput.buffer();
  const float* depth = mDepth.buffer();
  const float* speed = mSpeed.buffer();
  const float* blend = mBlend.buffer();
  float* output = mOutput.buffer();

  const float sampleRate = globalConfig.sampleRate;
  const float samplePeriod = globalConfig.samplePeriod;

  constexpr float loFiCutoffHz = 450.0f;
  const float loFiFilterCoeff = 1.0f - std::exp(-twoPi * loFiCutoffHz * samplePeriod);

  const float triangleSlewCoeff = 1.0f - std::exp(-1.0f / (0.008f * sampleRate));
  const float squareSlewCoeff = 1.0f - std::exp(-1.0f / (0.012f * sampleRate));
  const float squareRingDecay = std::exp(-1.0f / (0.022f * sampleRate));
  constexpr float squareRingHz = 16.0f;

  float env = mEnv;
  float compGain = mCompGain;
  float phase = mPhase;
  float squareState = mSquareState;
  float squareRingPhase = mSquareRingPhase;
  float squareRingEnvelope = mSquareRingEnvelope;
  float squarePreviousTarget = mSquarePreviousTarget;
  float triangleState = mTriangleState;
  float loFiLp = mLoFiLp;
  int writeIndex = mWrite;

  const int waveform = mWaveform.value();
  constexpr float threshold = 0.12f;
  constexpr float compressorExponent = -0.8333333333f;

  if (waveform == 6 && mPreviousWaveform != 6) {
    const float startingShape = (mPreviousWaveform == 4)
        ? sineLfo(phase)
        : triangleState;
    squareState = startingShape;
    squarePreviousTarget = phase < 0.5f ? 1.0f : -1.0f;
    squareRingEnvelope = 0.0f;
    squareRingPhase = 0.0f;
  }

  if (waveform == 5 && mPreviousWaveform != 5) {
    triangleState = (mPreviousWaveform == 4)
        ? sineLfo(phase)
        : squareState;
  }

  for (int i = 0; i < FRAMELENGTH; i++) {
    const float x = input[i];

    const float magnitude = std::fabs(x);
    if (magnitude > env) {
      env += mAttackCoeff * (magnitude - env);
    } else {
      env += mReleaseCoeff * (magnitude - env);
    }

    float targetGain = 1.0f;
    if (env > threshold) {
      targetGain = std::pow(env / threshold, compressorExponent);
    }
    compGain += mGainSmoothingCoeff * (targetGain - compGain);

    const float compressed = softSaturate(x * compGain * 2.0f) * 1.15f;
    mBuffer[writeIndex] = compressed;

    const float speedAmount = clampf(speed[i], 0.0f, 1.0f);
    phase += speedToHertz(speedAmount) * samplePeriod;
    if (phase >= 1.0f) {
      phase -= std::floor(phase);
    }

    float modulation = 0.0f;
    float maximumExcursionMs = 1.18f;

    if (waveform == 6) {
      const float squareTarget = phase < 0.5f ? 1.0f : -1.0f;
      if (squareTarget != squarePreviousTarget) {
        squarePreviousTarget = squareTarget;
        squareRingEnvelope = 1.0f;
        squareRingPhase = 0.0f;
      }

      squareState += squareSlewCoeff * (squareTarget - squareState);

      const float ring = 0.028f * squareRingEnvelope
          * std::sin(squareRingPhase) * squareTarget;
      squareRingPhase += twoPi * squareRingHz * samplePeriod;
      if (squareRingPhase >= twoPi) {
        squareRingPhase -= twoPi;
      }
      squareRingEnvelope *= squareRingDecay;

      modulation = clampf(squareState + ring, -1.03f, 1.03f);
      maximumExcursionMs = 0.30f;
    } else if (waveform == 4) {
      modulation = sineLfo(phase);
      maximumExcursionMs = 1.18f;
      squareState = modulation;
      squareRingEnvelope = 0.0f;
    } else {
      const float rawTriangle = triangleLfo(phase);
      triangleState += triangleSlewCoeff * (rawTriangle - triangleState);
      modulation = triangleState;
      maximumExcursionMs = 0.95f;
      squareState = modulation;
      squareRingEnvelope = 0.0f;
    }

    const float depthAmount = smoothDepth(depth[i]);

    constexpr float baseDelayMs = 2.8f;
    const float baseDelaySamples = baseDelayMs * 0.001f * sampleRate;
    const float compPosition = static_cast<float>(writeIndex) - baseDelaySamples;
    const float compressedForBlend = readCubic(mBuffer, mSize, compPosition);

    const float excursionMs = maximumExcursionMs * depthAmount * modulation;
    const float loFiDelaySamples = clampf((baseDelayMs + excursionMs) * 0.001f * sampleRate,
                                          2.0f,
                                          static_cast<float>(mSize - 4));
    const float loFiPosition = static_cast<float>(writeIndex) - loFiDelaySamples;
    float loFi = readCubic(mBuffer, mSize, loFiPosition);

    loFi = softSaturate(loFi * 1.08f) * 1.04f;
    loFiLp += loFiFilterCoeff * (loFi - loFiLp);
    loFi = loFiLp;

    const float mix = clampf(blend[i], 0.0f, 1.0f);
    output[i] = compressedForBlend * (1.0f - mix) + loFi * mix;

    if (++writeIndex >= mSize) {
      writeIndex = 0;
    }
  }

  mEnv = env;
  mCompGain = compGain;
  mPhase = phase;
  mSquareState = squareState;
  mSquareRingPhase = squareRingPhase;
  mSquareRingEnvelope = squareRingEnvelope;
  mSquarePreviousTarget = squarePreviousTarget;
  mTriangleState = triangleState;
  mPreviousWaveform = waveform;
  mLoFiLp = loFiLp;
  mWrite = writeIndex;
}
