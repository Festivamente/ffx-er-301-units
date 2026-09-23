local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Gate = require "Unit.ViewControl.Gate"
local Encoder = require "Encoder"

local Leslie = Class {}
Leslie:include(Unit)

function Leslie:init(args)
  args.title = "Leslie Cabinet"
  args.mnemonic = "Ls"
  Unit.init(self, args)
end

function Leslie:onLoadGraph(channelCount)
  local leslie = self:addObject("leslie", libFestivamente.Leslie())

  local fast = self:addObject("fast", app.Comparator())
  fast:setToggleMode()

  local depth      = self:addObject("depth", app.GainBias())
  local depthRange = self:addObject("depthRange", app.MinMax())
  local drive      = self:addObject("drive", app.GainBias())
  local driveRange = self:addObject("driveRange", app.MinMax())
  local blend      = self:addObject("blend", app.GainBias())
  local blendRange = self:addObject("blendRange", app.MinMax())

  connect(self, "In1", leslie, "In")
  connect(fast, "Out", leslie, "Fast")
  connect(depth, "Out", depthRange, "In")
  connect(depth, "Out", leslie, "Depth")
  connect(drive, "Out", driveRange, "In")
  connect(drive, "Out", leslie, "Drive")
  connect(blend, "Out", blendRange, "In")
  connect(blend, "Out", leslie, "Blend")

  connect(leslie, "OutL", self, "Out1")

  self:addMonoBranch("fast", fast, "In", fast, "Out")
  self:addMonoBranch("depth", depth, "In", depth, "Out")
  self:addMonoBranch("drive", drive, "In", drive, "Out")
  self:addMonoBranch("blend", blend, "In", blend, "Out")

  if channelCount > 1 then

    local leslieR = self:addObject("leslieR", libFestivamente.Leslie())
    connect(self, "In2", leslieR, "In")
    connect(fast, "Out", leslieR, "Fast")
    connect(depth, "Out", leslieR, "Depth")
    connect(drive, "Out", leslieR, "Drive")
    connect(blend, "Out", leslieR, "Blend")
    connect(leslieR, "OutR", self, "Out2")
  end
end

local views = {
  expanded = {
    "fast",
    "depth",
    "drive",
    "blend"
  },
  collapsed = {}
}

function Leslie:onLoadViews(objects, branches)
  local controls = {}

  controls.fast = Gate {
    button = "fast",
    description = "Fast/Slow",
    branch = branches.fast,
    comparator = objects.fast
  }

  controls.depth = GainBias {
    button = "depth",
    description = "Rotor Depth",
    branch = branches.depth,
    gainbias = objects.depth,
    range = objects.depthRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 0.8
  }

  controls.drive = GainBias {
    button = "drive",
    description = "Drive",
    branch = branches.drive,
    gainbias = objects.drive,
    range = objects.driveRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 0.2
  }

  controls.blend = GainBias {
    button = "blend",
    description = "Dry/Wet Blend",
    branch = branches.blend,
    gainbias = objects.blend,
    range = objects.blendRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 1.0
  }

  return controls, views
end

return Leslie
