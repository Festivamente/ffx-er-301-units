local app = app
local libFestivamente = require "ffx.libFestivamente"
local libcore = require "core.libcore"
local Class = require "Base.Class"
local Unit = require "Unit"
local SamplePool = require "Sample.Pool"
local SamplePoolInterface = require "Sample.Pool.Interface"
local SampleEditor = require "Sample.Editor"
local Encoder = require "Encoder"
local Fader = require "Unit.ViewControl.Fader"
local Gate = require "Unit.ViewControl.Gate"
local Pitch = require "Unit.ViewControl.Pitch"
local GainBias = require "Unit.ViewControl.GainBias"
local Task = require "Unit.MenuControl.Task"
local MenuHeader = require "Unit.MenuControl.Header"

local Morpho = Class {}
Morpho:include(Unit)

function Morpho:init(args)
  args.title = "Morpho"
  args.mnemonic = "MO"
  Unit.init(self, args)
end

function Morpho:onLoadGraph(channelCount)
  local head = self:addObject("head", libFestivamente.MorphoHead(channelCount))

  local trig = self:addObject("trig", app.Comparator())
  connect(trig, "Out", head, "Trigger")
  self:addMonoBranch("trig", trig, "In", trig, "Out")

  local tune = self:addObject("tune", app.ConstantOffset())
  local pitch = self:addObject("pitch", libcore.VoltPerOctave())
  local multiply = self:addObject("multiply", app.Multiply())
  local clipper = self:addObject("clipper", libcore.Clipper())
  clipper:setMaximum(64.0)
  clipper:setMinimum(-64.0)
  local speed = self:addObject("speed", app.GainBias())
  speed:hardSet("Bias", 1.0)
  local tuneRange = self:addObject("tuneRange", app.MinMax())
  local speedRange = self:addObject("speedRange", app.MinMax())
  connect(tune, "Out", pitch, "In")
  connect(tune, "Out", tuneRange, "In")
  connect(pitch, "Out", multiply, "Left")
  connect(speed, "Out", multiply, "Right")
  connect(speed, "Out", speedRange, "In")
  connect(multiply, "Out", clipper, "In")
  connect(clipper, "Out", head, "Speed")
  self:addMonoBranch("speed", speed, "In", speed, "Out")
  self:addMonoBranch("tune", tune, "In", tune, "Out")

  local size = self:addObject("size", app.ParameterAdapter())
  local slide = self:addObject("slide", app.ParameterAdapter())
  local organize = self:addObject("organize", app.ParameterAdapter())
  local morph = self:addObject("morph", app.ParameterAdapter())
  local gain = self:addObject("gain", app.ParameterAdapter())
  gain:hardSet("Bias", 1.0)
  tie(head, "Gene Size", size, "Out")
  tie(head, "Slide", slide, "Out")
  tie(head, "Organize", organize, "Out")
  tie(head, "Morph", morph, "Out")
  tie(head, "Gain", gain, "Out")
  self:addMonoBranch("size", size, "In", size, "Out")
  self:addMonoBranch("slide", slide, "In", slide, "Out")
  self:addMonoBranch("organize", organize, "In", organize, "Out")
  self:addMonoBranch("morph", morph, "In", morph, "Out")
  self:addMonoBranch("gain", gain, "In", gain, "Out")

  connect(head, "Left Out", self, "Out1")
  if channelCount > 1 then
    connect(head, "Right Out", self, "Out2")
  end
end

function Morpho:serialize()
  local t = Unit.serialize(self)
  local sample = self.sample
  if sample then
    t.sample = SamplePool.serializeSample(sample)
  end
  return t
end

function Morpho:deserialize(t)
  Unit.deserialize(self, t)
  if t.sample then
    local sample = SamplePool.deserializeSample(t.sample, self.chain)
    if sample then
      self:setSample(sample)
    else
      local Utils = require "Utils"
      app.logError("%s:deserialize: failed to load sample.", self)
      Utils.pp(t.sample)
    end
  end
end

function Morpho:setSample(sample)
  if self.sample then
    self.sample:release(self)
    self.sample = nil
  end

  if sample == nil or sample:getChannelCount() == 0 then
    self.objects.head:setSample(nil)
  else
    self.objects.head:setSample(sample.pSample)
    self.sample = sample
    self.sample:claim(self)
  end

  if self.sampleEditor then
    self.sampleEditor:setSample(sample)
  end
  self:notifyControls("setSample", sample)
end

function Morpho:doDetachSample()
  local Overlay = require "Overlay"
  Overlay.flashMainMessage("Sample detached.")
  self:setSample()
end

function Morpho:doAttachSampleFromCard()
  local task = function(sample)
    if sample then
      local Overlay = require "Overlay"
      Overlay.flashMainMessage("Attached sample: %s", sample.name)
      self:setSample(sample)
    end
  end
  local Pool = require "Sample.Pool"
  Pool.chooseFileFromCard(self.loadInfo.id, task)
end

function Morpho:doAttachSampleFromPool()
  local chooser = SamplePoolInterface(self.loadInfo.id, "choose")
  chooser:setDefaultChannelCount(self.channelCount)
  chooser:highlight(self.sample)
  local task = function(sample)
    if sample then
      local Overlay = require "Overlay"
      Overlay.flashMainMessage("Attached sample: %s", sample.name)
      self:setSample(sample)
    end
  end
  chooser:subscribe("done", task)
  chooser:show()
end

function Morpho:showSampleEditor()
  if self.sample then
    if self.sampleEditor == nil then
      self.sampleEditor = SampleEditor(self, self.objects.head)
      self.sampleEditor:setSample(self.sample)
      self.sampleEditor:setPointerLabel("G")
    end
    self.sampleEditor:show()
  else
    local Overlay = require "Overlay"
    Overlay.flashMainMessage("You must first select a sample.")
  end
end

local menu = {
  "sampleHeader",
  "selectFromCard",
  "selectFromPool",
  "detachBuffer",
  "editSample"
}

function Morpho:onShowMenu(objects, branches)
  local controls = {}

  controls.sampleHeader = MenuHeader {
    description = "Sample Menu"
  }

  controls.selectFromCard = Task {
    description = "Select from Card",
    task = function()
      self:doAttachSampleFromCard()
    end
  }

  controls.selectFromPool = Task {
    description = "Select from Pool",
    task = function()
      self:doAttachSampleFromPool()
    end
  }

  controls.detachBuffer = Task {
    description = "Detach Buffer",
    task = function()
      self:doDetachSample()
    end
  }

  controls.editSample = Task {
    description = "Edit Buffer",
    task = function()
      self:showSampleEditor()
    end
  }

  local sub = {}
  if self.sample then
    sub[1] = {
      position = app.GRID5_LINE1,
      justify = app.justifyLeft,
      text = "Attached Sample:"
    }
    sub[2] = {
      position = app.GRID5_LINE2,
      justify = app.justifyLeft,
      text = "+ " .. self.sample:getFilenameForDisplay(24)
    }
    sub[3] = {
      position = app.GRID5_LINE3,
      justify = app.justifyLeft,
      text = "+ " .. self.sample:getDurationText()
    }
    sub[4] = {
      position = app.GRID5_LINE4,
      justify = app.justifyLeft,
      text = string.format("+ %s %s %s", self.sample:getChannelText(),
                           self.sample:getSampleRateText(),
                           self.sample:getMemorySizeText())
    }
  else
    sub[1] = {
      position = app.GRID5_LINE3,
      justify = app.justifyCenter,
      text = "No sample attached."
    }
  end

  return controls, menu, sub
end

local views = {
  expanded = {
    "trigger",
    "pitch",
    "speed",
    "size",
    "slide",
    "organize",
    "morph",
    "splices",
    "gain"
  },
  collapsed = {}
}

function Morpho:onLoadViews(objects, branches)
  local controls = {}

  controls.trigger = Gate {
    button = "play",
    description = "Play / Retrigger",
    branch = branches.trig,
    comparator = objects.trig
  }

  controls.pitch = Pitch {
    button = "V/oct",
    description = "V/oct",
    branch = branches.tune,
    offset = objects.tune,
    range = objects.tuneRange
  }

  controls.speed = GainBias {
    button = "speed",
    description = "Vari-Speed",
    branch = branches.speed,
    gainbias = objects.speed,
    range = objects.speedRange,
    biasMap = Encoder.getMap("speed"),
    biasUnits = app.unitMultiplier,
    initialBias = 1.0
  }

  controls.size = GainBias {
    button = "gene",
    description = "Gene Size",
    branch = branches.size,
    gainbias = objects.size,
    range = objects.size,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitSecs,
    initialBias = 0.0
  }

  controls.slide = GainBias {
    button = "slide",
    description = "Slide",
    branch = branches.slide,
    gainbias = objects.slide,
    range = objects.slide,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 0.0
  }

  controls.organize = GainBias {
    button = "org",
    description = "Organize",
    branch = branches.organize,
    gainbias = objects.organize,
    range = objects.organize,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 0.0
  }

  controls.morph = GainBias {
    button = "morph",
    description = "Morph",
    branch = branches.morph,
    gainbias = objects.morph,
    range = objects.morph,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitNone,
    initialBias = 0.0
  }

  controls.splices = Fader {
    button = "splices",
    description = "Splice Count",
    param = objects.head:getParameter("Splices"),
    map = Encoder.getMap("int[1,32]")
  }

  controls.gain = GainBias {
    button = "gain",
    description = "Gain",
    branch = branches.gain,
    gainbias = objects.gain,
    range = objects.gain,
    initialBias = 1.0
  }

  return controls, views
end

function Morpho:onRemove()
  self:setSample(nil)
  Unit.onRemove(self)
end

return Morpho
