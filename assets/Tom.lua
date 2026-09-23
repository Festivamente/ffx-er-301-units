local app = app
local libcore = require "core.libcore"
local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local Pitch = require "Unit.ViewControl.Pitch"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"
local Encoder = require "Encoder"

local Tom = Class {}
Tom:include(Unit)

function Tom:init(args)
  args.title = "Tom"
  args.mnemonic = "Tm"
  Unit.init(self, args)
end

function Tom:onLoadGraph(channelCount)
  local tom = self:addObject("tom", libFestivamente.Tom())

  local trig = self:addObject("trig", app.Comparator())
  trig:setTriggerMode()

  local tune       = self:addObject("tune", app.ConstantOffset())
  local tuneRange  = self:addObject("tuneRange", app.MinMax())
  local vpo        = self:addObject("vpo", libcore.VoltPerOctave())
  local f0         = self:addObject("f0", app.GainBias())
  local f0Range    = self:addObject("f0Range", app.MinMax())
  local frequency  = self:addObject("frequency", app.Multiply())

  local decay      = self:addObject("decay", app.GainBias())
  local decayRange = self:addObject("decayRange", app.MinMax())
  local bend       = self:addObject("bend", app.GainBias())
  local bendRange  = self:addObject("bendRange", app.MinMax())

  connect(trig, "Out", tom, "Trig")

  connect(tune, "Out", tuneRange, "In")
  connect(tune, "Out", vpo, "In")
  connect(vpo, "Out", frequency, "Left")

  connect(f0, "Out", f0Range, "In")
  connect(f0, "Out", frequency, "Right")
  connect(frequency, "Out", tom, "Tune")

  connect(decay, "Out", decayRange, "In")
  connect(decay, "Out", tom, "Decay")
  connect(bend, "Out", bendRange, "In")
  connect(bend, "Out", tom, "Bend")
  connect(tom, "Out", self, "Out1")

  self:addMonoBranch("trig", trig, "In", trig, "Out")
  self:addMonoBranch("tune", tune, "In", tune, "Out")
  self:addMonoBranch("f0", f0, "In", f0, "Out")
  self:addMonoBranch("decay", decay, "In", decay, "Out")
  self:addMonoBranch("bend", bend, "In", bend, "Out")

  if channelCount > 1 then
    connect(tom, "Out", self, "Out2")
  end
end

local views = {
  expanded = { "trig", "tune", "freq", "decay", "bend" },
  collapsed = {}
}

local function linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

local decayMap = linMap(0.05, 2.0, 0.25, 0.05, 0.01, 0.001)

function Tom:onLoadViews(objects, branches)
  local controls = {}

  controls.trig = Gate {
    button = "trig",
    description = "Trigger",
    branch = branches.trig,
    comparator = objects.trig
  }

  controls.tune = Pitch {
    button = "V/oct",
    branch = branches.tune,
    description = "V/oct",
    offset = objects.tune,
    range = objects.tuneRange
  }

  controls.freq = GainBias {
    button = "f0",
    description = "Fundamental",
    branch = branches.f0,
    gainbias = objects.f0,
    range = objects.f0Range,
    biasMap = Encoder.getMap("oscFreq"),
    biasUnits = app.unitHertz,
    initialBias = 120,
    gainMap = Encoder.getMap("freqGain"),
    scaling = app.octaveScaling
  }

  controls.decay = GainBias {
    button = "decay",
    description = "Decay",
    branch = branches.decay,
    gainbias = objects.decay,
    range = objects.decayRange,
    biasMap = decayMap,
    biasUnits = app.unitSecs,
    initialBias = 0.35
  }

  controls.bend = GainBias {
    button = "bend",
    description = "Pitch Bend",
    branch = branches.bend,
    gainbias = objects.bend,
    range = objects.bendRange,
    biasMap = Encoder.getMap("[0,1]"),
    initialBias = 0.4
  }

  return controls, views
end

return Tom
