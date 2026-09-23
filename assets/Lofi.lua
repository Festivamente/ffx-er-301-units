local app = app
local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local OptionControl = require "Unit.ViewControl.OptionControl"
local Encoder = require "Encoder"

local Lofi = Class {}
Lofi:include(Unit)

function Lofi:init(args)
  args.title = "Lo-Fi Filter"
  args.mnemonic = "LF"
  Unit.init(self, args)
end

function Lofi:onLoadGraph(channelCount)
  local lofiL = self:addObject("lofiL", libFestivamente.Lofi())
  local lofiR

  if channelCount == 2 then
    lofiR = self:addObject("lofiR", libFestivamente.Lofi())
    tie(lofiR, "Waveform", lofiL, "Waveform")
  end

  local depth = self:addObject("depth", app.GainBias())
  local depthRange = self:addObject("depthRange", app.MinMax())
  depth:hardSet("Bias", 0.35)

  local speed = self:addObject("speed", app.GainBias())
  local speedRange = self:addObject("speedRange", app.MinMax())
  speed:hardSet("Bias", 0.50)

  local blend = self:addObject("blend", app.GainBias())
  local blendRange = self:addObject("blendRange", app.MinMax())
  blend:hardSet("Bias", 1.0)

  local level = self:addObject("level", app.GainBias())
  local levelRange = self:addObject("levelRange", app.MinMax())
  level:hardSet("Bias", 0.5)

  connect(depth, "Out", depthRange, "In")
  connect(speed, "Out", speedRange, "In")
  connect(blend, "Out", blendRange, "In")
  connect(level, "Out", levelRange, "In")

  local targets = { lofiL }
  if lofiR then
    targets[2] = lofiR
  end

  for _, lofi in ipairs(targets) do
    connect(depth, "Out", lofi, "Depth")
    connect(speed, "Out", lofi, "Speed")
    connect(blend, "Out", lofi, "Blend")
  end

  if channelCount == 2 then
    local outputLevelL = self:addObject("outputLevelL", app.Multiply())
    local outputLevelR = self:addObject("outputLevelR", app.Multiply())

    connect(self, "In1", lofiL, "In")
    connect(self, "In2", lofiR, "In")

    connect(lofiL, "Out", outputLevelL, "Left")
    connect(level, "Out", outputLevelL, "Right")
    connect(lofiR, "Out", outputLevelR, "Left")
    connect(level, "Out", outputLevelR, "Right")

    connect(outputLevelL, "Out", self, "Out1")
    connect(outputLevelR, "Out", self, "Out2")
  else
    local outputLevel = self:addObject("outputLevel", app.Multiply())

    connect(self, "In1", lofiL, "In")
    connect(lofiL, "Out", outputLevel, "Left")
    connect(level, "Out", outputLevel, "Right")
    connect(outputLevel, "Out", self, "Out1")
  end

  self:addMonoBranch("blend", blend, "In", blend, "Out")
  self:addMonoBranch("speed", speed, "In", speed, "Out")
  self:addMonoBranch("depth", depth, "In", depth, "Out")
  self:addMonoBranch("level", level, "In", level, "Out")
end

local views = {
  expanded = {
    "blend",
    "speed",
    "depth",
    "waveform",
    "level"
  },
  collapsed = {}
}

function Lofi:onLoadViews(objects, branches)
  local controls = {}

  controls.level = GainBias {
    button = "level",
    description = "Volume",
    branch = branches.level,
    gainbias = objects.level,
    range = objects.levelRange,
    initialBias = 0.5
  }

  controls.blend = GainBias {
    button = "blend",
    description = "Comp <-> Lo-Fi",
    branch = branches.blend,
    gainbias = objects.blend,
    range = objects.blendRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 1.0,
    gainMap = Encoder.getMap("[-10,10]")
  }

  controls.speed = GainBias {
    button = "speed",
    description = "Vibrato Speed",
    branch = branches.speed,
    gainbias = objects.speed,
    range = objects.speedRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 0.50,
    gainMap = Encoder.getMap("[-10,10]")
  }

  controls.depth = GainBias {
    button = "depth",
    description = "Vibrato Depth",
    branch = branches.depth,
    gainbias = objects.depth,
    range = objects.depthRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 0.35,
    gainMap = Encoder.getMap("[-10,10]")
  }

  controls.waveform = OptionControl {
    button = "wave",
    description = "Vibrato Waveform",
    option = objects.lofiL:getOption("Waveform"),
    choices = {
      "prev", "", "next",
      "Sine",
      "Triangle",
      "Square"
    },
    muteOnChange = true
  }

  local oldSubReleased = controls.waveform.subReleased
  function controls.waveform:subReleased(i, shifted)
    if shifted then return false end

    local value = self.option:value()

    if i == 1 then
      value = math.max(4, value - 1)
      self.option:set(value)
      self:update()
      return true
    elseif i == 3 then
      value = math.min(6, value + 1)
      self.option:set(value)
      self:update()
      return true
    elseif i == 2 then
      self:update()
      return true
    else
      return oldSubReleased(self, i, shifted)
    end
  end

  return controls, views
end

return Lofi
