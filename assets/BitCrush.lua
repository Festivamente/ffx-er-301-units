local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Encoder = require "Encoder"

local BitCrush = Class {}
BitCrush:include(Unit)

function BitCrush:init(args)
  args.title = "BitCrush"
  args.mnemonic = "BC"
  Unit.init(self, args)
end

function BitCrush:onLoadGraph(channelCount)
  local level       = self:addObject("level", app.GainBias())
  local levelRange  = self:addObject("levelRange", app.MinMax())
  local bitDepth    = self:addObject("bitDepth", app.GainBias())
  local numRange    = self:addObject("numRange", app.MinMax())
  local sampleRate  = self:addObject("sr", app.GainBias())
  local srRange     = self:addObject("srRange", app.MinMax())

  self:addMonoBranch("level", level, "In", level, "Out")
  self:addMonoBranch("bitdepth", bitDepth, "In", bitDepth, "Out")
  self:addMonoBranch("sr", sampleRate, "In", sampleRate, "Out")

  connect(level, "Out", levelRange, "In")
  connect(bitDepth, "Out", numRange, "In")
  connect(sampleRate, "Out", srRange, "In")

  if channelCount == 2 then
    local bitcrushL = self:addObject("bitcrushL", libFestivamente.BitCrush())
    local bitcrushR = self:addObject("bitcrushR", libFestivamente.BitCrush())
    local vcaL      = self:addObject("vcaL", app.Multiply())
    local vcaR      = self:addObject("vcaR", app.Multiply())

    connect(self, "In1", bitcrushL, "In")
    connect(self, "In2", bitcrushR, "In")
    connect(bitDepth, "Out", bitcrushL, "BitDepth")
    connect(bitDepth, "Out", bitcrushR, "BitDepth")
    connect(sampleRate, "Out", bitcrushL, "SR")
    connect(sampleRate, "Out", bitcrushR, "SR")

    connect(level, "Out", vcaL, "Left")
    connect(level, "Out", vcaR, "Left")
    connect(bitcrushL, "Out", vcaL, "Right")
    connect(bitcrushR, "Out", vcaR, "Right")

    connect(vcaL, "Out", self, "Out1")
    connect(vcaR, "Out", self, "Out2")

    self.objects.bitcrush = bitcrushL
  else
    local bitcrush = self:addObject("bitcrush", libFestivamente.BitCrush())
    local vca      = self:addObject("vca", app.Multiply())

    connect(self, "In1", bitcrush, "In")
    connect(bitDepth, "Out", bitcrush, "BitDepth")
    connect(sampleRate, "Out", bitcrush, "SR")
    connect(level, "Out", vca, "Left")
    connect(bitcrush, "Out", vca, "Right")
    connect(vca, "Out", self, "Out1")

    self.objects.bitcrush = bitcrush
  end
end

local views = {
  expanded = { "bitDepth", "sr" },
  collapsed = {}
}

local function linMap(min, max, superCoarse, coarse, fine, superFine)
  local map = app.LinearDialMap(min, max)
  map:setSteps(superCoarse, coarse, fine, superFine)
  return map
end

local srMap = linMap(1, 64, 1, 1, 1, 1)

function BitCrush:onLoadViews(objects, branches)
  local controls = {}

  controls.level = GainBias {
    button = "level",
    description = "Level",
    branch = branches.level,
    gainbias = objects.level,
    range = objects.levelRange,
    initialBias = 0.5
  }

  controls.bitDepth = GainBias {
    button = "bitDepth",
    description = "Bit Depth",
    branch = branches.bitdepth,
    gainbias = objects.bitDepth,
    range = objects.numRange,
    biasMap = Encoder.getMap("int[1,32]"),
    biasPrecision = 0,
    initialBias = 32
  }

  controls.sr = GainBias {
    button = "sr",
    description = "Rate Divide",
    branch = branches.sr,
    gainbias = objects.sr,
    range = objects.srRange,
    biasMap = srMap,
    biasPrecision = 0,
    initialBias = 1
  }

  return controls, views
end

return BitCrush
