local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"
local libcore = require "core.libcore"
local Encoder = require "Encoder"
local Pitch = require "Unit.ViewControl.Pitch"

local Pluck = Class {}
Pluck:include(Unit)

function Pluck:init(args)
  args.title = "Pluck"
  args.mnemonic = "Pk6"
  Unit.init(self, args)
end

function Pluck:createControl(name, type)
  local control = self:addObject(name, type)
  local controlRange = self:addObject(name .. "Range", app.MinMax())
  connect(control, "Out", controlRange, "In")
  self:addMonoBranch(name, control, "In", control, "Out")
  return control
end

function Pluck:createAdapterControl(name)
  local adapter = self:addObject(name, app.ParameterAdapter())
  self:addMonoBranch(name, adapter, "In", adapter, "Out")
  return adapter
end

function Pluck:onLoadGraph(channelCount)

  local pluck = self:addObject("pluck", libFestivamente.Pluck(0.5))

  local exciter    = self:addObject("exciter", libcore.WhiteNoise())
  local vcaexc     = self:addObject("vcaexc", app.Multiply())
  local comparator = self:addObject("comparator", app.Comparator())
  local env        = self:addObject("env", libcore.SkewedSineEnvelope())
  env:hardSet("Duration", 0.005)
  env:hardSet("Skew", -1.0)
  local ampl = self:addObject("ampl", app.Constant())
  ampl:hardSet("Value", 1.0)

  connect(comparator, "Out", env, "Trigger")

  connect(comparator, "Out", pluck, "Trigger")
  connect(ampl, "Out", env, "Level")
  connect(exciter, "Out", vcaexc, "Right")
  connect(env, "Out", vcaexc, "Left")
  connect(vcaexc, "Out", pluck, "In")

  local tune      = self:createControl("tune", app.ConstantOffset())
  local f0        = self:createAdapterControl("f0")
  local pitch     = self:addObject("pitch", libcore.VoltPerOctave())
  local frequency = self:addObject("frequency", app.ConstantGain())

  connect(tune, "Out", pitch, "In")
  connect(pitch, "Out", frequency, "In")
  tie(frequency, "Gain", f0, "Out")
  connect(frequency, "Out", pluck, "f0")

  local damp = self:createControl("damp", app.GainBias())
  local fdbk = self:createControl("fdbk", app.GainBias())
  connect(damp, "Out", pluck, "Damp")
  connect(fdbk, "Out", pluck, "Decay")

  local level      = self:addObject("level", app.GainBias())
  local levelRange = self:addObject("levelRange", app.MinMax())
  local vca        = self:addObject("vca", app.Multiply())

  connect(level, "Out", levelRange, "In")
  connect(level, "Out", vca, "Left")
  connect(pluck, "Out", vca, "Right")
  connect(vca, "Out", self, "Out1")

  self:addMonoBranch("trig", comparator, "In", comparator, "Out")
  self:addMonoBranch("level", level, "In", level, "Out")

  if channelCount > 1 then
    connect(vca, "Out", self, "Out2")
  end
end

local function freqMap(from, to, F0, step)
  local n = 0
  for x = from, to, step do n = n + 1 end
  local map = app.LUTDialMap(n)
  for x = from, to, step do map:add((2 ^ x) * F0) end
  return map
end

local views = {
  expanded = {
    "tune",
    "freq",
    "damp",
    "trig",
    "fdbk",
    "level"
  },

  collapsed = {}
}

function Pluck:onLoadViews(objects, branches)
  local controls = {}

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
    range = objects.f0,
    biasMap = freqMap(-3, 3, 27.5, 1.0 / 12),
    biasUnits = app.unitHertz,
    initialBias = 27.5,
    gainMap = Encoder.getMap("freqGain"),
    scaling = app.octaveScaling
  }

  controls.damp = GainBias {
    button = "damp",
    description = "Damping",
    branch = branches.damp,
    gainbias = objects.damp,
    range = objects.dampRange,
    biasMap = Encoder.getMap("filterFreq"),
    biasUnits = app.unitHertz,
    initialBias = 3000,
    gainMap = Encoder.getMap("freqGain"),
    scaling = app.octaveScaling
  }

  controls.trig = Gate {
    button = "trig",
    description = "Gate",
    branch = branches.trig,
    comparator = objects.comparator
  }

  controls.fdbk = GainBias {
    button = "fdbk",
    description = "Decay Time",
    branch = branches.fdbk,
    gainbias = objects.fdbk,
    range = objects.fdbkRange,
    biasMap = Encoder.getMap("unit"),
    initialBias = 0.9
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

return Pluck
