#pragma once

namespace timefx {

inline float readLinearAt(const float* buffer, int size, float position) {
  if (position < 0.0f)
    position += (float)size;

  const int index0 = (int)position;
  const float fraction = position - (float)index0;
  int index1 = index0 + 1;
  if (index1 >= size)
    index1 = 0;

  return buffer[index0] + fraction * (buffer[index1] - buffer[index0]);
}

inline float readLinearDelay(const float* buffer,
                             int size,
                             int writeIndex,
                             float delay) {
  return readLinearAt(buffer, size, (float)writeIndex - delay);
}

}
