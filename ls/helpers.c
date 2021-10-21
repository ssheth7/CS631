/*
 * Contains helper methods for traversal
*/
#include <sys/stat.h>

#include <errno.h>
#include <fts.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ls.h"


/* flags  */
extern int A_allentries;        /* Includes all files but current/previous directory  */
extern int a_allentries;        /* Includes hidden files  */
extern int c_lastchanged;       /* Shows last changed  */
extern int d_directories;       /* Shows directories as plain files */
extern int F_specialsymbols;    /* Adds symbols to special files */
extern int f_unsorted;          /* Files are unsorted */
extern int h_humanreadable;     /* Shows sizes in readable format */
extern int i_inodes;            /* Shows file inodes */
extern int k_kilobytes;         /* Shows size in kilobytes*/
extern int l_longformat;        /* Shows long format */                                   
extern int n_numericalids;      /* Shows IDs numerically */                               
extern int q_forcenonprintable; /* Forces nonprintable characters */                      
extern int R_recurse;           /* Recurses into directories */                           
extern int r_reverseorder;      /* Reverses output*/                                      
extern int S_sizesorted;        /* Sorts by size */                                       
extern int s_systemblocks;      /* Shows number of blocks used  */                        
extern int t_modifiedsorted;    /* Sort by time modified */                               
extern int u_lastaccess;        /* Shows time of last access  */                          
extern int w_forcenonprintable; /* Forces raw nonprintable characters  */ 

extern long BLOCKSIZE;
extern int EXIT_STATUS;
extern int NUMDIRS;
extern int NUMNONDIRS;
extern char **DIRS;
extern char **NONDIRS;

/*
 * Gets all the immediate children of a directory and formats each child
*/
void
getimmediatechildren(FTSENT* children, int parentlevel, paddings *paddings) 
{

	while(children != NULL) {
		if (children->fts_level == parentlevel + 1) {
			formatentry(0, children->fts_name, children->fts_path,
			  *children->fts_statp, *paddings);
		}
		children = children->fts_link;
	}
}

/*
 * Counts the number of block in a directory given a directory structure
*/
int 
countblocks(FTSENT *children, paddings *paddings)
{
	int blocks, blocksum, printallflags, size, total;
	
	printallflags = A_allentries || a_allentries; 
	blocksum = 0;
	size = 0;
	total = 0;

	while (children != NULL) {
		while(children != NULL) {
			if (printallflags || strncmp(children->fts_name, ".", 1) != 0) {
				
				blocks = children->fts_statp->st_blocks;
				size = children->fts_statp->st_size;	
				total += size;
				blocksum += blocks;
	
				getpaddingsizes(*children->fts_statp, paddings); 
			}
			children = children->fts_link;
		}
	}
	if (h_humanreadable == 1) {
		return total;
	}
	return blocksum;
}

/*
 * Updates the print paddings if longer elements are provided
*/
void
getpaddingsizes(struct stat sb, paddings *paddings) 
{
	int digitlen, gid, links, majornum, minornum, uid;
	blkcnt_t blocks;
	dev_t devicetype;
	ino_t inode;
	mode_t mode;
	off_t size;
	struct group *group;
	struct passwd *passwd;

	blocks = sb.st_blocks;
	size = sb.st_size;
	uid = sb.st_uid;
	gid = sb.st_gid;
	mode = sb.st_mode;
	devicetype = sb.st_rdev;
	inode = sb.st_ino;
	links = sb.st_nlink;
	digitlen = 0;
	
	digitlen = countdigits(blocks);
	if (digitlen > paddings->block) {
		paddings->block = digitlen;
	}

	digitlen = countdigits(size);
	if (digitlen > paddings->size) {
		paddings->size = digitlen;
	}

	digitlen = countdigits(links);
	if (digitlen > paddings->link) {
		paddings->link = digitlen;
	}

	if (n_numericalids == 1 || (passwd = getpwuid(uid)) == NULL) {
		digitlen = countdigits(uid) + 1;
	} else {
		digitlen = strlen(passwd->pw_name) + 1;
	}
	if (digitlen > paddings->user) {
		paddings->user = digitlen;
	}

	if (n_numericalids == 1 || (group = getgrgid(gid)) == NULL) {
		digitlen = countdigits(gid);	
	} else {
		digitlen = strlen(group->gr_name);

	}

	if (digitlen > paddings->group) {
		paddings->group = digitlen;
	}
	
	digitlen = countdigits(inode);
	if (digitlen > paddings->inode) {
		paddings->inode = digitlen;
	}

	if (S_ISBLK(mode) || S_ISCHR(mode)) {
		majornum = major(devicetype);
		minornum = minor(devicetype);	
		digitlen = countdigits(majornum) + 1;
		if (digitlen > paddings->major) {
			paddings->major = digitlen;
		}
		digitlen = countdigits(minornum) + 1;
		if (digitlen > paddings->minor) {
			paddings->minor = digitlen;
		}
	}
}
/*
 * Counts the digits in a number
*/
int
countdigits(long digit)
{
	int numdigits = 0;
	
	while (digit > 0){
		numdigits++;
		digit/=10;
	}
	return numdigits;
}

/*
 * Parses command line arguments and sets option flags
*/
int
parseargs(int argc, char **argv)
{	
	int opt, flagsset, envlength;
	char* blocksize;
		
	if (isatty(1)) {
		q_forcenonprintable = 1;	
	}
	BLOCKSIZE = 512; 
	flagsset = 0;
	while ((opt = getopt(argc, argv, "AacdFfhiklnqRrSstuw")) != -1) {
		flagsset = 1;
		switch (opt) {
			case 'A':
				A_allentries = 1;
				break;
			case 'a':
				a_allentries = 1;
				A_allentries = 0;
				break;
			case 'c':
				if (f_unsorted == 1) {
					break;
				}
				c_lastchanged = 1;
				u_lastaccess = 0;
				break;
			case 'd':
				d_directories = 1;
				break;
			case 'F':
				F_specialsymbols = 1;
				break;
			case 'f':
				f_unsorted = a_allentries = 1;
				r_reverseorder = S_sizesorted = u_lastaccess = c_lastchanged = 0;
				break;
			case 'h':
				h_humanreadable = 1;
				k_kilobytes = 0;
				break;
			case 'i':
				i_inodes = 1;
				break;
			case 'k':
				k_kilobytes = 1;
				h_humanreadable = 0;
				BLOCKSIZE = 1024;
				break;
			case 'l':
				l_longformat = 1;
				n_numericalids = 0;
				break;
			case 'n':
				n_numericalids = 1;
				l_longformat = 0;
				break;
			case 'q':
				q_forcenonprintable = 1;
				break;
			case 'R':
				R_recurse = 1;
				break;
			case 'r':
				if (f_unsorted == 1) {
					break;
				}
				r_reverseorder = 1;
				break;
			case 'S':
				if (f_unsorted == 1) {
					break;
				}
				S_sizesorted = 1;
				break;
			case 's':
				s_systemblocks = 1;
				break;
			case 't':
				if (f_unsorted == 1) {
					break;
				}
				t_modifiedsorted = 1;
				break;
			case 'u':
				if (f_unsorted == 1) {
					break;
				}
				u_lastaccess = 1;
				c_lastchanged = 0;
				break;
			case 'w':
				q_forcenonprintable = 0;
				break;
			default:
				(void)fprintf(stderr, "%s [-AacdFfhiklnqRrSstuw] [file ...]\n", getprogname());
				/* NOTREACHED */
				exit(EXIT_FAILURE);
		}
	}
	if (getuid() == 0 && a_allentries == 0) {
		A_allentries = 1;
	}
	if (s_systemblocks == 1 && h_humanreadable == 0 && k_kilobytes == 0) {
		getbsize(NULL, &BLOCKSIZE);
	}
	return flagsset;
}

/*
 * Splits arguments into DIR and NONDIR arrays
*/
void
splitargs(int argc, char **argv, int offset)
{
	int i, dirindex, nondirindex, arglength;
	struct stat sb;
	
	dirindex = 0;
	nondirindex = 0;
	
	if (NUMDIRS == 0 && NUMNONDIRS == 0) { /* If no command line arguments are provided */
		if (d_directories == 0){
			NUMDIRS++;
			if ((DIRS = malloc(NUMDIRS * sizeof(char*))) == NULL) {
				(void)fprintf(stderr, "Memory could not be allocated");
				exit(EXIT_FAILURE);
			}
			if ((DIRS[0] = malloc(strlen(".") + 1)) == NULL) {
				(void)fprintf(stderr, "Could not allocate memory.\n");
				exit(EXIT_FAILURE);
			}
			if (strncpy(DIRS[0], ".", strlen(".")) == NULL) {
				(void)fprintf(stderr, "Could not copy to destination\n");
				exit(EXIT_FAILURE);
			}
		} else {
			NUMNONDIRS++;
			if ((NONDIRS = malloc(NUMNONDIRS * sizeof(char*))) == NULL) {
				(void)fprintf(stderr, "Memory could not be allocated");
				exit(EXIT_FAILURE);
			}
			if ((NONDIRS[0] = malloc(strlen(".") + 1)) == NULL) {
				(void)fprintf(stderr, "Could not allocate memory.\n");
				exit(EXIT_FAILURE);
			}
			if (strncpy(NONDIRS[0], ".", strlen(".")) == NULL) {
				(void)fprintf(stderr, "Could not copy to destination\n");
				exit(EXIT_FAILURE);
			}
		}
	}
	
	for (i = 1 + offset; i < argc; i++){

		if (lstat(argv[i], &sb) == 0) {
			arglength = strlen(argv[i]);
			if (S_ISDIR(sb.st_mode) && d_directories == 0) {
				if ((DIRS[dirindex] = malloc(arglength + 1)) == NULL) {
					(void)fprintf(stderr, "Could not allocate memory.\n");
				}
				if (strncpy(DIRS[dirindex], argv[i], arglength) == NULL) {
					(void)fprintf(stderr, "Could not copy %s to buffer.\n", argv[i]);
				}
				dirindex++;
			} else {
				if ((NONDIRS[nondirindex] = malloc(arglength + 1)) == NULL) {
					(void)fprintf(stderr, "Could not allocate memory.\n");
				}
				
				if (strncpy(NONDIRS[nondirindex], argv[i], arglength) == NULL) {
					(void)fprintf(stderr, "Could not copy %s to buffer.\n", argv[i]);
				}
				nondirindex++;
			}
		} else {
			EXIT_STATUS = EXIT_FAILURE;
		}
	}
}
