/* #includes */ /*{{{C}}}*//*{{{*/
#undef  _POSIX_SOURCE
#define _POSIX_SOURCE   1
#undef  _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 2

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
extern char *optarg;
extern int optind,opterr,optopt;
int getopt(int argc, char * const *argv, const char *optstring);

#include "cpmfs.h"
/*}}}*/

const char cmd[]="cpmcp";
static int text=0;

/**
 * Return the user number.
 * @param s CP/M filename in 0[0]:aaaaaaaa.bbb format.
 * @returns The user number or -1 for no match.
 */
static int userNumber(const char *s) /*{{{*/
{
  if (isdigit(*s) && *(s+1)==':') return (*s-'0');
  if (isdigit(*s) && isdigit(*(s+1)) && *(s+2)==':') return (10*(*s-'0')+(*(s+1)));
  return -1;
}
/*}}}*/

/**
 * Copy one file from CP/M to UNIX.
 * @param root The inode for the root directory.
 * @param src  The CP/M filename in 00aaaaaaaabbb format.
 * @param dest The UNIX filename.
 * @returns 0 for success, 1 for error.
 */
static int cpmToUnix(const struct cpmInode *root, const char *src, const char *dest) /*{{{*/
{
  struct cpmInode ino;
  int exitcode=0;

      if (cpmNamei(root,src,&ino)==-1) { fprintf(stderr,"%s: can not open %s: %s\n",cmd,src,boo); exitcode=1; }
      else
      {
        struct cpmFile file;
        FILE *ufp;

        cpmOpen(&ino,&file,O_RDONLY);
        if ((ufp=fopen(dest,text ? "w" : "wb"))==(FILE*)0) { fprintf(stderr,"%s: can not create %s: %s\n",cmd,dest,strerror(errno)); exitcode=1; }
        else
        {
          int crpending=0;
          int ohno=0;
          int res;
          char buf[4096];

          while ((res=cpmRead(&file,buf,sizeof(buf)))!=0)
          {
            int j;

            for (j=0; j<res; ++j)
            {
              if (text)
              {
                if (buf[j]=='\032') goto endwhile;
                if (crpending)
                {
                  if (buf[j]=='\n') 
                  {
                    if (putc('\n',ufp)==EOF) { fprintf(stderr,"%s: can not write %s: %s\n",cmd,dest,strerror(errno)); exitcode=1; ohno=1; goto endwhile; }
                    crpending=0;
                  }
                  else if (putc('\r',ufp)==EOF) { fprintf(stderr,"%s: can not write %s: %s\n",cmd,dest,strerror(errno)); exitcode=1; ohno=1; goto endwhile; }
                  crpending=(buf[j]=='\r');
                }
                else
                {
                  if (buf[j]=='\r') crpending=1;
                  else if (putc(buf[j],ufp)==EOF) { fprintf(stderr,"%s: can not write %s: %s\n",cmd,dest,strerror(errno)); exitcode=1; ohno=1; goto endwhile; }
                }
              }
              else if (putc(buf[j],ufp)==EOF) { fprintf(stderr,"%s: can not write %s: %s\n",cmd,dest,strerror(errno)); exitcode=1; ohno=1; goto endwhile; }
            }
          }
          endwhile:
          if (fclose(ufp)==EOF && !ohno) { fprintf(stderr,"%s: can not close %s: %s\n",cmd,dest,strerror(errno)); exitcode=1; }
        }
        cpmClose(&file);
      }
      return exitcode;
}
/*}}}*/

static void usage(void) /*{{{*/
{
  fprintf(stderr,"Usage: %s [-f format] [-t] image 0:file file\n",cmd);
  fprintf(stderr,"       %s [-f format] [-t] image 0:file ... directory\n",cmd);
  fprintf(stderr,"       %s [-f format] [-t] image file 0:file\n",cmd);
  fprintf(stderr,"       %s [-f format] [-t] image file ... 0:\n",cmd);
  exit(1);
}
/*}}}*/

int main(int argc, char *argv[])
{
  /* variables */ /*{{{*/
  char *image;
  const char *format=FORMAT;
  int c,readcpm=-1,todir=-1;
  struct cpmInode root;
  struct cpmSuperBlock drive;
  int exitcode=0;
  /*}}}*/

  /* parse options */ /*{{{*/
  while ((c=getopt(argc,argv,"f:h?t"))!=EOF) switch(c)
  {
    case 'f': format=optarg; break;
    case 'h':
    case '?': usage(); break;
    case 't': text=1; break;
  }
  /*}}}*/
  /* parse arguments */ /*{{{*/
  if ((optind+2)>=argc) usage();
  image=argv[optind];

  if (userNumber(argv[optind+1])>=0) /* cpm -> unix? */ /*{{{*/
  {
    int i;
    struct stat statbuf;

    for (i=optind+1; i<(argc-1); ++i) if (userNumber(argv[i])==-1) usage();
    todir=((argc-optind-1)>2);
    if (stat(argv[argc-1],&statbuf)==-1) { if (todir) usage(); }
    else if (S_ISDIR(statbuf.st_mode)) todir=1; else if (todir) usage();
    readcpm=1;
  }
  /*}}}*/
  else if (userNumber(argv[argc-1])>=0) /* unix -> cpm */ /*{{{*/
  {
    int i;

    todir=0;
    for (i=optind+1; i<(argc-1); ++i) if (userNumber(argv[i])>=0) usage();
    if ((argc-optind-1)>2 && *(strchr(argv[argc-1],':')+1)!='\0') usage();
    if (*(strchr(argv[argc-1],':')+1)=='\0') todir=1;
    readcpm=0;
  }
  /*}}}*/
  else usage();
  /*}}}*/
  /* open image file */ /*{{{*/
  if ((drive.fd = open(image,readcpm ? O_RDONLY : O_RDWR)) == -1 ) 
  {
    fprintf(stderr,"%s: can not open %s\n",cmd,image);
    exit(1);
  }
  getformat(format,&drive,&root);
  /*}}}*/
  if (readcpm) /* copy from CP/M to UNIX */ /*{{{*/
  {
    int i,count,total=0;
    char src[2+8+1+3+1];
    struct cpmFile dir;
    struct cpmDirent dirent;

    /* expand pattern and check if we try to copy multiple files to a file */ /*{{{*/
    for (i=optind+1,total=0; i<(argc-1); ++i)
    {
      cpmOpendir(&root,&dir);
      count=0;
      while (cpmReaddir(&dir,&dirent)) if (match(dirent.name,argv[i])) ++count;
      if (count==0) ++total; else total+=count;
      cpmClose(&dir);
    }
    if (total>1 && !todir) usage();
    /*}}}*/

    for (i=optind+1; i<(argc-1); ++i)
    {
      char dest[_POSIX_PATH_MAX];

      count=0;
      cpmOpendir(&root,&dir);
      while (cpmReaddir(&dir,&dirent))
      {
        if (match(dirent.name,argv[i]))
        {
          if (todir)
          {
            strcpy(dest,argv[argc-1]);
            strcat(dest,"/");
            strcat(dest,dirent.name+2);
          }
          else strcpy(dest,argv[argc-1]);
          if (cpmToUnix(&root,dirent.name,dest)) exitcode=1;
          ++count;
        }
      }
      cpmClose(&dir);
      if (count==0) /* try the pattern itself and find it does not work :) */ /*{{{*/
      {
        if (todir)
        {
          strcpy(dest,argv[argc-1]);
          strcat(dest,"/");
          strcat(dest,src+2);
        }
        else strcpy(dest,argv[argc-1]);
        if (cpmToUnix(&root,src,dest)) exitcode=1;
      }
      /*}}}*/
    }
  }
  /*}}}*/
  else /* copy from UNIX to CP/M */ /*{{{*/
  {
    int i;

    for (i=optind+1; i<(argc-1); ++i)
    {
      /* variables */ /*{{{*/
      char *dest=(char*)0;
      FILE *ufp;
      /*}}}*/

      if ((ufp=fopen(argv[i],"r"))==(FILE*)0) /* cry a little */ /*{{{*/
      {
        fprintf(stderr,"%s: can not open %s: %s\n",cmd,argv[i],strerror(errno));
        exitcode=1;
      }
      /*}}}*/
      else
      {
        struct cpmInode ino;
        char cpmname[2+8+3+1]; /* 00foobarxy.zzy\0 */

        if (todir)
        {
          if ((dest=strrchr(argv[i],'/'))!=(char*)0) ++dest; else dest=argv[i];
          sprintf(cpmname,"%02d%s",userNumber(argv[argc-1]),dest);
        }
        else
        {
          sprintf(cpmname,"%02d%s",userNumber(argv[argc-1]),strchr(argv[argc-1],':')+1);
        }
        if (cpmCreat(&root,cpmname,&ino,0666)==-1) /* just cry */ /*{{{*/
        {
          fprintf(stderr,"%s: can not create %s: %s\n",cmd,cpmname,boo);
          exitcode=1;
        }
        /*}}}*/
        else
        {
          struct cpmFile file;
          int ohno=0;
          char buf[4096+1];

          cpmOpen(&ino,&file,O_WRONLY);
          do
          {
            int j;

            for (j=0; j<(sizeof(buf)/2) && (c=getc(ufp))!=EOF; ++j)
            {
              if (text && c=='\n') buf[j++]='\r';
              buf[j]=c;
            }
            if (text && c==EOF) buf[j++]='\032';
            if (cpmWrite(&file,buf,j)!=j)
            {
              fprintf(stderr,"%s: can not write %s: %s\n",cmd,dest,boo);
              ohno=1;
              exitcode=1;
              break;
            }
          } while (c!=EOF);
          if (cpmClose(&file)==EOF && !ohno) /* I just can't hold back the tears */ /*{{{*/
          {
            fprintf(stderr,"%s: can not close %s: %s\n",cmd,dest,boo);
            exitcode=1;
          }
          /*}}}*/
        }
        fclose(ufp);
      }
    }
  }
  /*}}}*/
  cpmUmount(&drive);
  exit(exitcode);
}
