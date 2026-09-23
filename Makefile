PKGNAME ?= ffx
PKGVERSION ?= 1.1.0
LIBNAME ?= libFestivamente
PROFILE ?= release
SDKPATH ?= ../..

headers = \
  objects/BitCrush.h \
  objects/SoftClip.h \
  objects/BandSpreader.h \
  objects/CompressorObject.h \
  objects/Exciter.h \
  objects/FuzzBox.h \
  objects/Lofi.h \
  objects/Grotesque.h \
  objects/Shaper.h \
  objects/WetDryFade.h \
  objects/WetDryUnit.h \
  objects/Comb.h \
  objects/Flanger.h \
  objects/Freeze.h \
  objects/Leslie.h \
  objects/MorphoHead.h \
  objects/PitchShifter.h \
  objects/Pluck.h \
  objects/ReverseDelay.h \
  objects/TapDelay.h \
  objects/Unison.h \
  objects/Kick.h \
  objects/Snare.h \
  objects/HiHat.h \
  objects/Tom.h \
  objects/Cowbell.h \
  objects/Cymbal.h \
  objects/drum_common.h \
  graphics/CatPhoto.h \
  graphics/PhotoData.h \
  dsp/CircularBuffer.h \
  dsp/DspMath.h \
  dsp/DualGrainPitch.h \
  dsp/morpho/Grain.h \
  dsp/morpho/MonoGrain.h \
  dsp/morpho/StereoGrain.h

sources = \
  Festivamente.cpp.swig \
  objects/BitCrush.cpp \
  objects/SoftClip.cpp \
  objects/BandSpreader.cpp \
  objects/CompressorObject.cpp \
  objects/Exciter.cpp \
  objects/FuzzBox.cpp \
  objects/Lofi.cpp \
  objects/Grotesque.cpp \
  objects/Shaper.cpp \
  objects/WetDryFade.cpp \
  objects/WetDryUnit.cpp \
  objects/Comb.cpp \
  objects/Flanger.cpp \
  objects/Freeze.cpp \
  objects/Leslie.cpp \
  objects/MorphoHead.cpp \
  objects/PitchShifter.cpp \
  objects/Pluck.cpp \
  objects/ReverseDelay.cpp \
  objects/TapDelay.cpp \
  objects/Unison.cpp \
  objects/Kick.cpp \
  objects/Snare.cpp \
  objects/HiHat.cpp \
  objects/Tom.cpp \
  objects/Cowbell.cpp \
  objects/Cymbal.cpp \
  graphics/CatPhoto.cpp \
  dsp/morpho/Grain.cpp \
  dsp/morpho/MonoGrain.cpp \
  dsp/morpho/StereoGrain.cpp

assets = \
  assets/toc.lua \
  assets/BitCrush.lua \
  assets/SoftClip.lua \
  assets/BandSpreader.lua \
  assets/CompressorUnit.lua \
  assets/Exciter.lua \
  assets/FuzzBoxUnit.lua \
  assets/Lofi.lua \
  assets/Grotesque.lua \
  assets/Shaper.lua \
  assets/WDWrap.lua \
  assets/Comb.lua \
  assets/Flanger.lua \
  assets/Freeze.lua \
  assets/Leslie.lua \
  assets/Morpho.lua \
  assets/PitchShifterBase.lua \
  assets/PitchShifter.lua \
  assets/PitchShifter2V.lua \
  assets/PitchShifter3V.lua \
  assets/Pluck.lua \
  assets/ReverseDelayUnit.lua \
  assets/TapDelay.lua \
  assets/Unison.lua \
  assets/Kick.lua \
  assets/Snare.lua \
  assets/HiHat.lua \
  assets/Tom.lua \
  assets/Cowbell.lua \
  assets/Cymbal.lua \
  assets/Cat.lua \
  assets/CatControl.lua

includes = . objects graphics dsp dsp/morpho
symbols =

include $(SDKPATH)/scripts/tutorial.mk

ifeq ($(ARCH),darwin)
CFLAGS.swig += -Wno-unknown-warning-option -Wno-overloaded-virtual
endif
