CC=		gcc
CFLAGS=		-g -pipe -O2 -Wall -Wmissing-prototypes -Wstrict-prototypes -Wcast-qual -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-declarations -Wnested-externs -pedantic -fno-common
DISKDEFS=	/usr/cpm/etc/diskdefs
CPPFLAGS=	-DDISKDEFS=\"$(DISKDEFS)\" -DFORMAT=\"ibm-3740\"
#CPPFLAGS=	-DDISKDEFS=\"$(DISKDEFS)\" -DFORMAT=\"ibm-3740\" -I/usr/dmalloc/include -DDMALLOC -DDMALLOC_FUNC_CHECK
LDFLAGS=	-g
#LDFLAGS=	-g -L/usr/dmalloc/lib
LIBCURSES=	-lcurses
LIBS=
#LIBS=		-ldmalloc
MAKEDEPEND=	mkdep -d
#MAKEDEPEND=	gcc -MM
#MAKEDEPEND=	makedepend -f-
BINDIR=		/usr/cpm/bin
MANDIR=		/usr/cpm/man/en

ALL=		cpmls cpmrm cpmcp mkfs.cpm fsck.cpm fsed.cpm

all:		$(ALL)

cpmls:		cpmls.o cpmfs.o
		$(CC) $(LDFLAGS) -o $@ cpmls.o cpmfs.o $(LIBS)

cpmrm:		cpmrm.o cpmfs.o
		$(CC) $(LDFLAGS) -o $@ cpmrm.o cpmfs.o $(LIBS)

cpmcp:		cpmcp.o cpmfs.o
		$(CC) $(LDFLAGS) -o $@ cpmcp.o cpmfs.o $(LIBS)

mkfs.cpm:	mkfs.cpm.o cpmfs.o
		$(CC) $(LDFLAGS) -o $@ mkfs.cpm.o cpmfs.o $(LIBS)

fsck.cpm:	fsck.cpm.o cpmfs.o
		$(CC) $(LDFLAGS) -o $@ fsck.cpm.o cpmfs.o $(LIBS)

fsed.cpm:	fsed.cpm.o
		$(CC) $(LDFLAGS) -o $@ fsed.cpm.o $(LIBCURSES) $(LIBS)

fsck.test:	fsck.cpm
		-./fsck.cpm -n badfs/status
		-./fsck.cpm -n badfs/extno
		-./fsck.cpm -n badfs/lcr
		-./fsck.cpm -n badfs/name
		-./fsck.cpm -n badfs/extension
		-./fsck.cpm -n badfs/blocknumber
		-./fsck.cpm -n badfs/recordcount
		-./fsck.cpm -n badfs/hugecom
		-./fsck.cpm -n badfs/timestamps
		-./fsck.cpm -n badfs/multipleblocks
		-./fsck.cpm -n badfs/doubleext
		-./fsck.cpm -f pcw -n badfs/label

install:	all
		install -c -s -m 755 cpmls $(BINDIR)/cpmls
		install -c -s -m 755 cpmcp $(BINDIR)/cpmcp
		install -c -s -m 755 cpmrm $(BINDIR)/cpmrm
		install -c -s -m 755 mkfs.cpm $(BINDIR)/mkfs.cpm
		install -c -s -m 755 fsck.cpm $(BINDIR)/fsck.cpm
		install -c -s -m 755 fsed.cpm $(BINDIR)/fsed.cpm
		install -c -m 644 diskdefs $(DISKDEFS)
		install -c -m 644 cpmls.1 $(MANDIR)/man1/cpmls.1
		install -c -m 644 cpmcp.1 $(MANDIR)/man1/cpmcp.1
		install -c -m 644 cpmrm.1 $(MANDIR)/man1/cpmrm.1
		install -c -m 644 mkfs.cpm.1 $(MANDIR)/man1/mkfs.cpm.1
		install -c -m 644 fsck.cpm.1 $(MANDIR)/man1/fsck.cpm.1
		install -c -m 644 fsed.cpm.1 $(MANDIR)/man1/fsed.cpm.1

clean:
		rm -f *.o

clobber:	clean
		rm -f $(ALL) *.out

tar:		clobber
		(b=`pwd`; b=`basename $$b`; cd ..; tar zcvf $$b.tar.gz $$b)

depend:
		$(MAKEDEPEND) $(CPPFLAGS) *.c >.depend

include .depend
