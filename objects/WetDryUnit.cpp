#include <WetDryUnit.h>
#include <hal/ops.h>

using namespace od;

WetDryUnit::WetDryUnit(const std::string& name, int channelCount)
    : od::CustomUnit(name, channelCount) {
  for (int i = 0; i < channelCount; i++) {
    Monitor* monitor = new Monitor();
    monitor->attach();
    addInput(i, monitor, "In");
    mInputMonitors.push_back(monitor);
  }

  for (int i = 0; i < channelCount; i++) {
    Monitor* monitor = new Monitor();
    monitor->attach();
    mOutputMonitors.push_back(monitor);
  }

  for (int i = 0; i < channelCount; i++) {
    WetDryFade* fader = new WetDryFade();
    fader->attach();
    fader->mInputA.connect(mInputMonitors[i]->getOutput(0));
    fader->mInputB.connect(mOutputMonitors[i]->getOutput(0));
    setOutput(i, fader, "Out");
    mFaders.push_back(fader);
  }

  connectUnits();
}

WetDryUnit::~WetDryUnit() {
  disconnectUnits();

  for (WetDryFade* fader : mFaders)
    fader->release();

  for (Monitor* monitor : mInputMonitors)
    monitor->release();

  for (Monitor* monitor : mOutputMonitors)
    monitor->release();
}

od::Object* WetDryUnit::getFader(int channel) {
  if (channel >= 0 && channel < static_cast<int>(mFaders.size()))
    return mFaders[channel];

  return nullptr;
}

void WetDryUnit::connectUnits() {
  Unit* previous = nullptr;

  for (Unit* unit : mUnits) {
    if (previous) {
      Unit::connect(previous, unit);
    } else {
      const int n = MIN(unit->getInputCount(), static_cast<int>(mInputMonitors.size()));
      for (int i = 0; i < n; i++) {
        Outlet* outlet = mInputMonitors[i]->getOutput(0);
        std::vector<Inlet*>& inlets = unit->getInputs(i);
        for (Inlet* inlet : inlets)
          inlet->connect(outlet);
      }
    }

    if (!unit->getBypass())
      previous = unit;
  }

  if (previous) {
    const int n = MIN(previous->getOutputCount(), static_cast<int>(mOutputMonitors.size()));
    for (int i = 0; i < n; i++) {
      Outlet* outlet = previous->getOutput(i);
      if (outlet)
        mOutputMonitors[i]->getInput(0)->connect(outlet);
    }
  } else {
    const int n = MIN(static_cast<int>(mInputMonitors.size()),
                      static_cast<int>(mOutputMonitors.size()));
    for (int i = 0; i < n; i++)
      mOutputMonitors[i]->getInput(0)->connect(mInputMonitors[i]->getOutput(0));
  }
}

void WetDryUnit::disconnectUnits() {
  for (Unit* unit : mUnits)
    unit->disconnect();

  for (Monitor* monitor : mOutputMonitors)
    monitor->getInput(0)->disconnect();
}

void WetDryUnit::process() {
  mMutex.enter();

  if (mEnabled) {
    Unit::process();
  }

  for (Monitor* monitor : mInputMonitors)
    monitor->process();

  for (Unit* unit : mUnits) {
    if (unit->mEnabled)
      unit->process();
  }

  for (Monitor* monitor : mOutputMonitors)
    monitor->process();

  for (WetDryFade* fader : mFaders) {
    fader->updateParameters();
    fader->process();
  }

  mMutex.leave();
}
