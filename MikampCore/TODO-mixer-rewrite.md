# Rebuild a better Virtch Mixer

Motivated in part by quality issues of the misxer. The mixer sounds muddy - possibly the
interpolation coefficients are inverted or something more sinister. Use of wavfile tracing
and inspection within Audacity will be used to determine the cause and confirm fixes.

The architecture of the mixer make any changes difficult, with a lot of the mixer design
philosophies focused on supporting 25MHz CPUs of the 1990s. Rewriting this code is thus
prescribed as it till facilitate more rapid implementation of some additional improvements
in fidelity.


## Notes in no specific order

 - Support only float/double as the target mixer buffer (`TICKBUF`)
 - Remove the downsample to 8/16 bit, favor separare functions optionally called by drivers
 - Use C++ for the core mixer implementations, for specialization purposes.
 - Improve fixed function volume control with per-sample slide capability (mostly ignore
   performance considerations)
 - Add bi-cubic interpolation
 - Add adhoc wavfile writing support that can write per-channel wavfiles for analysis of
   specific behaviors without having to use solo channelling features

#### MIXER struct refactor
 - Remove "Surround" mode, replace it with inverted lvol/rvol
 - Remove "CalculateVolumes" in favor of a fixed volume calculation pipeline (the original
   reason for CalculateVolumes was mainly for the self-modifying code (SMC) mixer)
 - Remove "NoClick" in favor of a built in micro-volume ramping logic applied to all mixers.
   (original reason this wasn't done was again because of the SMC optimization of the original
   assembly mixers)

## Rationales for Bigger Changes

#### removing support for int16/int32 target mixer buffer

This will not be supported since integer based internal mixing support tends to need to be
carefully optimized to whatever limitations of the target platform. In many cases, add-with-
saturation logic is required and efficient implementation of this logic varies across
platforms. In other special use cases add-with-saturation can be ignored (by way of controlling
other aspects of the mixer inputs). It is determined that such customized implementations
should be created and maintained independently.
