## ChibiOS test LED blinker for Raspberry Pi Zero ##

### What it is ###

This project serves as a starting point for writing code on the Raspberry Pi Zero using
[Steve Bate's](https://www.stevebate.net/chibios-rpi/GettingStarted.html)
[ChibiOS-RPi](https://github.com/steve-bate/ChibiOS-RPi) fork of [ChibiOS](https://github.com/mabl/ChibiOS).

This particular variant experiments with using the BCM2835's hardware PWM
on GPIO pin 18, using ChibiOS-RPi's PWM driver.

### How to build ###

**Prerequisite : arm-none-eabi toolchain**

You'll need an `arm-none-eabi` toolchain built with `--with-cpu=arm1176jzf-s --with-fpu=vfp --with-float=hard`.
([This page](https://forums.raspberrypi.com/viewtopic.php?t=225731) was helpful
in figuring out why typical ARM toolchains were no good for the RPi Zero, and to know what challenges needed solving.)

At the start of this project, it was difficult to find a modern, ready-made toolchain like that, especially
if path-indepedence is one of your pet-peeves.
So I made [this one](https://drive.google.com/file/d/1Cn6uXWjJw1NNnBi1Az99kEKoWq8GaSdi/view?usp=sharing)
using [crosstool-ng](https://crosstool-ng.github.io/).
Due to health issues and related time constraints, I didn't get to feature-creep
it as much as I'd like. It's also only built for Linux (x86_64) right now. But
it does have a working GCC cross-compiler (version 11.2.0) and relevant
binutils+runtimes+etc. And it should work from any directory you choose to place
it in (in other words, it is path-independent); all you need to do is make sure
that the `<toolchain-path>/bin` path is in your *PATH* environment variable.

**Prerequisite : FAT32 formatted microSD card**

The executable produced by this project will need to be placed onto a microSD
card that is inserted into the Raspberry Pi Zero. Regarding the partitioning
and formatting of this card, here's what worked for me:

* The Raspberry Pi Zero's microSD card should contain a DOS partition table
with a single partition having type "W95 FAT32 (LBA)" (hex code `c` in `fdisk`).

* The single partition should be formatted with a vfat (FAT32) filesystem.
Note that `mkfs.vfat` has a `-n <NAME>` option that can be used to label (name)
the filesystem, which can be helpful for identifying the thing.

**Prerequisite : LEDs and/or Oscilloscope**

Like the project this was forked from, this code sets and clears GPIO pin number
16 on the RPi Zero. Unlike that project, this also uses the PWM driver to
set and clear GPIO pin number 18. By setting the PWM period to something fairly
long, it can actually make this visible to the naked eye.

So hooking up LEDs to GPIO18 is important here. Of less importance, it might
still be helpful to put a LED on GPIO16 just to see if the BCM2835 is alive,
should anything go wrong.

Ideally you'll have an oscilloscope or some other means of verifying higher
frequency PWM signals, and you'd hook a probe up to GPIO18, then play with
the code to get faster (and thus more useful) PWM signals.

**Build and install**

```
git clone https://github.com/chadjoan/cdj-testing-rpzero-blinker.git
cd cdj-testing-rpzero-blinker
make
```

It should mention the creation of the `build/sdcard-final-contents` directory.
This directory has all of the files that need to end up on the microSD card.
So the next step simply involves plugging in the microSD card and copying
the contents of `build/sdcard-final-contents` onto the microSD card:
```
cp -r build/sdcard-final-contents/* /path/to/sd-card/
```

After that, just unmount (eject) and remove the microSD card, put it into the
Raspberry Pi Zero, plug in (turn on) the RPi Zero, then (hopefully) enjoy some
successful and happy blinkenlight.
