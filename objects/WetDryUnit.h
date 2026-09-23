#pragma once

#include <od/units/CustomUnit.h>
#include <od/objects/measurement/Monitor.h>
#include <WetDryFade.h>
#include <vector>

class WetDryUnit : public od::CustomUnit {
public:
  WetDryUnit(const std::string& name, int channelCount);
  ~WetDryUnit();

  od::Object* getFader(int channel);

#ifndef SWIGLUA
  virtual void process();
#endif

protected:
  std::vector<od::Monitor*> mInputMonitors;
  std::vector<od::Monitor*> mOutputMonitors;
  std::vector<WetDryFade*> mFaders;

  virtual void connectUnits();
  virtual void disconnectUnits();
};
