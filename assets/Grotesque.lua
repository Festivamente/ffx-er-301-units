local app = app
local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Encoder = require "Encoder"

local Grotesque = Class {}
Grotesque:include(Unit)

function Grotesque:init(args)
  args.title = "Grotesque Filter"
  args.mnemonic = "GF"
  Unit.init(self, args)
end

function Grotesque:onLoadGraph(channelCount)
  local grotesqueL = self:addObject("grotesqueL", libFestivamente.Grotesque())

  local input = self:addObject("input", app.GainBias())
  local inputRange = self:addObject("inputRange", app.MinMax())
  local gain = self:addObject("gain", app.GainBias())
  local gainRange = self:addObject("gainRange", app.MinMax())
  local high = self:addObject("high", app.GainBias())
  local highRange = self:addObject("highRange", app.MinMax())
  local low = self:addObject("low", app.GainBias())
  local lowRange = self:addObject("lowRange", app.MinMax())
  local level = self:addObject("level", app.GainBias())
  local levelRange = self:addObject("levelRange", app.MinMax())

  local outputLevelL = self:addObject("outputLevelL", app.Multiply())

  connect(input, "Out", inputRange, "In")
  connect(gain, "Out", gainRange, "In")
  connect(high, "Out", highRange, "In")
  connect(low, "Out", lowRange, "In")
  connect(level, "Out", levelRange, "In")

  connect(self, "In1", grotesqueL, "In")
  connect(high, "Out", grotesqueL, "High")
  connect(low, "Out", grotesqueL, "Low")
  connect(gain, "Out", grotesqueL, "Gain")
  connect(input, "Out", grotesqueL, "Input")

  connect(grotesqueL, "Out", outputLevelL, "Right")
  connect(level, "Out", outputLevelL, "Left")
  connect(outputLevelL, "Out", self, "Out1")

  if channelCount == 2 then
    local grotesqueR = self:addObject("grotesqueR", libFestivamente.Grotesque())
    local outputLevelR = self:addObject("outputLevelR", app.Multiply())

    connect(self, "In2", grotesqueR, "In")
    connect(high, "Out", grotesqueR, "High")
    connect(low, "Out", grotesqueR, "Low")
    connect(gain, "Out", grotesqueR, "Gain")
    connect(input, "Out", grotesqueR, "Input")

    connect(grotesqueR, "Out", outputLevelR, "Right")
    connect(level, "Out", outputLevelR, "Left")
    connect(outputLevelR, "Out", self, "Out2")
  end

  self:addMonoBranch("input", input, "In", input, "Out")
  self:addMonoBranch("gain", gain, "In", gain, "Out")
  self:addMonoBranch("high", high, "In", high, "Out")
  self:addMonoBranch("low", low, "In", low, "Out")
  self:addMonoBranch("level", level, "In", level, "Out")
end

local views = {
  expanded = {
    "input",
    "gain",
    "high",
    "low",
    "level"
  },
  collapsed = {}
}

function Grotesque:onLoadViews(objects, branches)
  local controls = {}

  controls.input = GainBias {
    button = "input",
    description = "Input",
    branch = branches.input,
    gainbias = objects.input,
    range = objects.inputRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 0.70,
    gainMap = Encoder.getMap("[-10,10]")
  }

  controls.gain = GainBias {
    button = "gain",
    description = "Gain",
    branch = branches.gain,
    gainbias = objects.gain,
    range = objects.gainRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 0.35,
    gainMap = Encoder.getMap("[-10,10]")
  }

  controls.high = GainBias {
    button = "high",
    description = "High",
    branch = branches.high,
    gainbias = objects.high,
    range = objects.highRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 0.55,
    gainMap = Encoder.getMap("[-10,10]")
  }

  controls.low = GainBias {
    button = "low",
    description = "Low",
    branch = branches.low,
    gainbias = objects.low,
    range = objects.lowRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 0.55,
    gainMap = Encoder.getMap("[-10,10]")
  }

  controls.level = GainBias {
    button = "level",
    description = "Level",
    branch = branches.level,
    gainbias = objects.level,
    range = objects.levelRange,
    initialBias = 0.5
  }
  return controls, views
end

return Grotesque
