local app = app
local libcore = require "core.libcore"
local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local Pitch = require "Unit.ViewControl.Pitch"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"
local Encoder = require "Encoder"

local Cymbal = Class {}
Cymbal:include(Unit)

function Cymbal:init(args)
  args.title = "Cymbal"
  args.mnemonic = "Cy"
  Unit.init(self, args)
end

function Cymbal:onLoadGraph(channelCount)
  local cymbal = self:addObject("cymbal", libFestivamente.Cymbal())

  local trig = self:addObject("trig", app.Comparator())
  trig:setTriggerMode()

  local tune        = self:addObject("tune", app.ConstantOffset())
  local tuneRange   = self:addObject("tuneRange", app.MinMax())
  local vpo         = self:addObject("vpo", libcore.VoltPerOctave())
  local f0          = self:addObject("f0", app.GainBias())
  local f0Range     = self:addObject("f0Range", app.MinMax())
  local frequency   = self:addObject("frequency", app.Multiply())

  local decay       = self:addObject("decay", app.GainBias())
  local decayRange  = self:addObject("decayRange", app.MinMax())
  local tone        = self:addObject("tone", app.GainBias())
  local toneRange   = self:addObject("toneRange", app.MinMax())
  local sizzle      = self:addObject("sizzle", app.GainBias())
  local sizzleRange = self:addObject("sizzleRange", app.MinMax())

  connect(trig, "Out", cymbal, "Trig")

  connect(tune, "Out", tuneRange, "In")
  connect(tune, "Out", vpo, "In")
  connect(vpo, "Out", frequency, "Left")

  connect(f0, "Out", f0Range, "In")
  connect(f0, "Out", frequency, "Right")
  connect(frequency, "Out", cymbal, "Tune")

  connect(decay, "Out", decayRange, "In")
  connect(decay, "Out", cymbal, "Decay")
  connect(tone, "Out", toneRange, "In")
  connect(tone, "Out", cymbal, "Tone")
  connect(sizzle, "Out", sizzleRange, "In")
  connect(sizzle, "Out", cymbal, "Sizzle")
  connect(cymbal, "Out", self, "Out1")

  self:addMonoBranch("trig", trig, "In", trig, "Out")
  self:addMonoBranch("tune", tune, "In", tune, "Out")
  self:addMonoBranch("f0", f0, "In", f0, "Out")
  self:addMonoBranch("decay", decay, "In", decay, "Out")
  self:addMonoBranch("tone", tone, "In", tone, "Out")
  self:addMonoBranch("sizzle", sizzle, "In", sizzle, "Out")

  if channelCount > 1 then
    connect(cymbal, "Out", self, "Out2")
  end
end

local views = {
  expanded = { "trig", "tune", "freq", "decay", "tone", "sizzle" },
  collapsed = {}
}

local function linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

local decayMap = linMap(0.2, 5.0, 0.5, 0.1, 0.01, 0.001)
local toneMap = linMap(1500, 8000, 1000, 100, 10, 1)
local sizzleMap = linMap(0.0, 1.0, 0.25, 0.05, 0.01, 0.001)

function Cymbal:onLoadViews(objects, branches)
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
    initialBias = 373,
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
    initialBias = 1.5
  }

  controls.tone = GainBias {
    button = "tone",
    description = "Tone",
    branch = branches.tone,
    gainbias = objects.tone,
    range = objects.toneRange,
    biasMap = toneMap,
    biasUnits = app.unitHertz,
    initialBias = 5000
  }

  controls.sizzle = GainBias {
    button = "sizzle",
    description = "Sizzle",
    branch = branches.sizzle,
    gainbias = objects.sizzle,
    range = objects.sizzleRange,
    biasMap = sizzleMap,
    biasUnits = app.unitNone,
    initialBias = 0.4
  }

  return controls, views
end

return Cymbal
