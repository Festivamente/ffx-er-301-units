local app = app
local libcore = require "core.libcore"
local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local Pitch = require "Unit.ViewControl.Pitch"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"
local Encoder = require "Encoder"

local Cowbell = Class {}
Cowbell:include(Unit)

function Cowbell:init(args)
  args.title = "Cowbell"
  args.mnemonic = "Cb"
  Unit.init(self, args)
end

function Cowbell:onLoadGraph(channelCount)
  local cowbell = self:addObject("cowbell", libFestivamente.Cowbell())

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
  local clank      = self:addObject("clank", app.GainBias())
  local clankRange = self:addObject("clankRange", app.MinMax())

  connect(trig, "Out", cowbell, "Trig")

  connect(tune, "Out", tuneRange, "In")
  connect(tune, "Out", vpo, "In")
  connect(vpo, "Out", frequency, "Left")

  connect(f0, "Out", f0Range, "In")
  connect(f0, "Out", frequency, "Right")
  connect(frequency, "Out", cowbell, "Tune")

  connect(decay, "Out", decayRange, "In")
  connect(decay, "Out", cowbell, "Decay")
  connect(clank, "Out", clankRange, "In")
  connect(clank, "Out", cowbell, "Clank")
  connect(cowbell, "Out", self, "Out1")

  self:addMonoBranch("trig", trig, "In", trig, "Out")
  self:addMonoBranch("tune", tune, "In", tune, "Out")
  self:addMonoBranch("f0", f0, "In", f0, "Out")
  self:addMonoBranch("decay", decay, "In", decay, "Out")
  self:addMonoBranch("clank", clank, "In", clank, "Out")

  if channelCount > 1 then
    connect(cowbell, "Out", self, "Out2")
  end
end

local views = {
  expanded = { "trig", "tune", "freq", "decay", "clank" },
  collapsed = {}
}

local function linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

local decayMap = linMap(0.03, 1.0, 0.25, 0.05, 0.01, 0.001)

function Cowbell:onLoadViews(objects, branches)
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
    initialBias = 540,
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
    initialBias = 0.3
  }

  controls.clank = GainBias {
    button = "clank",
    description = "Clank",
    branch = branches.clank,
    gainbias = objects.clank,
    range = objects.clankRange,
    biasMap = Encoder.getMap("[0,1]"),
    initialBias = 0.35
  }

  return controls, views
end

return Cowbell
