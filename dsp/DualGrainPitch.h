#pragma once

#include <dsp/CircularBuffer.h>
#include <dsp/DspMath.h>
#include <cmath>

namespace timefx {

inline float readDualGrainPitch(const float* buffer,
                                int bufferSize,
                                int writeIndex,
                                float baseDelay,
                                float window,
                                float ratio,
                                float& phase) {
  const float invWindow = 1.0f / window;

  phase += (1.0f - ratio) * invWindow;
  phase -= floorf(phase);

  float phase2 = phase + 0.5f;
  if (phase2 >= 1.0f)
    phase2 -= 1.0f;

  const float sample1 = readLinearDelay(buffer, bufferSize, writeIndex,
                                        baseDelay + phase * window);
  const float sample2 = readLinearDelay(buffer, bufferSize, writeIndex,
                                        baseDelay + phase2 * window);

  return sinPi01(phase) * sample1 + sinPi01(phase2) * sample2;
}

inline float grainMatchError(const float* buffer,
                             int bufferSize,
                             int writeIndex,
                             float candidateDelay,
                             float referenceDelay) {
  const float c0 =
      readLinearDelay(buffer, bufferSize, writeIndex, candidateDelay);
  const float c1 =
      readLinearDelay(buffer, bufferSize, writeIndex, candidateDelay + 1.0f);
  const float c2 =
      readLinearDelay(buffer, bufferSize, writeIndex, candidateDelay + 2.0f);

  const float r0 =
      readLinearDelay(buffer, bufferSize, writeIndex, referenceDelay);
  const float r1 =
      readLinearDelay(buffer, bufferSize, writeIndex, referenceDelay + 1.0f);
  const float r2 =
      readLinearDelay(buffer, bufferSize, writeIndex, referenceDelay + 2.0f);

  const float valueError = c0 - r0;
  const float slopeError = (c0 - c1) - (r0 - r1);
  const float curveError =
      (c0 - 2.0f * c1 + c2) - (r0 - 2.0f * r1 + r2);

  return valueError * valueError +
         4.0f * slopeError * slopeError +
         2.0f * curveError * curveError;
}

inline float findAlignedGrainCorrection(const float* buffer,
                                        int bufferSize,
                                        int writeIndex,
                                        float nominalDelay,
                                        float referenceDelay,
                                        int searchSamples) {

  int limit = searchSamples;
  const int available =
      (int)((float)bufferSize - 4.0f - nominalDelay);
  if (limit > available)
    limit = available;
  if (limit <= 0)
    return 0.0f;

  int bestOffset = 0;
  float bestError = grainMatchError(buffer, bufferSize, writeIndex,
                                    nominalDelay, referenceDelay);

  for (int offset = 1; offset <= limit; offset++) {
    const float error = grainMatchError(buffer, bufferSize, writeIndex,
                                        nominalDelay + (float)offset,
                                        referenceDelay);
    if (error < bestError) {
      bestError = error;
      bestOffset = offset;
    }
  }

  return (float)bestOffset;
}

inline float triangleGrainWindow(float phase) {
  return 1.0f - fabsf(2.0f * phase - 1.0f);
}

inline float readPhaseAlignedPitch(const float* buffer,
                                   int bufferSize,
                                   int writeIndex,
                                   float baseDelay,
                                   float window,
                                   int searchSamples,
                                   bool historyReady,
                                   float ratio,
                                   float& phase0,
                                   float& phase1,
                                   float& correction0,
                                   float& correction1) {
  const float delta = (1.0f - ratio) / window;

  const float raw0 = phase0 + delta;
  const float raw1 = phase1 + delta;
  const bool wrapped0 = raw0 >= 1.0f || raw0 < 0.0f;
  const bool wrapped1 = raw1 >= 1.0f || raw1 < 0.0f;

  phase0 = raw0 - floorf(raw0);
  phase1 = raw1 - floorf(raw1);

  if (historyReady) {
    if (wrapped0) {
      const float referenceDelay =
          baseDelay + phase1 * window + correction1;
      const float nominalDelay = baseDelay + phase0 * window;
      correction0 = findAlignedGrainCorrection(
          buffer, bufferSize, writeIndex, nominalDelay,
          referenceDelay, searchSamples);
    }

    if (wrapped1) {
      const float referenceDelay =
          baseDelay + phase0 * window + correction0;
      const float nominalDelay = baseDelay + phase1 * window;
      correction1 = findAlignedGrainCorrection(
          buffer, bufferSize, writeIndex, nominalDelay,
          referenceDelay, searchSamples);
    }
  }

  const float sample0 = readLinearDelay(
      buffer, bufferSize, writeIndex,
      baseDelay + phase0 * window + correction0);
  const float sample1 = readLinearDelay(
      buffer, bufferSize, writeIndex,
      baseDelay + phase1 * window + correction1);

  const float gain0 = triangleGrainWindow(phase0);
  const float gain1 = 1.0f - gain0;

  return gain0 * sample0 + gain1 * sample1;
}

}
