[![Build status](https://ci.appveyor.com/api/projects/status/2ogybcn80lyv9rj2/branch/cpm4l/cpmtools-2.21?svg=true)](https://ci.appveyor.com/project/rexut/cpmtools/branch/cpm4l/cpmtools-2.21)
[![Build Status](https://travis-ci.org/lipro-cpm4l/cpmtools.svg?branch=cpm4l%2Fcpmtools-2.21)](https://travis-ci.org/lipro-cpm4l/cpmtools)

cpmtools - Tools to access CP/M file systems
============================================

This package allows to access CP/M file systems similar to the well-known
mtools package, which accesses MSDOS file systems. It can be used for file
exchange with a Z80-PC simulator, but it works on floppy devices as well.

Currently it contains:

- `cpmls` - list sorted directory with output similar to ls, DIR, P2DOS DIR
   and CP/M3 DIR
- `cpmcp` - copy files from and to CP/M file systems
- `cpmrm` - erase files from CP/M file systems
- `cpmchmod` - change file permissions
- `cpmchattr` - change file attributes
- `mkfs.cpm` - make a CP/M file system
- `fsck.cpm` - check and repair a CP/M file system
- `fsed.cpm` - view CP/M file system
- manual pages for everything including the CP/M file system format

All features of CP/M file systems are supported.

## Compilation

### Requirements

You will need an ANSI standard C compiler, prefered is the GNU CC.
Optional you can build against the disk image library `libdsk`.
You should by able to compile and work out of the box on each POSIX
compliant system (Linux, BSD and MacOS).  It can be additionally
compiled for Win32 or Cygwin systems.

```bash
sudo apt-get install libncurses-dev
```

**Only when you want to compile against the disk image library libdsk**:
```bash
sudo apt-get install libdsk4-dev ### OR: libdsk5-dev
```

### Get the Code

```bash
git clone https://github.com/lipro-cpm4l/cpmtools.git
cd cpmtools
```

### Build and install the binary

```bash
./configure
make all
sudo make install
```

## Documentation

- Manpage [CP/M disk and file system format](cpm.ps) (cpm.5)
- Abstract of the [floppy user guide](http://www.moria.de/~michael/floppy/).
  The user guide is available as:
  [PostScript](http://www.moria.de/~michael/floppy/floppy.ps) and
  [PostScript](http://www.moria.de/~michael/floppy/floppy.pdf) file.
- Debian [[HOWTO]](http://forums.debian.net/viewtopic.php?f=16&t=112244)
  "Access CP/M Floppy's via libdsk & cpmtools"
- [[RC2014-Z80]](https://groups.google.com/d/msg/rc2014-z80/VD22SEht0PY/V3MYDmK4AgAJ)
  "Using cpmtools to copy CP/M software to the compact flash card"
- [[RC2014-Z80]](https://groups.google.com/d/msg/rc2014-z80/CExA08NHClg/erg4P84XCAAJ)
  "82c55 IDE adapter - FATFS for CP/M"
- [CP/M 2.2 for Exidy FDS](www.vcfed.org/forum/showthread.php?73156)
  (Floppy Disk Subsystem for the Sorcerer)
- [[CP/M-68K]](http://home.earthlink.net/~schultdw/cpm68/)
  "In order to use cpmtools you must either use a ..."
- Aminet [Retro-Computing](http://aminet.net/docs/hard/Retro-Computing.pdf)
  © 2008 – 2015 by Peter Sieg, PDF in German, 2015/08,
  [Aminet Package Info](http://aminet.net/package/docs/hard/Retro-Computing),
  [Peter Sieg at GitHub](https://github.com/petersieg/Retro-Computing)

---

This is an unofficial fork!
===========================

Original written by Michael Haardt <michael@moria.de> and John Elliott
<seasip.webmaster@gmail.com> and distributed at first under the GNU General
Public License version 2 (original) and now version 3.

*Primary-site*: http://www.moria.de/~michael/cpmtools/

## License terms and liability

The author provides the software in accordance with the terms of
the GNU-GPL. The use of the software is free of charge and is
therefore only at your own risk! **No warranty or liability!**

**Any guarantee and liability is excluded!**

## Authorship

*Primary-site*: http://www.moria.de/~michael/cpmtools/

### Source code

**Michael Haardt is the originator of the C source code and**
as well as the associated scripts, descriptions and help files.
This part is released and distributed under the GNU General
Public License (GNU-GPL) Version 3.

**A few parts of the source code are based on the work of other
authors:**

> - **John Elliott**:
>   LibDsk integration and bringing cpmtools to Windows.
> - **Bill Buckels**:
>   Building cpmtools-2.9 in Windows XP, the Cygwin port.
> - **David Schmidt**, **Udo Munk**, **Peter Dassow**:
>   For Cygwin feedback.
> - **Stevo Tarkin**, **Volker Pohlers**:
>   For Msys feedback.
> - **Rolf Harmann**:
>   For Linux feedback.
> - **Richard Brady**:
>   who may or may not know watfor:)
> - **Raoul Golan**:
>   Contributed code to read apple II disk images.

*see*: [COPYING](COPYING), [README](README),
[README.win32.cygwin.txt](README.win32.cygwin.txt),
[README.win32-libdsk](README.win32-libdsk)
