#ifndef CPMFS_H
#define CPMFS_H

#include <sys/stat.h>
#include <sys/types.h>

/* eventually all tools will communicate with this module like a VFS */
struct cpmInode
{
  ino_t ino;
  mode_t mode;
  off_t size;
  time_t atime;
  time_t mtime;
  time_t ctime;
  struct cpmSuperBlock *sb;
};

struct cpmFile
{
  mode_t mode;
  off_t pos;
  struct cpmInode *ino;
};

struct cpmDirent
{
  ino_t ino;
  off_t off;
  size_t reclen;
  char name[2+8+1+3+1]; /* 00foobarxy.zzy\0 */
};

struct cpmStat
{
  ino_t ino;
  mode_t mode;
  off_t size;
  time_t atime;
  time_t mtime;
  time_t ctime;
};

#define CPMFS_DR22  0
#define CPMFS_P2DOS 1
#define CPMFS_DR3   2

struct cpmSuperBlock
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

  int size;
  int extents;
  struct PhysDirectoryEntry *dir;
  int alvSize;
  int *alv;
  int *skewtab;
  int cnotatime;
  char *label;
  size_t labelLength;
  char *passwd;
  size_t passwdLength;
  struct cpmInode *root;
};

struct cpmStatFS
{
  long f_bsize;
  long f_blocks;
  long f_bfree;
  long f_bused;
  long f_bavail;
  long f_files;
  long f_ffree;
  long f_namelen;
};

extern const char cmd[];
extern const char *boo;

int getformat(const char *format, struct cpmSuperBlock *drive, struct cpmInode *root);
int match(const char *a, const char *pattern);
void glob(int opti, int argc, char *argv[], struct cpmInode *root, int *gargc, char ***gargv);

int cpmNamei(const struct cpmInode *dir, const char *filename, struct cpmInode *i);
void cpmStatFS(const struct cpmInode *ino, struct cpmStatFS *buf);
int cpmUnlink(const struct cpmInode *dir, const char *fname);
int cpmRename(const struct cpmInode *dir, const char *old, const char *new);
int cpmOpendir(struct cpmInode *dir, struct cpmFile *dirp);
int cpmReaddir(struct cpmFile *dir, struct cpmDirent *ent);
void cpmStat(const struct cpmInode *ino, struct cpmStat *buf);
int cpmOpen(struct cpmInode *ino, struct cpmFile *file, mode_t mode);
int cpmRead(struct cpmFile *file, char *buf, int count);
int cpmWrite(struct cpmFile *file, const char *buf, int count);
int cpmClose(struct cpmFile *file);
int cpmCreat(struct cpmInode *dir, const char *fname, struct cpmInode *ino, mode_t mode);
int cpmSync(struct cpmSuperBlock *sb);
void cpmUmount(struct cpmSuperBlock *sb);

#endif
