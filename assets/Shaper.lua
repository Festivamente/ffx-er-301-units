local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Encoder = require "Encoder"

local Shaper = Class {}
Shaper:include(Unit)

function Shaper:init(args)
  args.title = "Shaper"
  args.mnemonic = "Sh"
  Unit.init(self, args)
end

function Shaper:onLoadGraph(channelCount)
  local shaper = self:addObject("shaper", libFestivamente.Shaper())
  local shaperR
  if channelCount > 1 then
    shaperR = self:addObject("shaperR", libFestivamente.Shaper())
  end

  local attack       = self:addObject("attack", app.GainBias())
  local attackRange  = self:addObject("attackRange", app.MinMax())
  local sustain      = self:addObject("sustain", app.GainBias())
  local sustainRange = self:addObject("sustainRange", app.MinMax())
  local level        = self:addObject("level", app.GainBias())
  local levelRange   = self:addObject("levelRange", app.MinMax())
  local vca          = self:addObject("vca", app.Multiply())

  connect(attack, "Out", attackRange, "In")
  connect(attack, "Out", shaper, "Atk")
  connect(sustain, "Out", sustainRange, "In")
  connect(sustain, "Out", shaper, "Sus")

  connect(level, "Out", levelRange, "In")
  connect(level, "Out", vca, "Left")

  connect(self, "In1", shaper, "In")
  connect(shaper, "Out", vca, "Right")
  connect(vca, "Out", self, "Out1")

  if channelCount > 1 then
    local vcaR = self:addObject("vcaR", app.Multiply())

    connect(attack, "Out", shaperR, "Atk")
    connect(sustain, "Out", shaperR, "Sus")
    connect(level, "Out", vcaR, "Left")

    connect(self, "In2", shaperR, "In")
    connect(shaperR, "Out", vcaR, "Right")
    connect(vcaR, "Out", self, "Out2")
  end

  self:addMonoBranch("attack", attack, "In", attack, "Out")
  self:addMonoBranch("sustain", sustain, "In", sustain, "Out")
  self:addMonoBranch("level", level, "In", level, "Out")
end

local views = {
  expanded = {
    "attack",
    "sustain",
    "level"
  },
  collapsed = {}
}

function Shaper:onLoadViews(objects, branches)
  local controls = {}

  controls.attack = GainBias {
    button = "attack",
    description = "Attack",
    branch = branches.attack,
    gainbias = objects.attack,
    range = objects.attackRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 0.5
  }

  controls.sustain = GainBias {
    button = "sustain",
    description = "Sustain",
    branch = branches.sustain,
    gainbias = objects.sustain,
    range = objects.sustainRange,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 0.5
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

return Shaper
