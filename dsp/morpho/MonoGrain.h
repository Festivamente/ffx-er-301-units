#pragma once

#include "Grain.h"

namespace morpho {
using namespace od;

class MonoGrain : public Grain {
public:
  MonoGrain();
  virtual ~MonoGrain();

  void synthesizeFromMonoToMono(float* out);
  void synthesizeFromMonoToStereo(float* left, float* right);
  void synthesizeFromStereo(float* out);

protected:
  inline void incrementPhaseOnMono();
  inline void incrementPhaseOnStereo();

  float mFifo[3] = {0, 0, 0};
};

}
