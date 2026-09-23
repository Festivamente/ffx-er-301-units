local libFestivamente = require "ffx.libFestivamente"
local libcore = require "core.libcore"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Pitch = require "Unit.ViewControl.Pitch"
local Encoder = require "Encoder"
local OptionControl = require "Unit.ViewControl.OptionControl"

local Comb = Class {}
Comb:include(Unit)

function Comb:init(args)
  args.title = "Comb"
  args.mnemonic = "cmb"
  Unit.init(self, args)
end

function Comb:onLoadGraph(channelCount)
  local comb = self:addObject("comb", libFestivamente.Comb())

  local tune = self:addObject("tune", app.ConstantOffset())
  local tuneRange = self:addObject("tuneRange", app.MinMax())
  local pitch = self:addObject("pitch", libcore.VoltPerOctave())
  local f0 = self:addObject("f0", app.GainBias())
  local f0Range = self:addObject("f0Range", app.MinMax())
  local frequency = self:addObject("frequency", app.Multiply())
  local res = self:addObject("res", app.GainBias())
  local resRange = self:addObject("resRange", app.MinMax())

  connect(self, "In1", comb, "In")
  connect(comb, "Out", self, "Out1")

  if channelCount > 1 then
    connect(self, "In2", comb, "InR")
    connect(comb, "OutR", self, "Out2")
  end

  connect(tune, "Out", tuneRange, "In")
  connect(tune, "Out", pitch, "In")
  connect(pitch, "Out", frequency, "Left")
  connect(f0, "Out", f0Range, "In")
  connect(f0, "Out", frequency, "Right")
  connect(frequency, "Out", comb, "f0")

  connect(res, "Out", resRange, "In")
  connect(res, "Out", comb, "Q")

  self.objects.comb = comb

  self:addMonoBranch("tune", tune, "In", tune, "Out")
  self:addMonoBranch("Q", res, "In", res, "Out")
  self:addMonoBranch("f0", f0, "In", f0, "Out")
end

local views = {
  expanded = {
    "tune",
    "freq",
    "type",
    "resonance"
  },
  collapsed = {}
}

function Comb:onLoadViews(objects, branches)
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
    branch = branches.f0,
    description = "Fundamental",
    gainbias = objects.f0,
    range = objects.f0Range,
    biasMap = Encoder.getMap("filterFreq"),
    biasUnits = app.unitHertz,
    initialBias = 440,
    gainMap = Encoder.getMap("freqGain"),
    scaling = app.octaveScaling
  }

  controls.type = OptionControl {
    button = "o",
    description = "Type",
    option = objects.comb:getOption("Type"),
    choices = {
      "pos+",
      "",
      "neg-"
    },
    muteOnChange = true
  }

  controls.resonance = GainBias {
    button = "Q",
    branch = branches.Q,
    description = "Resonance",
    gainbias = objects.res,
    range = objects.resRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 0.25,
    gainMap = Encoder.getMap("[-10,10]")
  }

  return controls, views
end

return Comb
