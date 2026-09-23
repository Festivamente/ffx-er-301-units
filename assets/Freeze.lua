local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"

local Freeze = Class {}
Freeze:include(Unit)

function Freeze:init(args)
  args.title = "Freeze"
  args.mnemonic = "Fz"
  Unit.init(self, args)
end

function Freeze:onLoadGraph(channelCount)
  local freezer = self:addObject("freezer", libFestivamente.Freeze())
  local freezerR
  if channelCount > 1 then
    freezerR = self:addObject("freezerR", libFestivamente.Freeze())
  end

  local freeze = self:addObject("freeze", app.Comparator())
  freeze:setGateMode()

  local size      = self:addObject("size", app.GainBias())
  local sizeRange = self:addObject("sizeRange", app.MinMax())

  connect(self, "In1", freezer, "In")
  if freezerR then
    connect(self, "In2", freezerR, "In")
  end

  connect(freeze, "Out", freezer, "Freeze")
  if freezerR then
    connect(freeze, "Out", freezerR, "Freeze")
  end

  connect(size, "Out", sizeRange, "In")
  connect(size, "Out", freezer, "Size")
  if freezerR then
    connect(size, "Out", freezerR, "Size")
  end

  connect(freezer, "Out", self, "Out1")
  if freezerR then
    connect(freezerR, "Out", self, "Out2")
  end

  self:addMonoBranch("freeze", freeze, "In", freeze, "Out")
  self:addMonoBranch("size", size, "In", size, "Out")
end

local views = {
  expanded = {
    "freeze",
    "size"
  },
  collapsed = {}
}

local function linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

local sizeMap = linMap(0.005, 1.0, 0.1, 0.01, 0.001, 0.0005)

function Freeze:onLoadViews(objects, branches)
  local controls = {}

  controls.freeze = Gate {
    button = "freeze",
    description = "Freeze",
    branch = branches.freeze,
    comparator = objects.freeze
  }

  controls.size = GainBias {
    button = "size",
    description = "Loop Size",
    branch = branches.size,
    gainbias = objects.size,
    range = objects.sizeRange,
    biasMap = sizeMap,
    biasUnits = app.unitSecs,
    initialBias = 0.25
  }

  return controls, views
end

return Freeze
