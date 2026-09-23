local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Encoder = require "Encoder"

local Flanger = Class {}
Flanger:include(Unit)

function Flanger:init(args)
  args.title = "Flanger"
  args.mnemonic = "Fl"
  Unit.init(self, args)
end

function Flanger:onLoadGraph(channelCount)
  local flanger = self:addObject("flanger", libFestivamente.Flanger())

  local rate       = self:addObject("rate", app.GainBias())
  local rateRange  = self:addObject("rateRange", app.MinMax())
  local depth      = self:addObject("depth", app.GainBias())
  local depthRange = self:addObject("depthRange", app.MinMax())
  local fdbk       = self:addObject("fdbk", app.GainBias())
  local fdbkRange  = self:addObject("fdbkRange", app.MinMax())
  local blend      = self:addObject("blend", app.GainBias())
  local blendRange = self:addObject("blendRange", app.MinMax())

  connect(self, "In1", flanger, "In")
  connect(rate, "Out", rateRange, "In")
  connect(rate, "Out", flanger, "Rate")
  connect(depth, "Out", depthRange, "In")
  connect(depth, "Out", flanger, "Depth")
  connect(fdbk, "Out", fdbkRange, "In")
  connect(fdbk, "Out", flanger, "Feedback")
  connect(blend, "Out", blendRange, "In")
  connect(blend, "Out", flanger, "Blend")

  connect(flanger, "Out", self, "Out1")

  self:addMonoBranch("rate", rate, "In", rate, "Out")
  self:addMonoBranch("depth", depth, "In", depth, "Out")
  self:addMonoBranch("fdbk", fdbk, "In", fdbk, "Out")
  self:addMonoBranch("blend", blend, "In", blend, "Out")

  if channelCount > 1 then
    connect(self, "In2", flanger, "InR")
    connect(flanger, "OutR", self, "Out2")
  end
end

local views = {
  expanded = {
    "rate",
    "depth",
    "fdbk",
    "blend"
  },
  collapsed = {}
}

local function linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

local rateMap = linMap(0.01, 10.0, 1.0, 0.1, 0.01, 0.001)
local fdbkMap = linMap(-0.95, 0.95, 0.25, 0.05, 0.01, 0.001)

function Flanger:onLoadViews(objects, branches)
  local controls = {}

  controls.rate = GainBias {
    button = "rate",
    description = "LFO Rate",
    branch = branches.rate,
    gainbias = objects.rate,
    range = objects.rateRange,
    biasMap = rateMap,
    biasUnits = app.unitHertz,
    initialBias = 0.25
  }

  controls.depth = GainBias {
    button = "depth",
    description = "Sweep Depth",
    branch = branches.depth,
    gainbias = objects.depth,
    range = objects.depthRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 0.7
  }

  controls.fdbk = GainBias {
    button = "fdbk",
    description = "Feedback",
    branch = branches.fdbk,
    gainbias = objects.fdbk,
    range = objects.fdbkRange,
    biasMap = fdbkMap,
    biasUnits = app.unitNone,
    initialBias = 0.4
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

  return controls, views
end

return Flanger
