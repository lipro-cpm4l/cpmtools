/* #includes */ /*{{{C}}}*//*{{{*/
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
extern char *optarg;
extern int optind,opterr,optopt;
int getopt(int argc, char * const *argv, const char *optstring);

#include "cpmfs.h"
/*}}}*/

const char cmd[]="cpmrm";

int main(int argc, char *argv[]) /*{{{*/
{
  /* variables */ /*{{{*/
  char *image;
  const char *format=FORMAT;
  int c,i,usage=0,exitcode=0;
  struct cpmSuperBlock drive;
  struct cpmInode root;
  int gargc;
  char **gargv;
  /*}}}*/

  /* parse options */ /*{{{*/
  while ((c=getopt(argc,argv,"f:h?"))!=EOF) switch(c)
  {
    case 'f': format=optarg; break;
    case 'h':
    case '?': usage=1; break;
  }

  if (optind>=(argc-1)) usage=1;
  else image=argv[optind];

  if (usage)
  {
    fprintf(stderr,"Usage: %s [-f format] image pattern ...\n",cmd);
    exit(1);
  }
  /*}}}*/
  /* open image */ /*{{{*/
  if ((drive.fd=open(image,O_RDWR))==-1) 
  {
    fprintf(stderr,"%s: can not open %s: %s\n",cmd,image,strerror(errno));
    exit(1);
  }
  getformat(format,&drive,&root);
  /*}}}*/
  glob(optind,argc,argv,&root,&gargc,&gargv);
  for (i=0; i<gargc; ++i)
  {
    if (cpmUnlink(&root,gargv[i])==-1)
    {
      fprintf(stderr,"%s: can not erase %s: %s\n",cmd,gargv[i],boo);
      exitcode=1;
    }
  }
  exit(exitcode);
}
/*}}}*/
