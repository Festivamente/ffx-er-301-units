# ffx by Festivamente

Version **1.1.0**

Effects, percussion, and utility units for the ER-301 / ER-301 for VCV Rack.

More information: https://festivamente.com/ffx/

## Units

### Effects

- BitCrush
- SoftClip
- Band Spreader
- Compressor
- Exciter
- Shaper
- Fuzz Box
- Lo-Fi Filter
- Grotesque Filter
- Comb
- Flanger
- Freeze
- Leslie Cabinet
- Reverse Tape Delay
- Pitch Shifter (1/2/3 voices)
- Unison
- Tap Delay

### Instruments

- Pluck
- Morpho
- Kick
- Snare
- HiHat
- Tom
- Cowbell
- Cymbal

### Utility

- Wet/Dry
- Cat

## Build

ER-301 for VCV Rack:

```sh
make
```

ER-301 hardware:

```sh
make PROFILE=release ARCH=am335x
```

## Other make commands

Clean build output:

```sh
make dist-clean
```

