#ifndef _HELPERS_H_
#define _HELPERS_H_

typedef struct paddings {
	int block;
	int group;
	int inode;
	int link;
	int major;
	int minor;
	int size;
	int user;
} paddings;

void getimmediatechildren(FTSENT*, int, struct paddings*);
int countblocks(FTSENT*, struct paddings*);
void getpaddingsizes(struct stat, struct paddings*);
int countdigits(long);
int parseargs(int, char*[]);
void splitargs(int, char *[], int);

#endif /* !_HELPERS_H_  */
