cpmtools - Tools to access CP/M file systems
============================================

The cpmtools 1.6 package allows to create, check, read and write CP/M file
systems similar to the mtools package, which accesses DOS file systems.
Beside the regular user commands, there is a file system editor (read-only
for now).

Currently it contains:

- `cpmls` - list sorted directory with output similar to ls, DIR, P2DOS DIR
- `cpmcp` - copy files from and to CP/M file systems
- `cpmrm` - erase files from CP/M file systems
- `mkfs.cpm` - make a CP/M file system
- `fsck.cpm` - check and repair a CP/M file system
- `fsed.cpm` - view CP/M file system
- manual pages for everything including the CP/M file system format

All features of CP/M file systems are supported.

## Compilation

### Requirements

You will need an ANSI standard C compiler, prefered is the GNU CC.
You should by able to compile and work out of the box on each POSIX
compliant system (Linux, BSD and MacOS).

```bash
sudo apt-get install libncurses-dev
```

### Get the Code

```bash
git clone https://github.com/lipro-cpm4l/cpmtools.git
cd cpmtools
```

### Build and install the binary

```bash
make BINDIR=/usr/local/bin \
     MANDIR=/usr/local/share/man \
     DISKDEFS=/usr/local/etc/diskdefs \
     all
sudo mkdir -p /usr/local/bin \
              /usr/local/share/man/man1 \
              /usr/local/etc
sudo make BINDIR=/usr/local/bin \
          MANDIR=$/usr/local/share/man \
          DISKDEFS=/usr/local/etc/diskdefs \
          install
```

## Documentation

- Abstract of the [floppy user guide](http://www.moria.de/~michael/floppy/).
  The user guide is available as:
  [PostScript](http://www.moria.de/~michael/floppy/floppy.ps) and
  [PostScript](http://www.moria.de/~michael/floppy/floppy.pdf) file.

---

This is an unofficial fork!
===========================

Original written by Michael Haardt <michael@moria.de> and distributed
at first (this version) under an unkonw license and later under the
GNU General Public License version 2.

*Primary-site*: http://www.moria.de/~michael/cpmtools/

## License terms and liability

The author provides the software as is! **No warranty or liability!**

**Any guarantee and liability is excluded!**

## Authorship

*Primary-site*: http://www.moria.de/~michael/cpmtools/

### Source code

**Michael Haardt is the originator of the C source code and**
as well as the associated scripts, descriptions and help files.

**A few parts of the source code are based on the work of other
authors:**

> - **Raoul Golan**:
>   Contributed code to read apple II disk images.

*see*: [ANNOUNCE](ANNOUNCE)
