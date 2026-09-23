local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local CatControl = require "ffx.CatControl"

local Cat = Class {}
Cat:include(Unit)

function Cat:init(args)
  args.title = "Cat"
  args.mnemonic = "CAT"
  Unit.init(self, args)
end

function Cat:onLoadGraph(channelCount)
  local level = self:addObject("level", app.GainBias())
  local levelRange = self:addObject("levelRange", app.MinMax())
  local vcaL = self:addObject("vcaL", app.Multiply())

  connect(level, "Out", levelRange, "In")
  connect(level, "Out", vcaL, "Left")
  connect(self, "In1", vcaL, "Right")
  connect(vcaL, "Out", self, "Out1")

  if channelCount > 1 then
    local vcaR = self:addObject("vcaR", app.Multiply())
    connect(level, "Out", vcaR, "Left")
    connect(self, "In2", vcaR, "Right")
    connect(vcaR, "Out", self, "Out2")
  end

  self:addMonoBranch("level", level, "In", level, "Out")
end

local views = {
  expanded = { "photo", "level" },
  collapsed = {}
}

function Cat:onLoadViews(objects, branches)
  local controls = {}

  controls.photo = CatControl {}

  controls.level = GainBias {
    button = "level",
    description = "Level",
    branch = branches.level,
    gainbias = objects.level,
    range = objects.levelRange,
    initialBias = 1.0
  }

  return controls, views
end

return Cat
