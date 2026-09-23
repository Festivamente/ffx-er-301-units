local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local OptionControl = require "Unit.ViewControl.OptionControl"

local SoftClip = Class {}
SoftClip:include(Unit)

function SoftClip:init(args)
  args.title = "SoftClip"
  args.mnemonic = "SC"
  Unit.init(self, args)
end

function SoftClip:onLoadGraph(channelCount)
  local gainControl = self:addObject("gainControl", app.GainBias())
  local level       = self:addObject("level", app.GainBias())
  local levelRange  = self:addObject("levelRange", app.MinMax())

  self:addMonoBranch("level", level, "In", level, "Out")
  self:addMonoBranch("gain", gainControl, "In", gainControl, "Out")

  if channelCount == 2 then
    local softclipL = self:addObject("softclipL", libFestivamente.SoftClip())
    local softclipR = self:addObject("softclipR", libFestivamente.SoftClip())
    local vcaL      = self:addObject("vcaL", app.Multiply())
    local vcaR      = self:addObject("vcaR", app.Multiply())

    connect(self, "In1", softclipL, "In")
    connect(self, "In2", softclipR, "In")
    connect(gainControl, "Out", softclipL, "Gain")
    connect(gainControl, "Out", softclipR, "Gain")

    connect(level, "Out", levelRange, "In")
    connect(level, "Out", vcaL, "Left")
    connect(level, "Out", vcaR, "Left")
    connect(softclipL, "Out", vcaL, "Right")
    connect(softclipR, "Out", vcaR, "Right")

    connect(vcaL, "Out", self, "Out1")
    connect(vcaR, "Out", self, "Out2")

    tie(softclipR, "Algo", softclipL, "Algo")
    self.objects.softclip = softclipL
  else
    local softclip = self:addObject("softclip", libFestivamente.SoftClip())
    local vca      = self:addObject("vca", app.Multiply())

    connect(self, "In1", softclip, "In")
    connect(gainControl, "Out", softclip, "Gain")

    connect(level, "Out", levelRange, "In")
    connect(level, "Out", vca, "Left")
    connect(softclip, "Out", vca, "Right")

    connect(vca, "Out", self, "Out1")
    self.objects.softclip = softclip
  end
end

local views = {
  expanded = { "gain", "algo", "level" },
  collapsed = {}
}

local function linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

local gainMap = linMap(0.5, 4, 0.1, 0.01, 0.001, 0.001)

function SoftClip:onLoadViews(objects, branches)
  local controls = {}

  controls.gain = GainBias {
    button = "gain",
    description = "Input Gain",
    branch = branches.gain,
    gainbias = objects.gainControl,
    biasMap = gainMap,
    units = app.unitNone,
    initialBias = 0.5
  }

  controls.algo = OptionControl {
    button = "algo",
    description = "Algorithm",
    option = objects.softclip:getOption("Algo"),
    choices = {
      "prev", "", "next",
      "Hyperbolic Tanh",
      "Sinusoidal",
      "Exponential (E=2)",
      "Two-Stage Quadratic",
      "Cubic",
      "Reciprocal"
    },
    muteOnChange = true
  }

  local oldSubReleased = controls.algo.subReleased
  function controls.algo:subReleased(i, shifted)
    if shifted then return false end

    local value = self.option:value()

    if i == 1 then
      value = math.max(4, value - 1)
      self.option:set(value)
      self:update()
      return true
    elseif i == 3 then
      value = math.min(9, value + 1)
      self.option:set(value)
      self:update()
      return true
    elseif i == 2 then
      self:update()
      return true
    else
      return oldSubReleased(self, i, shifted)
    end
  end

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

return SoftClip
