local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local PatchMeter = require "Unit.ViewControl.PatchMeter"
local Patch = require "Chain.Patch"
local GainBias = require "Unit.ViewControl.GainBias"
local Encoder = require "Encoder"

local WDWrap = Class {}
WDWrap:include(Unit)

function WDWrap:init(args)
  local chain = args.chain or app.logError("%s.init: chain is missing.", self)
  args.title = "Wet/Dry"
  args.mnemonic = "WD"
  args.pUnit = libFestivamente.WetDryUnit(args.title, chain.channelCount)
  Unit.init(self, args)
end

function WDWrap:notifyBranches(method, ...)
  if self.patch then
    local f = self.patch[method]
    if f ~= nil then
      f(self.patch, ...)
    end
  end
  Unit.notifyBranches(self, method, ...)
end

function WDWrap:findUnitByTitle(title)
  local unit = Unit.findUnitByTitle(self, title)
  if unit then
    return unit
  end
  if self.patch then
    return self.patch:findUnitByTitle(title)
  end
end

function WDWrap:findByInstanceKey(key)
  local o = Unit.findByInstanceKey(self, key)
  if o then
    return o
  end
  if self.patch then
    return self.patch:findByInstanceKey(key)
  end
end

function WDWrap:onLoadGraph(channelCount)
  local wetdry = self:addObject("wetdry", app.GainBias())
  local wetdryRange = self:addObject("wetdryRange", app.MinMax())
  connect(wetdry, "Out", wetdryRange, "In")

  for i = 1, channelCount do
    connect(wetdry, "Out", self.pUnit:getFader(i - 1), "Fade")
  end

  self:addMonoBranch("wetdry", wetdry, "In", wetdry, "Out")

  self.patch = Patch {
    title = self.title,
    depth = self.depth,
    channelCount = channelCount,
    unit = self
  }
end

local views = {
  expanded = {
    "wetdry",
    "meter"
  },
  collapsed = {}
}

function WDWrap:onLoadViews(objects, branches)
  local controls = {}

  controls.wetdry = GainBias {
    button = "wet/dry",
    description = "Wet/Dry",
    branch = branches.wetdry,
    gainbias = objects.wetdry,
    range = objects.wetdryRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 1.0
  }

  controls.meter = PatchMeter {
    button = "open",
    description = "Patch",
    patch = self.patch
  }

  return controls, views
end

function WDWrap:onRemove()
  self.patch:stop()
  self.patch:releaseResources()
  self.patch:hide()
  Unit.onRemove(self)
end

function WDWrap:onGenerateTitle()
  local Settings = require "Settings"
  local mode = Settings.get("containerUnitNameGen")
  if mode == "off" then
    Unit.onGenerateTitle(self)
  else
    local Random = require "Random"
    self:setTitle(Random.generateName(mode))
    self.hasUserTitle = true
  end
end

function WDWrap:serialize()
  local t = Unit.serialize(self)
  t.bands = {
    self.patch:serialize()
  }
  return t
end

function WDWrap:deserialize(t)
  Unit.deserialize(self, t)
  if t.bands and t.bands[1] then
    self.patch:deserialize(t.bands[1])
  elseif t.patch then
    self.patch:deserialize(t.patch)
  end
end

return WDWrap
