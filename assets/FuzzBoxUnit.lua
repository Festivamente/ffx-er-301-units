local app = app
local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Fader = require "Unit.ViewControl.Fader"
local Encoder = require "Encoder"

local FuzzBoxUnit = Class {}
FuzzBoxUnit:include(Unit)

function FuzzBoxUnit:init(args)
  args.title = "Fuzz Box"
  args.mnemonic = "FZ"
  Unit.init(self, args)
end

function FuzzBoxUnit:onLoadGraph(channelCount)
  local fuzzL = self:addObject("fuzzL", libFestivamente.FuzzBox())
  local fuzzR
  if channelCount > 1 then
    fuzzR = self:addObject("fuzzR", libFestivamente.FuzzBox())
    tie(fuzzR, "Shape", fuzzL, "Shape")
  end

  local drive = self:addObject("drive", app.GainBias())
  local driveRange = self:addObject("driveRange", app.MinMax())
  drive:hardSet("Bias", 0.4)

  local grit = self:addObject("grit", app.GainBias())
  local gritRange = self:addObject("gritRange", app.MinMax())

  local tone = self:addObject("tone", app.GainBias())
  local toneRange = self:addObject("toneRange", app.MinMax())
  tone:hardSet("Bias", 0.7)

  local level = self:addObject("level", app.GainBias())
  local levelRange = self:addObject("levelRange", app.MinMax())
  level:hardSet("Bias", 0.5)

  local fader = self:addObject("fader", app.GainBias())
  local faderRange = self:addObject("faderRange", app.MinMax())
  fader:hardSet("Bias", 1.0)

  connect(drive, "Out", driveRange, "In")
  connect(grit, "Out", gritRange, "In")
  connect(tone, "Out", toneRange, "In")
  connect(level, "Out", levelRange, "In")
  connect(fader, "Out", faderRange, "In")

  connect(drive, "Out", fuzzL, "Drive")
  connect(grit, "Out", fuzzL, "Grit")
  connect(tone, "Out", fuzzL, "Tone")

  if channelCount > 1 then
    connect(drive, "Out", fuzzR, "Drive")
    connect(grit, "Out", fuzzR, "Grit")
    connect(tone, "Out", fuzzR, "Tone")

    local xfade = self:addObject("xfade", app.StereoCrossFade())
    local vcaL = self:addObject("vcaL", app.Multiply())
    local vcaR = self:addObject("vcaR", app.Multiply())

    connect(self, "In1", fuzzL, "In")
    connect(self, "In2", fuzzR, "In")

    connect(fuzzL, "Out", vcaL, "Left")
    connect(level, "Out", vcaL, "Right")
    connect(fuzzR, "Out", vcaR, "Left")
    connect(level, "Out", vcaR, "Right")

    connect(vcaL, "Out", xfade, "Left A")
    connect(vcaR, "Out", xfade, "Right A")
    connect(self, "In1", xfade, "Left B")
    connect(self, "In2", xfade, "Right B")
    connect(fader, "Out", xfade, "Fade")

    connect(xfade, "Left Out", self, "Out1")
    connect(xfade, "Right Out", self, "Out2")
  else
    local xfade = self:addObject("xfade", app.CrossFade())
    local vcaL = self:addObject("vcaL", app.Multiply())

    connect(self, "In1", fuzzL, "In")

    connect(fuzzL, "Out", vcaL, "Left")
    connect(level, "Out", vcaL, "Right")

    connect(vcaL, "Out", xfade, "A")
    connect(self, "In1", xfade, "B")
    connect(fader, "Out", xfade, "Fade")

    connect(xfade, "Out", self, "Out1")
  end

  self:addMonoBranch("drive", drive, "In", drive, "Out")
  self:addMonoBranch("grit", grit, "In", grit, "Out")
  self:addMonoBranch("tone", tone, "In", tone, "Out")
  self:addMonoBranch("level", level, "In", level, "Out")
  self:addMonoBranch("wet", fader, "In", fader, "Out")
end

local function shapeMap()
  local map = app.LinearDialMap(1, 3)
  map:setSteps(1, 1, 1, 1)
  return map
end

local views = {
  expanded = {
    "drive", "shape", "grit", "tone", "level", "wet"
  },
  collapsed = {}
}

function FuzzBoxUnit:onLoadViews(objects, branches)
  local controls = {}

  controls.drive = GainBias {
    button = "drive",
    description = "Drive",
    branch = branches.drive,
    gainbias = objects.drive,
    range = objects.driveRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone
  }

  controls.shape = Fader {
    button = "shape",
    description = "Shape: 1=Soft 2=Hard 3=Fuzz",
    param = objects.fuzzL:getParameter("Shape"),
    map = shapeMap(),
    units = app.unitNone,
    precision = 0
  }

  controls.grit = GainBias {
    button = "grit",
    description = "Grit (asymmetry)",
    branch = branches.grit,
    gainbias = objects.grit,
    range = objects.gritRange,
    biasMap = Encoder.getMap("[-1,1]"),
    biasUnits = app.unitNone
  }

  controls.tone = GainBias {
    button = "tone",
    description = "Tone",
    branch = branches.tone,
    gainbias = objects.tone,
    range = objects.toneRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone
  }

  controls.level = GainBias {
    button = "level",
    description = "Level",
    branch = branches.level,
    gainbias = objects.level,
    range = objects.levelRange
  }

  controls.wet = GainBias {
    button = "wet",
    description = "Wet/Dry",
    branch = branches.wet,
    gainbias = objects.fader,
    range = objects.faderRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone
  }

  return controls, views
end

return FuzzBoxUnit
