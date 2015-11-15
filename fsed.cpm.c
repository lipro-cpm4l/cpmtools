/* #includes */ /*{{{C}}}*//*{{{*/
#include <assert.h>
#include <ctype.h>
#include <curses.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
/*}}}*/

extern char **environ;

struct diskDefs /*{{{*/
{
  int secLength;
  int tracks;
  int sectrk;
  int blksiz;
  int maxdir;
  int skew;
  int boottrk;
  int type;
  int fd;

  int *skewtab;
  int size;
  int extents;
  unsigned long bytes;
};
/*}}}*/
static char *mapbuf;

#define DR22  0
#define P2DOS 1
#define DR3   2

/* physical sector I/O */
/* readSector         -- read a physical sector                  */ /*{{{*/
static int readSector(const struct diskDefs *drive, int track, int sector, char *buf)
{
  int res;

  assert(sector>=0);
  assert(sector<drive->sectrk);
  assert(track>=0);
  assert(track<drive->tracks);
  if (lseek(drive->fd,(off_t)(sector+track*drive->sectrk)*drive->secLength,SEEK_SET)==-1) 
  {
    return -1;
  }
  if ((res=read(drive->fd, buf, drive->secLength)) != drive->secLength) 
  {
    if (res==-1)
    {
      return -1;
    }
    else memset(buf+res,0,drive->secLength-res); /* hit end of disk image */
  }
  return 0;
}
/*}}}*/
/* writeSector        -- write physical sector                   */ /*{{{*/
static int writeSector(const struct diskDefs *drive, int track, int sector, const char *buf)
{
  assert(sector>=0);
  assert(sector<drive->sectrk);
  assert(track>=0);
  assert(track<drive->tracks);
  if (lseek(drive->fd,(off_t)(sector+track*drive->sectrk)*drive->secLength, SEEK_SET)==-1)
  {
    return -1;
  }
  if (write(drive->fd, buf, drive->secLength) == drive->secLength) return 0;
  return -1;
}
/*}}}*/

/* getformat          -- get DPB and init in-core data for drive */ /*{{{*/
static int getformat(const char *format, struct diskDefs *d)
{
  /* read format specific values from diskdefs file */ /*{{{*/
  {
    char buf[128],str[32],typestr[8];
    FILE *fp;
		  
    if ((fp=fopen(DISKDEFS,"r"))==(FILE*)0 && (fp=fopen("diskdefs","r"))==(FILE*)0)
    {
      fprintf(stderr,"fsed.cm: Neither " DISKDEFS " nor diskdefs could be opened.\n");
      exit(1);
    }
    while (fgets(buf,sizeof(buf),fp)!=(char*)0)
    {
      if (buf[0]=='#' || buf[0]=='\n') continue;
      if (sscanf(buf,"%s %d %d %d %d %d %d %d %s\n",str,&(d->secLength),&(d->tracks),&(d->sectrk),&(d->blksiz),&(d->maxdir),&(d->skew),&(d->boottrk),typestr)!=9)
      {
        fprintf(stderr,"fsed.cpm: invalid line for format %s\n",format);
        continue;
      }
      if (strcmp(str,format)==0) break;
    }
    fclose(fp);
    if (strcmp(str,format))
    {
      fprintf(stderr,"fsed.cpm: unknown format %s\n",format);
      exit(1);
    }
    if (strcmp(typestr,"p2dos")==0) d->type=P2DOS;
    else if (strcmp(typestr,"2.2")==0) d->type=DR22;
    else if (strcmp(typestr,"3")==0) d->type=DR3;
    else
    {
      fprintf(stderr,"fsed.cpm: invalid operating system %s\n",typestr);
      exit(1);
    }
    d->size=(d->secLength*d->sectrk*(unsigned long)(d->tracks-d->boottrk))/d->blksiz;
    d->bytes=d->sectrk*d->tracks*(unsigned long)d->secLength;
    d->extents=((d->size>=256 ? 8 : 16)*d->blksiz)/16384;
  }
  /*}}}*/
  /* generate skew table */ /*{{{*/
  {
    int	i,j,k;

    if (( d->skewtab = malloc(d->sectrk*sizeof(int))) == (int*)0) 
    {
      fprintf(stderr,"fsed.cpm: can not allocate memory for skew sector table\n");
      exit(1);
    }
    for (i=j=0; i<d->sectrk; ++i,j=(j+d->skew)%d->sectrk)
    {
      while (1)
      {
        for (k=0; k<i && d->skewtab[k]!=j; ++k);
        if (k<i) j=(j+1)%d->sectrk;
        else break;
      }
      d->skewtab[i]=j;
    }
  }
  /*}}}*/
  return 0;
}
/*}}}*/

static struct tm *cpmtime(char lday, char hday, char hour, char min) /*{{{*/
{
  static struct tm tm;
  unsigned long days=(lday&0xff)|((hday&0xff)<<8);
  int d;
  int md[12]={31,0,31,30,31,30,31,31,30,31,30,31};

  tm.tm_sec=0;
  tm.tm_min=((min>>4)&0xf)*10+(min&0xf);
  tm.tm_hour=((hour>>4)&0xf)*10+(hour&0xf);
  tm.tm_mon=0;
  tm.tm_year=1978;
  tm.tm_isdst=-1;
  if (days) --days;
  while (days>=(d=(((tm.tm_year%400)==0 || ((tm.tm_year%4)==0 && (tm.tm_year%100))) ? 366 : 365)))
  {
    days-=d;
    ++tm.tm_year;
  }
  md[1]=((tm.tm_year%400)==0 || ((tm.tm_year%4)==0 && (tm.tm_year%100))) ? 29 : 28;
  while (days>=md[tm.tm_mon])
  {
    days-=md[tm.tm_mon];
    ++tm.tm_mon;
  }
  tm.tm_mday=days+1;
  tm.tm_year-=1900;
  return &tm;
}
/*}}}*/
static void info(struct diskDefs *sb, const char *format, const char *image) /*{{{*/
{
  const char *msg;

  clear();
  msg="File system characteristics";
  move(0,(COLS-strlen(msg))/2); printw(msg);
  move(2,0); printw("                      Image: %s",image);
  move(3,0); printw("                     Format: %s",format);
  move(4,0); printw("                File system: ");
  switch (sb->type)
  {
    case DR22: printw("CP/M 2.2"); break;
    case P2DOS: printw("P2DOS 2.3"); break;
    case DR3: printw("CP/M Plus"); break;
  }

  move(6,0); printw("              Sector length: %d",sb->secLength);
  move(7,0); printw("           Number of tracks: %d",sb->tracks);
  move(8,0); printw("          Sectors per track: %d",sb->sectrk);

  move(10,0);printw("                 Block size: %d",sb->blksiz);
  move(11,0);printw("Number of directory entries: %d",sb->maxdir);
  move(12,0);printw("        Logical sector skew: %d",sb->skew);
  move(13,0);printw("    Number of system tracks: %d",sb->boottrk);
  move(14,0);printw(" Logical extents per extent: %d",sb->extents);
  move(15,0);printw("    Allocatable data blocks: %d",sb->size-(sb->maxdir*32+sb->blksiz-1)/sb->blksiz);

  msg="Any key to continue";
  move(23,(COLS-strlen(msg))/2); printw(msg);
  getch();
}
/*}}}*/
static void map(struct diskDefs *sb) /*{{{*/
{
  const char *msg;
  char bmap[18*80];
  int secmap,pos,system,directory;

  clear();
  msg="Data map";
  move(0,(COLS-strlen(msg))/2); printw(msg);

  secmap=(sb->tracks*sb->sectrk+80*18-1)/(80*18);
  memset(bmap,' ',sizeof(bmap));
  system=sb->boottrk*sb->sectrk;
  memset(bmap,'S',system/secmap);
  directory=(sb->maxdir*32+sb->secLength-1)/sb->secLength;
  memset(bmap+system/secmap,'D',directory/secmap);
  memset(bmap+(system+directory)/secmap,'.',sb->bytes/(sb->secLength*secmap));

  for (pos=0; pos<(sb->maxdir*32+sb->secLength-1)/sb->secLength; ++pos)
  {
    int entry;

    readSector(sb,sb->boottrk+pos/(sb->sectrk*sb->secLength),pos/sb->secLength,mapbuf);
    for (entry=0; entry<sb->secLength/32 && (pos*sb->secLength/32)+entry<sb->maxdir; ++entry)
    {
      int i;

      if (mapbuf[entry*32]>=0 && mapbuf[entry*32]<=(sb->type==P2DOS ? 31 : 15))
      {
        for (i=0; i<16; ++i)
        {
          unsigned int sector;

          sector=mapbuf[entry*32+16+i]&0xff;
          if (sb->size>=256) sector|=(((mapbuf[entry*32+16+ ++i]&0xff)<<8));
          if (sector>0 && sector<=sb->size)
          {
            /* not entirely correct without the last extent record count */
            sector=sector*(sb->blksiz/sb->secLength)+sb->sectrk*sb->boottrk;
            memset(bmap+sector/secmap,'#',sb->blksiz/(sb->secLength*secmap));
          }
        }
      }
    }
  }

  for (pos=0; pos<sizeof(bmap); ++pos)
  {
    move(2+pos%18,pos/18);
    addch(bmap[pos]);
  }
  move(21,0); printw("S=System area   D=Directory area   #=File data   .=Free");
  msg="Any key to continue";
  move(23,(COLS-strlen(msg))/2); printw(msg);
  getch();
}
/*}}}*/
static void data(struct diskDefs *sb, const char *buf, unsigned long int pos) /*{{{*/
{
  int offset=(pos%sb->secLength)&~0x7f;
  int i;

  for (i=0; i<128; ++i)
  {
    move(4+(i>>4),(i&0x0f)*3+!!(i&0x8)); printw("%02x",buf[i+offset]&0xff);
    if (pos%sb->secLength==i+offset) attron(A_REVERSE);
    move(4+(i>>4),50+(i&0x0f)); printw("%c",isprint(buf[i+offset]) ? buf[i+offset] : '.');
    attroff(A_REVERSE);
  }
  move(4+((pos&0x7f)>>4),((pos&0x7f)&0x0f)*3+!!((pos&0x7f)&0x8)+1);
}
/*}}}*/

int main(int argc, char *argv[]) /*{{{*/
{
  /* variables */ /*{{{*/
  char *image;
  const char *format=FORMAT;
  int c,usage=0;
  struct diskDefs sb;
  unsigned long pos;
  chtype ch;
  int reload;
  char *buf;
  /*}}}*/

  /* parse options */ /*{{{*/
  while ((c=getopt(argc,argv,"f:h?"))!=EOF) switch(c)
  {
    case 'f': format=optarg; break;
    case 'h':
    case '?': usage=1; break;
  }

  if (optind!=(argc-1)) usage=1;
  else image=argv[optind];

  if (usage)
  {
    fprintf(stderr,"Usage: fsed.cpm [-f format] image\n");
    exit(1);
  }
  /*}}}*/
  /* open image */ /*{{{*/
  if ((sb.fd=open(image,O_RDONLY))==-1) 
  {
    fprintf(stderr,"fsed.cpm: can not open %s: %s\n",image,strerror(errno));
    exit(1);
  }
  /*}}}*/
  getformat(format,&sb);
  /* alloc sector buffers */ /*{{{*/
  if ((buf=malloc(sb.secLength))==(char*)0 || (mapbuf=malloc(sb.secLength))==(char*)0)
  {
    fprintf(stderr,"fsed.cpm: can not allocate sector buffer (%s).\n",strerror(errno));
    exit(1);
  }
  /*}}}*/
  /* init curses */ /*{{{*/
  initscr();
  noecho();
  raw();
  nonl();
  idlok(stdscr,TRUE);
  idcok(stdscr,TRUE);
  keypad(stdscr,TRUE);
  clear();
  /*}}}*/

  pos=0;
  reload=1;
  do
  {
    /* display position and load data */ /*{{{*/
    clear();
    move(2,0); printw("Byte %8d (0x%08x)  ",pos,pos);
    if (pos<(sb.boottrk*sb.sectrk*sb.secLength))
    {
      printw("Physical sector %3d  ",((pos/sb.secLength)%sb.sectrk)+1);
    }
    else
    {
      printw("Sector %3d ",((pos/sb.secLength)%sb.sectrk)+1);
      printw("(physical %3d)  ",sb.skewtab[(pos/sb.secLength)%sb.sectrk]+1);
    }
    printw("Offset %5d  ",pos%sb.secLength);
    printw("Track %5d",pos/(sb.secLength*sb.sectrk));
    move(LINES-3,0); printw("N)ext track    P)revious track");
    move(LINES-2,0); printw("n)ext record   n)revious record     f)orward byte      b)ackward byte");
    move(LINES-1,0); printw("i)nfo          q)uit");
    if (reload)
    {
      int res;

      if (pos<(sb.boottrk*sb.sectrk*sb.secLength)) res=readSector(&sb,pos/(sb.secLength*sb.sectrk),(pos/sb.secLength)%sb.sectrk,buf);
      else res=readSector(&sb,pos/(sb.secLength*sb.sectrk),sb.skewtab[(pos/sb.secLength)%sb.sectrk],buf);
      if (res==-1)
      {
        move(4,0); printw("Data can not be read: %s",strerror(errno));
      }
      else reload=0;
    }
    /*}}}*/

    if /* position before end of system area */ /*{{{*/
    (pos<(sb.boottrk*sb.sectrk*sb.secLength))
    {
      const char *msg;

      msg="System area"; move(0,(COLS-strlen(msg))/2); printw(msg);
      move(LINES-3,36); printw("F)orward 16 byte   B)ackward 16 byte");
      if (!reload) data(&sb,buf,pos);
      switch (ch=getch())
      {
        case 'F': /* next 16 byte */ /*{{{*/
        {
          if (pos+16<sb.bytes)
          {
            if (pos/sb.secLength!=(pos+16)/sb.secLength) reload=1;
            pos+=16;
          }
          break;
        }
        /*}}}*/
        case 'B': /* previous 16 byte */ /*{{{*/
        {
          if (pos>=16)
          {
            if (pos/sb.secLength!=(pos-16)/sb.secLength) reload=1;
            pos-=16;
          }
          break;
        }
        /*}}}*/
      }
    }
    /*}}}*/
    else if /* position before end of directory area */ /*{{{*/
    (pos<(sb.boottrk*sb.sectrk*sb.secLength+sb.maxdir*32))
    {
      const char *msg;
      unsigned long entrystart=(pos&~0x1f)%sb.secLength;
      int entry=(pos-(sb.boottrk*sb.sectrk*sb.secLength))>>5;
      int offset=pos&0x1f;

      msg="Directory area"; move(0,(COLS-strlen(msg))/2); printw(msg);
      move(LINES-3,36); printw("F)orward entry     B)ackward entry");

      move(13,0); printw("Entry %3d: ",entry);      
      if /* free or used directory entry */ /*{{{*/
      ((buf[entrystart]>=0 && buf[entrystart]<=(sb.type==P2DOS ? 31 : 15)) || buf[entrystart]==(char)0xe5)
      {
        int i;

        if (buf[entrystart]==(char)0xe5)
        {
          if (offset==0) attron(A_REVERSE);
          printw("Free");
          attroff(A_REVERSE);
        }
        else printw("Directory entry");
        move(15,0);
        if (buf[entrystart]!=(char)0xe5)
        {
          printw("User: ");
          if (offset==0) attron(A_REVERSE);
          printw("%2d",buf[entrystart]);
          attroff(A_REVERSE);
          printw(" ");
        }
        printw("Name: ");
        for (i=0; i<8; ++i)
        {
          if (offset==1+i) attron(A_REVERSE);
          printw("%c",buf[entrystart+1+i]&0x7f);
          attroff(A_REVERSE);
        }
        printw(" Extension: ");
        for (i=0; i<3; ++i)
        {
          if (offset==9+i) attron(A_REVERSE);
          printw("%c",buf[entrystart+9+i]&0x7f);
          attroff(A_REVERSE);
        }
        move(16,0); printw("Extent: %3d",((buf[entrystart+12]&0xff)+((buf[entrystart+14]&0xff)<<5))/sb.extents);
        printw(" (low: ");
        if (offset==12) attron(A_REVERSE);
        printw("%2d",buf[entrystart+12]&0xff);
        attroff(A_REVERSE);
        printw(", high: ");
        if (offset==14) attron(A_REVERSE);
        printw("%2d",buf[entrystart+14]&0xff);
        attroff(A_REVERSE);
        printw(")");
        move(17,0); printw("Last extent record count: ");
        if (offset==15) attron(A_REVERSE);
        printw("%3d",buf[entrystart+15]&0xff);
        attroff(A_REVERSE);
        move(18,0); printw("Last record byte count: ");
        if (offset==13) attron(A_REVERSE);
        printw("%3d",buf[entrystart+13]&0xff);
        attroff(A_REVERSE);
        move(19,0); printw("Data blocks:");
        for (i=0; i<16; ++i)
        {
          unsigned int block=buf[entrystart+16+i]&0xff;
          if (sb.size>=256)
          {
            printw(" ");
            if (offset==16+i || offset==16+i+1) attron(A_REVERSE);
            printw("%5d",block|(((buf[entrystart+16+ ++i]&0xff)<<8)));
            attroff(A_REVERSE);
          }
          else
          {
            printw(" ");
            if (offset==16+i) attron(A_REVERSE);
            printw("%3d",block);
            attroff(A_REVERSE);
          }
        }
      }
      /*}}}*/
      else if /* disc label */ /*{{{*/
      (buf[entrystart]==0x20 && sb.type==DR3)
      {
        int i;
        const struct tm *tm;
        char s[30];

        if (offset==0) attron(A_REVERSE);
        printw("Disc label");
        attroff(A_REVERSE);
        move(15,0);
        printw("Label: ");
        for (i=0; i<11; ++i)
        {
          if (i+1==offset) attron(A_REVERSE);
          printw("%c",buf[entrystart+1+i]&0x7f);
          attroff(A_REVERSE);
        }
        move(16,0);
        printw("Bit 0,7: ");
        if (offset==12) attron(A_REVERSE);
        printw("Label %s",buf[entrystart+12]&1 ? "set" : "not set");
        printw(", password protection %s",buf[entrystart+12]&0x80 ? "set" : "not set");
        attroff(A_REVERSE);
        move(17,0);
        printw("Bit 4,5,6: ");
        if (offset==12) attron(A_REVERSE);
        printw("Time stamp ");
        if (buf[entrystart+12]&0x10) printw("on create, ");
        else printw("not on create, ");
        if (buf[entrystart+12]&0x20) printw("on modification, ");
        else printw("not on modifiction, ");
        if (buf[entrystart+12]&0x40) printw("on access");
        else printw("not on access");
        attroff(A_REVERSE); 
        move(18,0);
        printw("Password: ");
        for (i=0; i<8; ++i)
        {
          char c;

          if (offset==16+(7-i)) attron(A_REVERSE);
          c=(buf[entrystart+16+(7-i)]^buf[entrystart+13])&0x7f;
          printw("%c",isprint(c) ? c : ' ');
          attroff(A_REVERSE);
        }
        printw(" XOR value: ");
        if (offset==13) attron(A_REVERSE);
        printw("0x%02x",buf[entrystart+13]&0xff);
        attroff(A_REVERSE);
        move(19,0);
        printw("Created: ");
        tm=cpmtime(buf[entrystart+24],buf[entrystart+25],buf[entrystart+26],buf[entrystart+27]);
        if (offset==24 || offset==25) attron(A_REVERSE);
        strftime(s,sizeof(s),"%x",tm);
        printw("%s",s);
        attroff(A_REVERSE);
        printw(" ");
        if (offset==26) attron(A_REVERSE);
        printw("%2d",tm->tm_hour);
        attroff(A_REVERSE);
        printw(":");
        if (offset==27) attron(A_REVERSE);
        printw("%02d",tm->tm_min);
        attroff(A_REVERSE);
        printw(" Updated: ");
        tm=cpmtime(buf[entrystart+28],buf[entrystart+29],buf[entrystart+30],buf[entrystart+31]);
        if (offset==28 || offset==29) attron(A_REVERSE);
        strftime(s,sizeof(s),"%x",tm);
        printw("%s",s);
        attroff(A_REVERSE);
        printw(" ");
        if (offset==30) attron(A_REVERSE);
        printw("%2d",tm->tm_hour);
        attroff(A_REVERSE);
        printw(":");
        if (offset==31) attron(A_REVERSE);
        printw("%02d",tm->tm_min);
        attroff(A_REVERSE);
      }
      /*}}}*/
      else if /* time stamp */ /*{{{*/
      (buf[entrystart]==0x21 && (sb.type==P2DOS || sb.type==DR3))
      {
        const struct tm *tm;
        char s[30];

        if (offset==0) attron(A_REVERSE);
        printw("Time stamps");
        attroff(A_REVERSE);
        move(15,0);
        printw("3rd last extent: Created/Accessed ");
        tm=cpmtime(buf[entrystart+1],buf[entrystart+2],buf[entrystart+3],buf[entrystart+4]);
        if (offset==1 || offset==2) attron(A_REVERSE);
        strftime(s,sizeof(s),"%x",tm);
        printw("%s",s);
        attroff(A_REVERSE);
        printw(" ");
        if (offset==3) attron(A_REVERSE);
        printw("%2d",tm->tm_hour);
        attroff(A_REVERSE);
        printw(":");
        if (offset==4) attron(A_REVERSE);
        printw("%02d",tm->tm_min);
        attroff(A_REVERSE);
        printw(" Modified ");
        tm=cpmtime(buf[entrystart+5],buf[entrystart+6],buf[entrystart+7],buf[entrystart+8]);
        if (offset==5 || offset==6) attron(A_REVERSE);
        strftime(s,sizeof(s),"%x",tm);
        printw("%s",s);
        attroff(A_REVERSE);
        printw(" ");
        if (offset==7) attron(A_REVERSE);
        printw("%2d",tm->tm_hour);
        attroff(A_REVERSE);
        printw(":");
        if (offset==8) attron(A_REVERSE);
        printw("%02d",tm->tm_min);
        attroff(A_REVERSE);

        move(16,0);
        printw("2nd last extent: Created/Accessed ");
        tm=cpmtime(buf[entrystart+11],buf[entrystart+12],buf[entrystart+13],buf[entrystart+14]);
        if (offset==11 || offset==12) attron(A_REVERSE);
        strftime(s,sizeof(s),"%x",tm);
        printw("%s",s);
        attroff(A_REVERSE);
        printw(" ");
        if (offset==13) attron(A_REVERSE);
        printw("%2d",tm->tm_hour);
        attroff(A_REVERSE);
        printw(":");
        if (offset==14) attron(A_REVERSE);
        printw("%02d",tm->tm_min);
        attroff(A_REVERSE);
        printw(" Modified ");
        tm=cpmtime(buf[entrystart+15],buf[entrystart+16],buf[entrystart+17],buf[entrystart+18]);
        if (offset==15 || offset==16) attron(A_REVERSE);
        strftime(s,sizeof(s),"%x",tm);
        printw("%s",s);
        attroff(A_REVERSE);
        printw(" ");
        if (offset==17) attron(A_REVERSE);
        printw("%2d",tm->tm_hour);
        attroff(A_REVERSE);
        printw(":");
        if (offset==18) attron(A_REVERSE);
        printw("%02d",tm->tm_min);
        attroff(A_REVERSE);

        move(17,0);
        printw("    Last extent: Created/Accessed ");
        tm=cpmtime(buf[entrystart+21],buf[entrystart+22],buf[entrystart+23],buf[entrystart+24]);
        if (offset==21 || offset==22) attron(A_REVERSE);
        strftime(s,sizeof(s),"%x",tm);
        printw("%s",s);
        attroff(A_REVERSE);
        printw(" ");
        if (offset==23) attron(A_REVERSE);
        printw("%2d",tm->tm_hour);
        attroff(A_REVERSE);
        printw(":");
        if (offset==24) attron(A_REVERSE);
        printw("%02d",tm->tm_min);
        attroff(A_REVERSE);
        printw(" Modified ");
        tm=cpmtime(buf[entrystart+25],buf[entrystart+26],buf[entrystart+27],buf[entrystart+28]);
        if (offset==25 || offset==26) attron(A_REVERSE);
        strftime(s,sizeof(s),"%x",tm);
        printw("%s",s);
        attroff(A_REVERSE);
        printw(" ");
        if (offset==27) attron(A_REVERSE);
        printw("%2d",tm->tm_hour);
        attroff(A_REVERSE);
        printw(":");
        if (offset==28) attron(A_REVERSE);
        printw("%02d",tm->tm_min);
        attroff(A_REVERSE);
      }
      /*}}}*/
      else if /* password */ /*{{{*/
      (buf[entrystart]>=16 && buf[entrystart]<=31 && sb.type==DR3)
      {
        int i;

        if (offset==0) attron(A_REVERSE);
        printw("Password");
        attroff(A_REVERSE);

        move(15,0);
        printw("Name: ");
        for (i=0; i<8; ++i)
        {
          if (offset==1+i) attron(A_REVERSE);
          printw("%c",buf[entrystart+1+i]&0x7f);
          attroff(A_REVERSE);
        }
        printw(" Extension: ");
        for (i=0; i<3; ++i)
        {
          if (offset==9+i) attron(A_REVERSE);
          printw("%c",buf[entrystart+9+i]&0x7f);
          attroff(A_REVERSE);
        }

        move(16,0);
        printw("Password required for: ");
        if (offset==12) attron(A_REVERSE);
        if (buf[entrystart+12]&0x80) printw("Reading ");
        if (buf[entrystart+12]&0x40) printw("Writing ");
        if (buf[entrystart+12]&0x20) printw("Deleting ");
        attroff(A_REVERSE);

        move(17,0);
        printw("Password: ");
        for (i=0; i<8; ++i)
        {
          char c;

          if (offset==16+(7-i)) attron(A_REVERSE);
          c=(buf[entrystart+16+(7-i)]^buf[entrystart+13])&0x7f;
          printw("%c",isprint(c) ? c : ' ');
          attroff(A_REVERSE);
        }
        printw(" XOR value: ");
        if (offset==13) attron(A_REVERSE);
        printw("0x%02x",buf[entrystart+13]&0xff);
        attroff(A_REVERSE);
      }
      /*}}}*/
      else /* bad status */ /*{{{*/
      {
        printw("Bad status ");
        if (offset==0) attron(A_REVERSE);
        printw("0x%02x",buf[entrystart]);
        attroff(A_REVERSE);
      }
      /*}}}*/
      if (!reload) data(&sb,buf,pos);
      switch (ch=getch())
      {
        case 'F': /* next entry */ /*{{{*/
        {
          if (pos+32<sb.bytes)
          {
            if (pos/sb.secLength!=(pos+32)/sb.secLength) reload=1;
            pos+=32;
          }
          break;
        }
        /*}}}*/
        case 'B': /* previous entry */ /*{{{*/
        {
          if (pos>=32)
          {
            if (pos/sb.secLength!=(pos-32)/sb.secLength) reload=1;
            pos-=32;
          }
          break;
        }
        /*}}}*/
      }
    }
    /*}}}*/
    else /* data area */ /*{{{*/
    {
      const char *msg;

      msg="Data area"; move(0,(COLS-strlen(msg))/2); printw(msg);
      if (!reload) data(&sb,buf,pos);
      ch=getch();
    }
    /*}}}*/

    /* process common commands */ /*{{{*/
    switch (ch)
    {
      case 'n': /* next record */ /*{{{*/
      {
        if (pos+128<sb.bytes);
        {
          if (pos/sb.secLength!=(pos+128)/sb.secLength) reload=1;
          pos+=128;
        }
        break;
      }
      /*}}}*/
      case 'p': /* previous record */ /*{{{*/
      {
        if (pos>=128)
        {
          if (pos/sb.secLength!=(pos-128)/sb.secLength) reload=1;
          pos-=128;
        }
        break;
      }
      /*}}}*/
      case 'N': /* next track */ /*{{{*/
      {
        if ((pos+sb.sectrk*sb.secLength)<sb.bytes)
        {
          pos+=sb.sectrk*sb.secLength;
          reload=1;
        }
        break;
      }
      /*}}}*/
      case 'P': /* previous track */ /*{{{*/
      {
        if (pos>sb.sectrk*sb.secLength)
        {
          pos-=sb.sectrk*sb.secLength;
          reload=1;
        }
        break;
      }
      /*}}}*/
      case 'b': /* byte back */ /*{{{*/
      {
        if (pos)
        {
          if (pos/sb.secLength!=(pos-1)/sb.secLength) reload=1;
          --pos;
        }
        break;
      }
      /*}}}*/
      case 'f': /* byte forward */ /*{{{*/
      {
        if (pos+1<sb.tracks*sb.sectrk*sb.secLength)
        {
          if (pos/sb.secLength!=(pos+1)/sb.secLength) reload=1;
          ++pos;
        }
        break;
      }
      /*}}}*/
      case 'i': info(&sb,format,image); break;
      case 'm': map(&sb); break;
    }
    /*}}}*/
  } while (ch!='q');

  /* exit curses */ /*{{{*/
  move(LINES-1,0);
  refresh();
  echo();
  noraw();
  endwin();
  /*}}}*/
  exit(0);
}
/*}}}*/
