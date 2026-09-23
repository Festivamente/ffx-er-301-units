local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Encoder = require "Encoder"

local Unison = Class {}
Unison:include(Unit)

function Unison:init(args)
  args.title = "Unison"
  args.mnemonic = "Un"
  Unit.init(self, args)
end

function Unison:onLoadGraph(channelCount)
  self._channelCount = channelCount

  local unison = self:addObject("unison", libFestivamente.Unison())

  local detune      = self:addObject("detune", app.GainBias())
  local detuneRange = self:addObject("detuneRange", app.MinMax())
  local blend       = self:addObject("blend", app.GainBias())
  local blendRange  = self:addObject("blendRange", app.MinMax())
  local level       = self:addObject("level", app.GainBias())
  local levelRange  = self:addObject("levelRange", app.MinMax())
  level:hardSet("Bias", 0.5)

  connect(self, "In1", unison, "In")
  connect(detune, "Out", detuneRange, "In")
  connect(detune, "Out", unison, "Detune")
  connect(blend, "Out", blendRange, "In")
  connect(blend, "Out", unison, "Blend")
  connect(level, "Out", levelRange, "In")

  if channelCount > 1 then
    local spread      = self:addObject("spread", app.GainBias())
    local spreadRange = self:addObject("spreadRange", app.MinMax())
    local levelCompL  = self:addObject("levelCompL", app.ConstantGain())
    local levelCompR  = self:addObject("levelCompR", app.ConstantGain())
    local vcaL        = self:addObject("vcaL", app.Multiply())
    local vcaR        = self:addObject("vcaR", app.Multiply())
    levelCompL:hardSet("Gain", 2.0)
    levelCompR:hardSet("Gain", 2.0)

    connect(spread, "Out", spreadRange, "In")
    connect(spread, "Out", unison, "Spread")

    connect(unison, "OutL", levelCompL, "In")
    connect(unison, "OutR", levelCompR, "In")
    connect(level, "Out", vcaL, "Left")
    connect(level, "Out", vcaR, "Left")
    connect(levelCompL, "Out", vcaL, "Right")
    connect(levelCompR, "Out", vcaR, "Right")
    connect(vcaL, "Out", self, "Out1")
    connect(vcaR, "Out", self, "Out2")

    self:addMonoBranch("spread", spread, "In", spread, "Out")
  else
    local levelComp = self:addObject("levelComp", app.ConstantGain())
    local vca = self:addObject("vca", app.Multiply())
    levelComp:hardSet("Gain", 2.0)
    connect(unison, "OutMono", levelComp, "In")
    connect(level, "Out", vca, "Left")
    connect(levelComp, "Out", vca, "Right")
    connect(vca, "Out", self, "Out1")
  end

  self:addMonoBranch("detune", detune, "In", detune, "Out")
  self:addMonoBranch("blend", blend, "In", blend, "Out")
  self:addMonoBranch("level", level, "In", level, "Out")
end

local function linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

local detuneMap = linMap(0, 100, 10, 1, 0.1, 0.01)

function Unison:onLoadViews(objects, branches)
  local controls = {}
  local expanded
  if self._channelCount > 1 then

    expanded = { "detune", "blend", "spread", "level" }
  else
    expanded = { "detune", "blend", "level" }
  end

  controls.detune = GainBias {
    button = "detune",
    description = "Detune (cents)",
    branch = branches.detune,
    gainbias = objects.detune,
    range = objects.detuneRange,
    biasMap = detuneMap,
    biasUnits = app.unitNone,
    initialBias = 12
  }

  controls.blend = GainBias {
    button = "blend",
    description = "Dry/Wet Blend",
    branch = branches.blend,
    gainbias = objects.blend,
    range = objects.blendRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 0.5
  }

  if self._channelCount > 1 then
    controls.spread = GainBias {
      button = "spread",
      description = "Stereo Spread",
      branch = branches.spread,
      gainbias = objects.spread,
      range = objects.spreadRange,
      biasMap = Encoder.getMap("unit"),
      biasUnits = app.unitNone,
      initialBias = 0.0
    }
  end

  controls.level = GainBias {
    button = "level",
    description = "Level",
    branch = branches.level,
    gainbias = objects.level,
    range = objects.levelRange,
    initialBias = 0.5
  }

  local views = {
    expanded = expanded,
    collapsed = {}
  }

  return controls, views
end

return Unison
