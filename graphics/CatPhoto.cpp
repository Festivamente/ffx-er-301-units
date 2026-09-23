#include <graphics/CatPhoto.h>
#include <graphics/PhotoData.h>

namespace
{

constexpr uint8_t kBayer4x4[4][4] = {
    {0, 8, 2, 10},
    {12, 4, 14, 6},
    {3, 11, 1, 9},
    {15, 7, 13, 5}};
}

CatPhoto::CatPhoto(int left, int bottom, int width, int height)
    : od::Graphic(left, bottom, width, height)
{
}

void CatPhoto::draw(od::FrameBuffer &fb)
{

  const int width = kPhotoWidth < mWidth ? kPhotoWidth : mWidth;
  const int height = kPhotoHeight < mHeight ? kPhotoHeight : mHeight;
  const int x0 = mWorldLeft + (mWidth - width) / 2;

  const int yTop = mWorldBottom + (mHeight + height) / 2 - 1;

  if (fb.mIsMonoChrome)
  {
    for (int row = 0; row < height; ++row)
    {
      const int y = yTop - row;
      const uint8_t *line = kPhotoPixels + row * kPhotoWidth;

      for (int x = 0; x < width; ++x)
      {
        if (line[x] > kBayer4x4[row & 3][x & 3])
        {
          fb.pixel(WHITE, x0 + x, y);
        }
      }
    }
    return;
  }

  for (int row = 0; row < height; ++row)
  {
    const int y = yTop - row;
    const uint8_t *line = kPhotoPixels + row * kPhotoWidth;

    int x = 0;
    while (x < width)
    {
      const uint8_t color = line[x];
      int runEnd = x + 1;
      while (runEnd < width && line[runEnd] == color)
      {
        ++runEnd;
      }

      if (color > 0)
      {
        fb.hline(color, x0 + x, x0 + runEnd - 1, y);
      }
      x = runEnd;
    }
  }
}
