local app = app
local libFestivamente = require "ffx.libFestivamente"
local libcore = require "core.libcore"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"
local Pitch = require "Unit.ViewControl.Pitch"
local Encoder = require "Encoder"

local TapDelay = Class {}
TapDelay:include(Unit)

function TapDelay:init(args)
  args.title = "Tap Delay"
  args.mnemonic = "TD"
  Unit.init(self, args)
end

function TapDelay:onLoadGraph(channelCount)
  local tapdelay = self:addObject("tapdelay", libFestivamente.TapDelay())
  local tapdelayR
  local stereoMixer
  if channelCount > 1 then
    tapdelayR = self:addObject("tapdelayR", libFestivamente.TapDelay())
    stereoMixer = self:addObject("stereoMixer", libFestivamente.TapDelayStereoMixer())
  end

  local time = self:addObject("time", app.GainBias())
  local timeRange = self:addObject("timeRange", app.MinMax())
  local fdbk = self:addObject("fdbk", app.GainBias())
  local fdbkRange = self:addObject("fdbkRange", app.MinMax())
  local blend = self:addObject("blend", app.GainBias())
  local blendRange = self:addObject("blendRange", app.MinMax())

  local rev = self:addObject("rev", app.Comparator())
  rev:setToggleMode()

  connect(self, "In1", tapdelay, "In")
  if tapdelayR then
    connect(self, "In2", tapdelayR, "In")
    connect(self, "In1", stereoMixer, "DryL")
    connect(self, "In2", stereoMixer, "DryR")
    for t = 1, 4 do
      local suffix = tostring(t)
      connect(tapdelay, "Tap" .. suffix, stereoMixer, "Tap" .. suffix .. "L")
      connect(tapdelayR, "Tap" .. suffix, stereoMixer, "Tap" .. suffix .. "R")
    end
  end

  connect(time, "Out", timeRange, "In")
  connect(time, "Out", tapdelay, "Time")
  if tapdelayR then
    connect(time, "Out", tapdelayR, "Time")
  end

  connect(fdbk, "Out", fdbkRange, "In")
  connect(fdbk, "Out", tapdelay, "Feedback")
  if tapdelayR then
    connect(fdbk, "Out", tapdelayR, "Feedback")
  end

  connect(rev, "Out", tapdelay, "Reverse")
  if tapdelayR then
    connect(rev, "Out", tapdelayR, "Reverse")
  end

  connect(blend, "Out", blendRange, "In")
  if stereoMixer then
    connect(blend, "Out", stereoMixer, "Blend")
    connect(stereoMixer, "OutL", self, "Out1")
    connect(stereoMixer, "OutR", self, "Out2")
  else
    connect(blend, "Out", tapdelay, "Blend")
    connect(tapdelay, "Out", self, "Out1")
  end

  self:addMonoBranch("time", time, "In", time, "Out")
  self:addMonoBranch("fdbk", fdbk, "In", fdbk, "Out")
  self:addMonoBranch("rev", rev, "In", rev, "Out")
  self:addMonoBranch("blend", blend, "In", blend, "Out")

  for t = 1, 4 do
    local suffix = tostring(t)

    local tune = self:addObject("tune" .. suffix, app.ConstantOffset())
    local tuneRange = self:addObject("tune" .. suffix .. "Range", app.MinMax())
    local pitch = self:addObject("pitchCV" .. suffix, libcore.VoltPerOctave())
    local multiply = self:addObject("pitchMultiply" .. suffix, app.Multiply())
    local clipper = self:addObject("pitchClipper" .. suffix, libcore.Clipper())

    local f0 = self:addObject("pitch" .. suffix, app.GainBias())
    local f0Range = self:addObject("pitch" .. suffix .. "Range", app.MinMax())
    f0:hardSet("Bias", 1.0)

    clipper:setMinimum(-4.0)
    clipper:setMaximum(4.0)

    connect(tune, "Out", tuneRange, "In")
    connect(tune, "Out", pitch, "In")
    connect(pitch, "Out", multiply, "Left")
    connect(f0, "Out", multiply, "Right")
    connect(f0, "Out", f0Range, "In")
    connect(multiply, "Out", clipper, "In")
    connect(clipper, "Out", tapdelay, "Pitch" .. suffix)
    if tapdelayR then
      connect(clipper, "Out", tapdelayR, "Pitch" .. suffix)
    end

    self:addMonoBranch("tune" .. suffix, tune, "In", tune, "Out")
    self:addMonoBranch("pitch" .. suffix, f0, "In", f0, "Out")

    local lvl = self:addObject("lvl" .. suffix, app.GainBias())
    local lvlRange = self:addObject("lvl" .. suffix .. "Range", app.MinMax())

    connect(lvl, "Out", lvlRange, "In")

    if stereoMixer then
      local pan = self:addObject("pan" .. suffix, app.GainBias())
      local panRange = self:addObject("pan" .. suffix .. "Range", app.MinMax())
      connect(pan, "Out", panRange, "In")
      connect(lvl, "Out", stereoMixer, "Level" .. suffix)
      connect(pan, "Out", stereoMixer, "Pan" .. suffix)
      self:addMonoBranch("pan" .. suffix, pan, "In", pan, "Out")
    else
      connect(lvl, "Out", tapdelay, "Level" .. suffix)
    end

    self:addMonoBranch("lvl" .. suffix, lvl, "In", lvl, "Out")
  end
end

local monoViews = {
  expanded = {
    "time",
    "v1", "f1", "l1",
    "v2", "f2", "l2",
    "v3", "f3", "l3",
    "v4", "f4", "l4",
    "fdbk",
    "rev",
    "blend"
  },
  collapsed = {}
}

local stereoViews = {
  expanded = {
    "time",
    "v1", "f1", "l1", "p1",
    "v2", "f2", "l2", "p2",
    "v3", "f3", "l3", "p3",
    "v4", "f4", "l4", "p4",
    "fdbk",
    "rev",
    "blend"
  },
  collapsed = {}
}

local function linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

local timeMap = linMap(0.02, 0.5, 0.1, 0.01, 0.001, 0.0005)
local fdbkMap = linMap(0, 0.95, 0.25, 0.05, 0.01, 0.001)
local panMap = linMap(-1.0, 1.0, 0.5, 0.1, 0.01, 0.001)
local tapLevelDefaults = {1.0, 0.75, 0.55, 0.4}

function TapDelay:onLoadViews(objects, branches)
  local controls = {}

  controls.time = GainBias {
    button = "time",
    description = "Tap Spacing",
    branch = branches.time,
    gainbias = objects.time,
    range = objects.timeRange,
    biasMap = timeMap,
    biasUnits = app.unitSecs,
    initialBias = 0.15
  }

  for t = 1, 4 do
    local suffix = tostring(t)

    controls["v" .. suffix] = Pitch {
      button = "V/oct" .. suffix,
      description = "Tap " .. suffix .. " V/oct",
      branch = branches["tune" .. suffix],
      offset = objects["tune" .. suffix],
      range = objects["tune" .. suffix .. "Range"]
    }

    controls["f" .. suffix] = GainBias {
      button = "f0" .. suffix,
      description = "Tap " .. suffix .. " Base Ratio",
      branch = branches["pitch" .. suffix],
      gainbias = objects["pitch" .. suffix],
      range = objects["pitch" .. suffix .. "Range"],
      biasMap = Encoder.getMap("speed"),
      biasUnits = app.unitMultiplier,
      initialBias = 1.0
    }

    controls["l" .. suffix] = GainBias {
      button = "l" .. suffix,
      description = "Tap " .. suffix .. " Level",
      branch = branches["lvl" .. suffix],
      gainbias = objects["lvl" .. suffix],
      range = objects["lvl" .. suffix .. "Range"],
      initialBias = tapLevelDefaults[t]
    }

    if objects["pan" .. suffix] then
      controls["p" .. suffix] = GainBias {
        button = "pan" .. suffix,
        description = "Tap " .. suffix .. " Pan",
        branch = branches["pan" .. suffix],
        gainbias = objects["pan" .. suffix],
        range = objects["pan" .. suffix .. "Range"],
        biasMap = panMap,
        biasUnits = app.unitNone,
        initialBias = 0.0
      }
    end
  end

  controls.fdbk = GainBias {
    button = "fdbk",
    description = "Feedback (via Tap 4 pitch)",
    branch = branches.fdbk,
    gainbias = objects.fdbk,
    range = objects.fdbkRange,
    biasMap = fdbkMap,
    biasUnits = app.unitNone,
    initialBias = 0.3
  }

  controls.rev = Gate {
    button = "rev",
    description = "Reverse",
    branch = branches.rev,
    comparator = objects.rev
  }

  controls.blend = GainBias {
    button = "blend",
    description = "Dry/Wet Blend",
    branch = branches.blend,
    gainbias = objects.blend,
    range = objects.blendRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 0.5
  }

  local views = objects.stereoMixer and stereoViews or monoViews
  return controls, views
end

return TapDelay
