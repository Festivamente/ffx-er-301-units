#pragma once

namespace timefx {

inline float clamp(float x, float lo, float hi) {
  return x < lo ? lo : (x > hi ? hi : x);
}

inline float sinPi01(float x) {
  const float product = x * (1.0f - x);
  return 16.0f * product / (5.0f - 4.0f * product);
}

inline float sineCycle(float phase) {
  if (phase < 0.5f)
    return sinPi01(2.0f * phase);

  return -sinPi01(2.0f * (phase - 0.5f));
}

inline float wrapUnit(float phase) {
  if (phase >= 1.0f)
    phase -= 1.0f;
  return phase;
}

}
