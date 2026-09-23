local app = app
local libcore = require "core.libcore"
local libFestivamente = require "ffx.libFestivamente"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Task = require "Unit.MenuControl.Task"
local MenuHeader = require "Unit.MenuControl.Header"
local Encoder = require "Encoder"
local Utils = require "Utils"

local ReverseDelayUnit = Class {}
ReverseDelayUnit:include(Unit)

function ReverseDelayUnit:init(args)
  args.title = "Reverse Tape Delay"
  args.mnemonic = "RD"
  Unit.init(self, args)
end

function ReverseDelayUnit:onLoadGraph(channelCount)
  if channelCount > 1 then
    self:loadStereoGraph()
  else
    self:loadMonoGraph()
  end
end

function ReverseDelayUnit:loadMonoGraph()
  local delay = self:addObject("delay", libFestivamente.ReverseDelay(1))
  local secs = self:addObject("secs", app.ParameterAdapter())

  local xfade = self:addObject("xfade", app.CrossFade())
  local fader = self:addObject("fader", app.GainBias())
  local faderRange = self:addObject("faderRange", app.MinMax())

  local feedback = self:addObject("feedback", app.GainBias())
  local feedbackRange = self:addObject("feedbackRange", app.MinMax())
  local snap = self:addObject("snap", libcore.SnapToZero())
  snap:setThresholdInDecibels(-35.9)

  connect(delay, "Left Out", xfade, "A")
  tie(delay, "Delay", secs, "Out")

  connect(fader, "Out", xfade, "Fade")
  connect(fader, "Out", faderRange, "In")

  connect(snap, "Out", feedback, "In")
  connect(feedback, "Out", delay, "Feedback")
  connect(feedback, "Out", feedbackRange, "In")

  connect(self, "In1", xfade, "B")
  connect(self, "In1", delay, "Left In")
  connect(xfade, "Out", self, "Out1")

  self:addMonoBranch("delay", secs, "In", secs, "Out")
  self:addMonoBranch("wet", fader, "In", fader, "Out")
  self:addMonoBranch("feedback", snap, "In", snap, "Out")
end

function ReverseDelayUnit:loadStereoGraph()
  local delay = self:addObject("delay", libFestivamente.ReverseDelay(2))
  local secs = self:addObject("secs", app.ParameterAdapter())

  local xfade = self:addObject("xfade", app.StereoCrossFade())
  local fader = self:addObject("fader", app.GainBias())
  local faderRange = self:addObject("faderRange", app.MinMax())

  local feedback = self:addObject("feedback", app.GainBias())
  local feedbackRange = self:addObject("feedbackRange", app.MinMax())
  local snap = self:addObject("snap", libcore.SnapToZero())
  snap:setThresholdInDecibels(-35.9)

  connect(delay, "Left Out", xfade, "Left A")
  connect(delay, "Right Out", xfade, "Right A")
  tie(delay, "Delay", secs, "Out")

  connect(fader, "Out", xfade, "Fade")
  connect(fader, "Out", faderRange, "In")

  connect(snap, "Out", feedback, "In")
  connect(feedback, "Out", delay, "Feedback")
  connect(feedback, "Out", feedbackRange, "In")

  connect(self, "In1", xfade, "Left B")
  connect(self, "In1", delay, "Left In")

  connect(self, "In2", xfade, "Right B")
  connect(self, "In2", delay, "Right In")

  connect(xfade, "Left Out", self, "Out1")
  connect(xfade, "Right Out", self, "Out2")

  self:addMonoBranch("delay", secs, "In", secs, "Out")
  self:addMonoBranch("wet", fader, "In", fader, "Out")
  self:addMonoBranch("feedback", snap, "In", snap, "Out")
end

local function timeMap(min, max, n)
  local map = app.LinearDialMap(min, max)
  map:setCoarseRadix(n)
  return map
end

function ReverseDelayUnit:setMaxDelayTime(secs)
  local requested = Utils.round(secs, 1)
  local allocated = self.objects.delay:allocateTimeUpTo(requested)
  allocated = Utils.round(allocated, 1)
  if allocated > 0 then
    local minimum = self.objects.delay:minimumDelayTime()
    local map = timeMap(minimum, allocated, 100)
    self.controls.delay:setBiasMap(map)
  end
end

local menu = {
  "setHeader",
  "set200ms",
  "set2s",
  "set10s",
  "set30s",
  "wipeHeader",
  "wipe"
}

function ReverseDelayUnit:onShowMenu(objects, branches)
  local controls = {}
  local allocated = self.objects.delay:maximumDelayTime()
  allocated = Utils.round(allocated, 1)
  controls.setHeader = MenuHeader {
    description = string.format("Current Maximum Reverse Delay is %0.1fs.",
                                allocated)
  }

  controls.set200ms = Task {
    description = "0.2s",
    task = function()
      self:setMaxDelayTime(0.2)
    end
  }

  controls.set2s = Task {
    description = "2s",
    task = function()
      self:setMaxDelayTime(2)
    end
  }

  controls.set10s = Task {
    description = "10s",
    task = function()
      self:setMaxDelayTime(10)
    end
  }

  controls.set30s = Task {
    description = "30s",
    task = function()
      self:setMaxDelayTime(30)
    end
  }

  controls.wipeHeader = MenuHeader {
    description = "Tape maintenance:"
  }

  controls.wipe = Task {
    description = "Wipe the tape",
    task = function()
      self.objects.delay:zero()
    end
  }

  return controls, menu
end

local views = {
  expanded = {
    "delay",
    "feedback",
    "wet"
  },
  collapsed = {}
}

function ReverseDelayUnit:onLoadViews(objects, branches)
  local controls = {}

  controls.delay = GainBias {
    button = "delay",
    branch = branches.delay,
    description = "Reverse Delay",
    gainbias = objects.secs,
    range = objects.secs,
    biasMap = Encoder.getMap("unit"),
    biasUnits = app.unitSecs
  }

  controls.feedback = GainBias {
    button = "fdbk",
    description = "Feedback",
    branch = branches.feedback,
    gainbias = objects.feedback,
    range = objects.feedbackRange,
    biasMap = Encoder.getMap("feedback"),
    biasUnits = app.unitDecibels
  }
  controls.feedback:setTextBelow(-35.9, "-inf dB")

  controls.wet = GainBias {
    button = "wet",
    branch = branches.wet,
    description = "Wet/Dry",
    gainbias = objects.fader,
    range = objects.faderRange,
    biasMap = Encoder.getMap("unit")
  }

  return controls, views
end

function ReverseDelayUnit:onLoadFinished()
  self:setMaxDelayTime(2.0)
end

function ReverseDelayUnit:serialize()
  local t = Unit.serialize(self)
  t.maximumDelayTime = self.objects.delay:maximumDelayTime()
  return t
end

function ReverseDelayUnit:deserialize(t)
  local time = t.maximumDelayTime
  if time and time > 0 then
    self:setMaxDelayTime(time)
  end
  Unit.deserialize(self, t)
end

function ReverseDelayUnit:onRemove()
  self.objects.delay:deallocate()
  Unit.onRemove(self)
end

return ReverseDelayUnit
