local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local OptionControl = require "Unit.ViewControl.OptionControl"
local Encoder = require "Encoder"

local Exciter = Class {}
Exciter:include(Unit)

function Exciter:init(args)
  args.title = "Exciter"
  args.mnemonic = "Exc"
  Unit.init(self, args)
end

function Exciter:onLoadGraph(channelCount)
  local axMix      = self:addObject("axMix", app.GainBias())
  local axRange    = self:addObject("axRange", app.MinMax())
  local level      = self:addObject("level", app.GainBias())
  local levelRange = self:addObject("levelRange", app.MinMax())

  connect(axMix, "Out", axRange, "In")
  connect(level, "Out", levelRange, "In")

  self:addMonoBranch("axMix", axMix, "In", axMix, "Out")
  self:addMonoBranch("level", level, "In", level, "Out")

  if channelCount == 2 then
    local exciterL = self:addObject("exciterL", libFestivamente.Exciter())
    local exciterR = self:addObject("exciterR", libFestivamente.Exciter())
    local vcaL     = self:addObject("vcaL", app.Multiply())
    local vcaR     = self:addObject("vcaR", app.Multiply())

    connect(self, "In1", exciterL, "In")
    connect(self, "In2", exciterR, "In")
    connect(axMix, "Out", exciterL, "AxMix")
    connect(axMix, "Out", exciterR, "AxMix")

    connect(level, "Out", vcaL, "Left")
    connect(level, "Out", vcaR, "Left")
    connect(exciterL, "Out", vcaL, "Right")
    connect(exciterR, "Out", vcaR, "Right")

    connect(vcaL, "Out", self, "Out1")
    connect(vcaR, "Out", self, "Out2")

    tie(exciterR, "Mode", exciterL, "Mode")
    self.objects.exciter = exciterL
  else
    local exciter = self:addObject("exciter", libFestivamente.Exciter())
    local vca     = self:addObject("vca", app.Multiply())

    connect(self, "In1", exciter, "In")
    connect(axMix, "Out", exciter, "AxMix")

    connect(level, "Out", vca, "Left")
    connect(exciter, "Out", vca, "Right")
    connect(vca, "Out", self, "Out1")

    self.objects.exciter = exciter
  end
end

local views = {
  expanded = {
    "mode",
    "axMix",
    "level"
  },
  collapsed = {}
}

function Exciter:onLoadViews(objects, branches)
  local controls = {}

  controls.mode = OptionControl {
    button = "mode",
    description = "Mode",
    option = objects.exciter:getOption("Mode"),
    choices = {
      "prev", "", "next",
      "Soft",
      "Warm",
      "Bright",
      "Rect",
      "Asym",
      "Fold"
    },
    muteOnChange = true
  }

  local oldSubReleased = controls.mode.subReleased
  function controls.mode:subReleased(i, shifted)
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

  controls.axMix = GainBias {
    button = "AxMix",
    description = "AxMix",
    branch = branches.axMix,
    gainbias = objects.axMix,
    range = objects.axRange,
    biasMap = Encoder.getMap("unit"),
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

return Exciter
