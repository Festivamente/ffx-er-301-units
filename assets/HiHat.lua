local app = app
local libcore = require "core.libcore"
local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local Pitch = require "Unit.ViewControl.Pitch"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"
local Encoder = require "Encoder"

local HiHat = Class {}
HiHat:include(Unit)

function HiHat:init(args)
  args.title = "HiHat"
  args.mnemonic = "HH"
  Unit.init(self, args)
end

function HiHat:onLoadGraph(channelCount)
  local hat = self:addObject("hat", libFestivamente.HiHat())

  local trigC = self:addObject("trigC", app.Comparator())
  trigC:setTriggerMode()
  local trigO = self:addObject("trigO", app.Comparator())
  trigO:setTriggerMode()

  local tune       = self:addObject("tune", app.ConstantOffset())
  local tuneRange  = self:addObject("tuneRange", app.MinMax())
  local vpo        = self:addObject("vpo", libcore.VoltPerOctave())
  local f0         = self:addObject("f0", app.GainBias())
  local f0Range    = self:addObject("f0Range", app.MinMax())
  local frequency  = self:addObject("frequency", app.Multiply())

  local decC       = self:addObject("decC", app.GainBias())
  local decCRange  = self:addObject("decCRange", app.MinMax())
  local decO       = self:addObject("decO", app.GainBias())
  local decORange  = self:addObject("decORange", app.MinMax())
  local tone       = self:addObject("tone", app.GainBias())
  local toneRange  = self:addObject("toneRange", app.MinMax())

  connect(trigC, "Out", hat, "TrigC")
  connect(trigO, "Out", hat, "TrigO")

  connect(tune, "Out", tuneRange, "In")
  connect(tune, "Out", vpo, "In")
  connect(vpo, "Out", frequency, "Left")

  connect(f0, "Out", f0Range, "In")
  connect(f0, "Out", frequency, "Right")
  connect(frequency, "Out", hat, "Tune")

  connect(decC, "Out", decCRange, "In")
  connect(decC, "Out", hat, "DecayC")
  connect(decO, "Out", decORange, "In")
  connect(decO, "Out", hat, "DecayO")
  connect(tone, "Out", toneRange, "In")
  connect(tone, "Out", hat, "Tone")
  connect(hat, "Out", self, "Out1")

  self:addMonoBranch("trigC", trigC, "In", trigC, "Out")
  self:addMonoBranch("trigO", trigO, "In", trigO, "Out")
  self:addMonoBranch("tune", tune, "In", tune, "Out")
  self:addMonoBranch("f0", f0, "In", f0, "Out")
  self:addMonoBranch("decC", decC, "In", decC, "Out")
  self:addMonoBranch("decO", decO, "In", decO, "Out")
  self:addMonoBranch("tone", tone, "In", tone, "Out")

  if channelCount > 1 then
    connect(hat, "Out", self, "Out2")
  end
end

local views = {
  expanded = { "trigC", "trigO", "tune", "freq", "decC", "decO", "tone" },
  collapsed = {}
}

local function linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

local decCMap = linMap(0.005, 0.3, 0.05, 0.01, 0.001, 0.0005)
local decOMap = linMap(0.05, 2.0, 0.25, 0.05, 0.01, 0.001)
local toneMap = linMap(2000, 12000, 1000, 100, 10, 1)

function HiHat:onLoadViews(objects, branches)
  local controls = {}

  controls.trigC = Gate {
    button = "closed",
    description = "Closed Trigger",
    branch = branches.trigC,
    comparator = objects.trigC
  }

  controls.trigO = Gate {
    button = "open",
    description = "Open Trigger",
    branch = branches.trigO,
    comparator = objects.trigO
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
    initialBias = 263,
    gainMap = Encoder.getMap("freqGain"),
    scaling = app.octaveScaling
  }

  controls.decC = GainBias {
    button = "decC",
    description = "Closed Decay",
    branch = branches.decC,
    gainbias = objects.decC,
    range = objects.decCRange,
    biasMap = decCMap,
    biasUnits = app.unitSecs,
    initialBias = 0.04
  }

  controls.decO = GainBias {
    button = "decO",
    description = "Open Decay",
    branch = branches.decO,
    gainbias = objects.decO,
    range = objects.decORange,
    biasMap = decOMap,
    biasUnits = app.unitSecs,
    initialBias = 0.4
  }

  controls.tone = GainBias {
    button = "tone",
    description = "Tone",
    branch = branches.tone,
    gainbias = objects.tone,
    range = objects.toneRange,
    biasMap = toneMap,
    biasUnits = app.unitHertz,
    initialBias = 7000
  }

  return controls, views
end

return HiHat
