local app = app
local libcore = require "core.libcore"
local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local Pitch = require "Unit.ViewControl.Pitch"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"
local Encoder = require "Encoder"

local Kick = Class {}
Kick:include(Unit)

function Kick:init(args)
  args.title = "Kick"
  args.mnemonic = "Kk"
  Unit.init(self, args)
end

function Kick:onLoadGraph(channelCount)
  local kick = self:addObject("kick", libFestivamente.Kick())

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
  local punch      = self:addObject("punch", app.GainBias())
  local punchRange = self:addObject("punchRange", app.MinMax())
  local click      = self:addObject("click", app.GainBias())
  local clickRange = self:addObject("clickRange", app.MinMax())
  local drive      = self:addObject("drive", app.GainBias())
  local driveRange = self:addObject("driveRange", app.MinMax())

  connect(trig, "Out", kick, "Trig")

  connect(tune, "Out", tuneRange, "In")
  connect(tune, "Out", vpo, "In")
  connect(vpo, "Out", frequency, "Left")

  connect(f0, "Out", f0Range, "In")
  connect(f0, "Out", frequency, "Right")
  connect(frequency, "Out", kick, "Tune")

  connect(decay, "Out", decayRange, "In")
  connect(decay, "Out", kick, "Decay")
  connect(punch, "Out", punchRange, "In")
  connect(punch, "Out", kick, "Punch")
  connect(click, "Out", clickRange, "In")
  connect(click, "Out", kick, "Click")
  connect(drive, "Out", driveRange, "In")
  connect(drive, "Out", kick, "Drive")
  connect(kick, "Out", self, "Out1")

  self:addMonoBranch("trig", trig, "In", trig, "Out")
  self:addMonoBranch("tune", tune, "In", tune, "Out")
  self:addMonoBranch("f0", f0, "In", f0, "Out")
  self:addMonoBranch("decay", decay, "In", decay, "Out")
  self:addMonoBranch("punch", punch, "In", punch, "Out")
  self:addMonoBranch("click", click, "In", click, "Out")
  self:addMonoBranch("drive", drive, "In", drive, "Out")

  if channelCount > 1 then
    connect(kick, "Out", self, "Out2")
  end
end

local views = {
  expanded = { "trig", "tune", "freq", "decay", "punch", "click", "drive" },
  collapsed = {}
}

local function linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

local decayMap = linMap(0.03, 3.0, 0.5, 0.05, 0.01, 0.001)

function Kick:onLoadViews(objects, branches)
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
    initialBias = 50,
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
    initialBias = 0.4
  }

  controls.punch = GainBias {
    button = "punch",
    description = "Pitch Punch",
    branch = branches.punch,
    gainbias = objects.punch,
    range = objects.punchRange,
    biasMap = Encoder.getMap("[0,1]"),
    initialBias = 0.5
  }

  controls.click = GainBias {
    button = "click",
    description = "Click",
    branch = branches.click,
    gainbias = objects.click,
    range = objects.clickRange,
    biasMap = Encoder.getMap("[0,1]"),
    initialBias = 0.3
  }

  controls.drive = GainBias {
    button = "drive",
    description = "Drive",
    branch = branches.drive,
    gainbias = objects.drive,
    range = objects.driveRange,
    biasMap = Encoder.getMap("[0,1]"),
    initialBias = 0.25
  }

  return controls, views
end

return Kick
