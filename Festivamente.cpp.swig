%module ffx_libFestivamente

%include <od/glue/mod.cpp.swig>

%{
#undef SWIGLUA

#include <od/units/CustomUnit.h>

#include <BitCrush.h>
#include <SoftClip.h>
#include <BandSpreader.h>
#include <CompressorObject.h>
#include <Exciter.h>
#include <FuzzBox.h>
#include <Lofi.h>
#include <Grotesque.h>
#include <Shaper.h>
#include <WetDryFade.h>
#include <WetDryUnit.h>

#include <Comb.h>
#include <Flanger.h>
#include <Freeze.h>
#include <Leslie.h>
#include <MorphoHead.h>
#include <PitchShifter.h>
#include <Pluck.h>
#include <ReverseDelay.h>
#include <TapDelay.h>
#include <Unison.h>

#include <Kick.h>
#include <Snare.h>
#include <HiHat.h>
#include <Tom.h>
#include <Cowbell.h>
#include <Cymbal.h>

#include <CatPhoto.h>

#define SWIGLUA
%}

%import(module="app") <od/units/CustomUnit.h>

%include <BitCrush.h>
%include <SoftClip.h>
%include <BandSpreader.h>
%include <CompressorObject.h>
%include <Exciter.h>
%include <FuzzBox.h>
%include <Lofi.h>
%include <Grotesque.h>
%include <Shaper.h>
%include <WetDryFade.h>
%include <WetDryUnit.h>

%include <Comb.h>
%include <Flanger.h>
%include <Freeze.h>
%include <Leslie.h>
%include <MorphoHead.h>
%include <PitchShifter.h>
%include <Pluck.h>
%include <ReverseDelay.h>
%include <TapDelay.h>
%include <Unison.h>

%include <Kick.h>
%include <Snare.h>
%include <HiHat.h>
%include <Tom.h>
%include <Cowbell.h>
%include <Cymbal.h>

%include <CatPhoto.h>
