local app = app
local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Encoder = require "Encoder"

local BandSpreader = Class {}
BandSpreader:include(Unit)

function BandSpreader:init(args)
  args.title = "Band Spreader"
  args.mnemonic = "BS"
  Unit.init(self, args)
end

function BandSpreader:onLoadGraph(channelCount)
  if channelCount ~= 2 then
    app.logError("Band Spreader can only load into a stereo chain.")
  end

  local spreader = self:addObject("spreader", libFestivamente.BandSpreader())

  connect(self, "In1", spreader, "InL")
  connect(self, "In2", spreader, "InR")
  connect(spreader, "OutL", self, "Out1")
  connect(spreader, "OutR", self, "Out2")

  for band = 1, 6 do
    local panName = "pan" .. band
    local pan = self:addObject(panName, app.GainBias())
    local panRange = self:addObject(panName .. "Range", app.MinMax())

    connect(pan, "Out", panRange, "In")
    connect(pan, "Out", spreader, "Pan" .. band)
    self:addMonoBranch(panName, pan, "In", pan, "Out")

    local levelName = "lvl" .. band
    local level = self:addObject(levelName, app.GainBias())
    local levelRange = self:addObject(levelName .. "Range", app.MinMax())

    connect(level, "Out", levelRange, "In")
    connect(level, "Out", spreader, "Level" .. band)
    self:addMonoBranch(levelName, level, "In", level, "Out")
  end
end

local views = {
  expanded = {
    "pan1", "lvl1",
    "pan2", "lvl2",
    "pan3", "lvl3",
    "pan4", "lvl4",
    "pan5", "lvl5",
    "pan6", "lvl6"
  },
  collapsed = {}
}

local bandNames = {
  "Low <120Hz",
  "LoMid 120-350",
  "Mid 350-1k",
  "HiMid 1k-2.8k",
  "Pres 2.8k-8k",
  "Air >8k"
}

function BandSpreader:onLoadViews(objects, branches)
  local controls = {}

  for band = 1, 6 do
    local panName = "pan" .. band
    controls[panName] = GainBias {
      button = panName,
      description = bandNames[band] .. " Pan",
      branch = branches[panName],
      gainbias = objects[panName],
      range = objects[panName .. "Range"],
      biasMap = Encoder.getMap("[-1,1]"),
      initialBias = 0.0
    }

    local levelName = "lvl" .. band
    controls[levelName] = GainBias {
      button = levelName,
      description = bandNames[band] .. " Level",
      branch = branches[levelName],
      gainbias = objects[levelName],
      range = objects[levelName .. "Range"],
      biasMap = Encoder.getMap("unit"),
      initialBias = 1.0
    }
  end

  return controls, views
end

return BandSpreader
