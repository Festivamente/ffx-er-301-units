local category = "Festivamente"

local units = {
  {
    title = "BitCrush",
    moduleName = "BitCrush",
    category = category
  },
  {
    title = "SoftClip",
    moduleName = "SoftClip",
    category = category
  },
  {
    title = "Band Spreader",
    moduleName = "BandSpreader",
    category = category,
    channelCount = 2
  },
  {
    title = "Compressor",
    moduleName = "CompressorUnit",
    category = category
  },
  {
    title = "Exciter",
    moduleName = "Exciter",
    category = category
  },
  {
    title = "Shaper",
    moduleName = "Shaper",
    category = category
  },
  {
    title = "Fuzz Box",
    moduleName = "FuzzBoxUnit",
    category = category
  },
  {
    title = "Lo-Fi Filter",
    moduleName = "Lofi",
    category = category
  },
  {
    title = "Grotesque Filter",
    moduleName = "Grotesque",
    category = category
  },
  {
    title = "Wet/Dry",
    moduleName = "WDWrap",
    category = category
  },
  {
    title = "Pluck",
    moduleName = "Pluck",
    category = category
  },
  {
    title = "Comb",
    moduleName = "Comb",
    category = category
  },
  {
    title = "Flanger",
    moduleName = "Flanger",
    category = category
  },
  {
    title = "Freeze",
    moduleName = "Freeze",
    category = category
  },
  {
    title = "Leslie Cabinet",
    moduleName = "Leslie",
    category = category
  },
  {
    title = "Reverse Tape Delay",
    moduleName = "ReverseDelayUnit",
    category = category
  },
  {
    title = "Morpho",
    moduleName = "Morpho",
    category = category,
    keywords = "granular, tape, sample, effect"
  },
  {
    title = "Pitch Shifter (1 voice)",
    moduleName = "PitchShifter",
    category = category,
    keywords = "pitch, effect"
  },
  {
    title = "Pitch Shifter (2 voices)",
    moduleName = "PitchShifter2V",
    category = category,
    keywords = "pitch, effect"
  },
  {
    title = "Pitch Shifter (3 voices)",
    moduleName = "PitchShifter3V",
    category = category,
    keywords = "pitch, effect"
  },
  {
    title = "Unison",
    moduleName = "Unison",
    category = category
  },
  {
    title = "Tap Delay",
    moduleName = "TapDelay",
    category = category
  },
  {
    title = "Kick",
    moduleName = "Kick",
    category = category,
    keywords = "percussion, drum, synthesis"
  },
  {
    title = "Snare",
    moduleName = "Snare",
    category = category,
    keywords = "percussion, drum, synthesis"
  },
  {
    title = "HiHat",
    moduleName = "HiHat",
    category = category,
    keywords = "percussion, drum, synthesis"
  },
  {
    title = "Tom",
    moduleName = "Tom",
    category = category,
    keywords = "percussion, drum, synthesis"
  },
  {
    title = "Cowbell",
    moduleName = "Cowbell",
    category = category,
    keywords = "percussion, drum, synthesis"
  },
  {
    title = "Cymbal",
    moduleName = "Cymbal",
    category = category,
    keywords = "percussion, drum, synthesis"
  },
  {
    title = "Cat",
    moduleName = "Cat",
    category = category,
    keywords = "vca, utility, cat"
  }
}

return {
  title = "ffx by Festivamente",
  name = "ffx",
  aliases = {"InsertEffects", "TimeEffects", "IFX", "TFX", "perc", "Cat"},
  keyword = "festivamente, effects, percussion, synthesis, utility",
  author = "Festivamente",
  units = units
}
