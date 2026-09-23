#pragma once

#include <od/graphics/Graphic.h>

class CatPhoto : public od::Graphic
{
public:
  CatPhoto(int left, int bottom, int width, int height);
  ~CatPhoto() override = default;

  od::Graphic *asGraphic() { return this; }

#ifndef SWIGLUA
  void draw(od::FrameBuffer &fb) override;
#endif
};
