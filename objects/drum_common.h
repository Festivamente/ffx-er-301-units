#pragma once
#include <od/config.h>
#include <cmath>

static inline float clampf(float x, float lo, float hi)
{
  return x < lo ? lo : (x > hi ? hi : x);
}

static inline float sinpi01(float x)
{
  float k = x * (1.0f - x);
  return 16.0f * k / (5.0f - 4.0f * k);
}

static inline float sineLfo(float p)
{
  if (p < 0.5f)
    return sinpi01(2.0f * p);
  else
    return -sinpi01(2.0f * (p - 0.5f));
}

static inline float softclip(float x)
{
  x = clampf(x, -1.7f, 1.7f);
  return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

static inline float decayCoeff(float seconds)
{
  return expf(-1.0f / (seconds * globalConfig.sampleRate));
}

static inline float lcgNoise(unsigned int &state)
{
  state = state * 1664525u + 1013904223u;
  return (float)(int)state * 4.6566129e-10f;
}
