local app = app
local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Fader = require "Unit.ViewControl.Fader"
local Encoder = require "Encoder"
local libcore = require "core.libcore"

local CompressorUnit = Class {}
CompressorUnit:include(Unit)

function CompressorUnit:init(args)
  args.title = "Compressor"
  args.mnemonic = "cmp"
  Unit.init(self, args)
end

function CompressorUnit:onLoadGraph(channelCount)
  local compressorL = self:addObject("compressor", libFestivamente.CompressorObject())
  local inGainL = self:addObject("inGain", app.ConstantGain())
  local outGainL = self:addObject("outGain", app.ConstantGain())
  local envFolL = self:addObject("envFol", libcore.EnvelopeFollower())

  local attack = self:addObject("attack", app.ParameterAdapter())
  local release = self:addObject("release", app.ParameterAdapter())
  local threshold = self:addObject("threshold", app.GainBias())
  local thresholdRange = self:addObject("thresholdRange", app.MinMax())
  local ratio = self:addObject("ratio", app.GainBias())
  local ratioRange = self:addObject("ratioRange", app.MinMax())

  connect(self, "In1", inGainL, "In")
  connect(inGainL, "Out", compressorL, "In")
  connect(inGainL, "Out", envFolL, "In")
  connect(envFolL, "Out", compressorL, "SCIn")
  connect(compressorL, "Out", outGainL, "In")
  connect(outGainL, "Out", self, "Out1")

  connect(threshold, "Out", compressorL, "Threshold")
  connect(ratio, "Out", compressorL, "Ratio")
  connect(threshold, "Out", thresholdRange, "In")
  connect(ratio, "Out", ratioRange, "In")

  tie(envFolL, "Attack Time", attack, "Out")
  tie(envFolL, "Release Time", release, "Out")

  if channelCount > 1 then
    local compressorR = self:addObject("compressorR", libFestivamente.CompressorObject())
    local inGainR = self:addObject("inGainR", app.ConstantGain())
    local outGainR = self:addObject("outGainR", app.ConstantGain())
    local envFolR = self:addObject("envFolR", libcore.EnvelopeFollower())

    connect(self, "In2", inGainR, "In")
    connect(inGainR, "Out", compressorR, "In")
    connect(inGainR, "Out", envFolR, "In")
    connect(envFolR, "Out", compressorR, "SCIn")
    connect(compressorR, "Out", outGainR, "In")
    connect(outGainR, "Out", self, "Out2")

    connect(threshold, "Out", compressorR, "Threshold")
    connect(ratio, "Out", compressorR, "Ratio")
    tie(envFolR, "Attack Time", attack, "Out")
    tie(envFolR, "Release Time", release, "Out")
    tie(inGainR, "Gain", inGainL, "Gain")
    tie(outGainR, "Gain", outGainL, "Gain")
  end

  self:addMonoBranch("threshold", threshold, "In", threshold, "Out")
  self:addMonoBranch("ratio", ratio, "In", ratio, "Out")
  self:addMonoBranch("attack", attack, "In", attack, "Out")
  self:addMonoBranch("release", release, "In", release, "Out")

  inGainL:hardSet("Gain", 1.0)
  outGainL:hardSet("Gain", 1.0)
end

local views = {
  expanded = {
    "pre",
    "threshold",
    "ratio",
    "attack",
    "release",
    "post"
  },
  collapsed = {}
}

local function linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

local ratioMap = linMap(1, 20, 5, 1, 0.5, 0.1)
local attackMap = linMap(0.0005, 0.1, 0.01, 0.005, 0.001, 0.0001)
local releaseMap = linMap(0.005, 1.0, 0.1, 0.05, 0.01, 0.001)

function CompressorUnit:onLoadViews(objects, branches)
  local controls = {}

  controls.pre = Fader {
    button = "pre",
    description = "Pre-Gain",
    param = objects.inGain:getParameter("Gain"),
    map = Encoder.getMap("decibel36"),
    units = app.unitDecibels,
  }

  controls.threshold = GainBias {
    button = "Thr",
    branch = branches.threshold,
    description = "Threshold",
    gainbias = objects.threshold,
    range = objects.thresholdRange,
    biasMap = Encoder.getMap("feedback"),
    biasUnits = app.unitDecibels,
    initialBias = 0.25
  }
  controls.threshold:setTextBelow(-35.9, "-inf dB")

  controls.ratio = GainBias {
    button = "Rat",
    branch = branches.ratio,
    description = "Ratio",
    gainbias = objects.ratio,
    range = objects.ratioRange,
    biasMap = ratioMap,
    biasUnits = app.unitNone,
    initialBias = 2.0
  }

  controls.attack = GainBias {
    button = "Atk",
    branch = branches.attack,
    description = "Attack",
    gainbias = objects.attack,
    range = objects.attack,
    biasMap = attackMap,
    biasUnits = app.unitSecs,
    initialBias = 0.005
  }

  controls.release = GainBias {
    button = "Rel",
    branch = branches.release,
    description = "Release",
    gainbias = objects.release,
    range = objects.release,
    biasMap = releaseMap,
    biasUnits = app.unitSecs,
    initialBias = 0.100
  }

  controls.post = Fader {
    button = "post",
    description = "Post-Gain",
    param = objects.outGain:getParameter("Gain"),
    map = Encoder.getMap("decibel36"),
    units = app.unitDecibels,
  }

  return controls, views
end

return CompressorUnit
