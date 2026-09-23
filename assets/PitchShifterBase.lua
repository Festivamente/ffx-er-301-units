local app = app
local libFestivamente = require "ffx.libFestivamente"
local libcore = require "core.libcore"
local Class = require "Base.Class"
local Unit = require "Unit"
local GainBias = require "Unit.ViewControl.GainBias"
local Pitch = require "Unit.ViewControl.Pitch"
local Encoder = require "Encoder"

local function build(voiceCount)
  local PitchShifter = Class {}
  PitchShifter:include(Unit)

  function PitchShifter:init(args)
    if voiceCount == 1 then
      args.title = "Pitch Shifter"
      args.mnemonic = "PS"
    else
      args.title = string.format("Pitch Shifter (%d voices)", voiceCount)
      args.mnemonic = "PS" .. voiceCount
    end
    Unit.init(self, args)
  end

  function PitchShifter:onLoadGraph(channelCount)
    local shifter = libFestivamente.PitchShifter()
    shifter:setVoiceCount(voiceCount)
    shifter = self:addObject("shifter", shifter)

    local shifterR
    if channelCount > 1 then
      shifterR = libFestivamente.PitchShifter()
      shifterR:setVoiceCount(voiceCount)
      shifterR = self:addObject("shifterR", shifterR)
    end

    connect(self, "In1", shifter, "In")
    if shifterR then
      connect(self, "In2", shifterR, "In")
    end

    for v = 1, voiceCount do
      local oneVoice = voiceCount == 1
      local suffix = oneVoice and "" or tostring(v)

      local tuneName = "tune" .. suffix
      local tuneRangeName = "tuneRange" .. suffix
      local pitchName = "pitch" .. suffix
      local multiplyName = "multiply" .. suffix
      local clipperName = "clipper" .. suffix
      local speedName = oneVoice and "shift" or ("speed" .. suffix)
      local speedRangeName = oneVoice and "shiftRange"
          or ("speedRange" .. suffix)
      local speedBranchName = speedName

      local tune = self:addObject(tuneName, app.ConstantOffset())
      local tuneRange = self:addObject(tuneRangeName, app.MinMax())
      local pitch = self:addObject(pitchName, libcore.VoltPerOctave())
      local multiply = self:addObject(multiplyName, app.Multiply())
      local clipper = self:addObject(clipperName, libcore.Clipper())
      local speed = self:addObject(speedName, app.GainBias())
      local speedRange = self:addObject(speedRangeName, app.MinMax())

      clipper:setMaximum(4.0)
      clipper:setMinimum(-4.0)
      speed:hardSet("Bias", 1.0)

      connect(tune, "Out", pitch, "In")
      connect(tune, "Out", tuneRange, "In")
      connect(pitch, "Out", multiply, "Left")
      connect(speed, "Out", multiply, "Right")
      connect(speed, "Out", speedRange, "In")
      connect(multiply, "Out", clipper, "In")
      connect(clipper, "Out", shifter, "Speed" .. v)
      if shifterR then
        connect(clipper, "Out", shifterR, "Speed" .. v)
      end

      self:addMonoBranch(tuneName, tune, "In", tune, "Out")
      self:addMonoBranch(speedBranchName, speed, "In", speed, "Out")
    end

    local blend = self:addObject("blend", app.GainBias())
    local blendRange = self:addObject("blendRange", app.MinMax())
    connect(blend, "Out", blendRange, "In")
    connect(blend, "Out", shifter, "Blend")
    if shifterR then
      connect(blend, "Out", shifterR, "Blend")
    end
    self:addMonoBranch("blend", blend, "In", blend, "Out")

    local level = self:addObject("level", app.GainBias())
    local levelRange = self:addObject("levelRange", app.MinMax())
    level:hardSet("Bias", 0.5)
    connect(level, "Out", levelRange, "In")
    self:addMonoBranch("level", level, "In", level, "Out")

    if shifterR then
      local levelCompL = self:addObject("levelCompL", app.ConstantGain())
      local levelCompR = self:addObject("levelCompR", app.ConstantGain())
      local vcaL = self:addObject("vcaL", app.Multiply())
      local vcaR = self:addObject("vcaR", app.Multiply())
      levelCompL:hardSet("Gain", 2.0)
      levelCompR:hardSet("Gain", 2.0)

      connect(shifter, "Out", levelCompL, "In")
      connect(shifterR, "Out", levelCompR, "In")
      connect(level, "Out", vcaL, "Left")
      connect(level, "Out", vcaR, "Left")
      connect(levelCompL, "Out", vcaL, "Right")
      connect(levelCompR, "Out", vcaR, "Right")
      connect(vcaL, "Out", self, "Out1")
      connect(vcaR, "Out", self, "Out2")
    else
      local levelComp = self:addObject("levelComp", app.ConstantGain())
      local vca = self:addObject("vca", app.Multiply())
      levelComp:hardSet("Gain", 2.0)
      connect(shifter, "Out", levelComp, "In")
      connect(level, "Out", vca, "Left")
      connect(levelComp, "Out", vca, "Right")
      connect(vca, "Out", self, "Out1")
    end
  end

  local views = {
    expanded = {},
    collapsed = {}
  }

  if voiceCount == 1 then

    views.expanded = {"tune", "shift", "blend", "level"}
  else
    for v = 1, voiceCount do
      views.expanded[#views.expanded + 1] = "pitch" .. v
      views.expanded[#views.expanded + 1] = "speed" .. v
    end
    views.expanded[#views.expanded + 1] = "blend"
    views.expanded[#views.expanded + 1] = "level"
  end

  function PitchShifter:onLoadViews(objects, branches)
    local controls = {}

    if voiceCount == 1 then
      controls.tune = Pitch {
        button = "V/oct",
        description = "V/oct",
        branch = branches.tune,
        offset = objects.tune,
        range = objects.tuneRange
      }

      controls.shift = GainBias {
        button = "speed",
        description = "Speed",
        branch = branches.shift,
        gainbias = objects.shift,
        range = objects.shiftRange,
        biasMap = Encoder.getMap("speed"),
        biasUnits = app.unitMultiplier
      }
    else
      for v = 1, voiceCount do
        local suffix = tostring(v)

        controls["pitch" .. suffix] = Pitch {
          button = "V/oct" .. suffix,
          description = string.format("V/oct (voice %d)", v),
          branch = branches["tune" .. suffix],
          offset = objects["tune" .. suffix],
          range = objects["tuneRange" .. suffix]
        }

        controls["speed" .. suffix] = GainBias {
          button = "speed" .. suffix,
          description = string.format("Speed (voice %d)", v),
          branch = branches["speed" .. suffix],
          gainbias = objects["speed" .. suffix],
          range = objects["speedRange" .. suffix],
          biasMap = Encoder.getMap("speed"),
          biasUnits = app.unitMultiplier
        }
      end
    end

    controls.blend = GainBias {
      button = "blend",
      description = "Dry/Wet Blend",
      branch = branches.blend,
      gainbias = objects.blend,
      range = objects.blendRange,
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

  return PitchShifter
end

return build
