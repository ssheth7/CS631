/*
 * 10/15/2021
*/

#include <sys/stat.h>

#include <machine/int_fmtio.h>

#include <ctype.h>
#include <errno.h>
#include <fts.h>
#include <grp.h>
#include <pwd.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "helpers.h"

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

/* 
 * Ceiling divide to calculate the number of blocks with a given blocksize 
*/
#define CEIL(w, x) (1 + (w - 1)/(x / 512))

/*
 * Modifies string pointers to include mode symbols
*/
void
addsymbols_F(char** entry, struct stat sb)
{
	char appended;
	int entrymode, symboladded;
	char* modifiedentry;
	
	if ((modifiedentry = malloc(strlen(*entry) + 2)) == NULL) {
		(void)fprintf(stderr, "Could not allocate memory: %s\n", strerror(errno));
		EXIT_STATUS = EXIT_FAILURE;
	}
	
	entrymode = sb.st_mode;
	symboladded = 1;

	if (S_ISDIR(entrymode)) {
		appended = '/';
	} else if (S_ISLNK(entrymode)) {
		appended = '@';
	} else if (S_ISSOCK(entrymode)) {
		appended = '=';
	} else if(S_ISFIFO(entrymode)) {
		appended = '|';
	} else if (S_ISWHT(entrymode)) {
		appended = '%';
	} else if (S_IEXEC & entrymode) {
		appended = '*';
	} else {
		symboladded = 0;
	}
	if (symboladded == 1) {
		snprintf(modifiedentry, strlen(*entry) + 2, "%s%c", *entry, appended);
		*entry = modifiedentry; 
	}
	free(modifiedentry);
}

/*
 * Basic printing function
*/
void
printdefault(char* entry)
{
	
	(void)printf("%s\n", entry);
}

/*
 * Replaces all unprintable characters with '?'
*/
void
printraw_q(char* entry)
{
	int index = 0;

	while(entry[index]) {
		if (isprint(entry[index])) {
			(void)printf("%c", entry[index]);
		} else {
			(void)printf("?");
		}
		index++;
	}
	if (l_longformat == 0 && n_numericalids == 0) {
		(void)printf("\n");
	}
}

void
printinode_i(struct stat sb, int padding) 
{
	
	(void)printf("%*ld ", padding, sb.st_ino);
}

/*
 * Formats blocks and prints files blocks and total blocks
*/
void
printblocks_s(int blocks, int size, int isEntry, int blockpadding)
{
	int humanizeflags;
	char buf[6];
	long defaultblocksize; 
	
	if (h_humanreadable == 0) {
		if (isEntry == 1) {
			(void)printf("%*" PRIdMAX " ", blockpadding, (intmax_t)CEIL(blocks, 
				BLOCKSIZE));
		} else {
			(void)printf("total %" PRIdMAX "\n", (intmax_t)CEIL(blocks, 
				BLOCKSIZE));
		}
		return;	
	}
	
	if ((n_numericalids  == 1|| l_longformat == 1) && isEntry == 1) {
		size = blocks * BLOCKSIZE;
	}
	
	humanizeflags = HN_DECIMAL | HN_B | HN_NOSPACE;
	if (humanize_number(buf, sizeof(buf), size , " ",  
		HN_AUTOSCALE, humanizeflags) == -1) {
		(void)fprintf(stderr, "Humanize function failed: %s\n", strerror(errno)); 
		EXIT_STATUS = EXIT_FAILURE;
	}
	if (isEntry == 1) {
		(void)printf("%5s",  buf);
	} else {
		(void)printf("total %s\n", buf);
	}
}

/* 
 * Prints the long format of an entry 
*/ 
void
printlong_l(char* entry, char* path, struct stat sb, 
paddings paddings) 
{
	char* timeformat;
	int invalidgid, invaliduid, humanizeflags; /* Local print flags  */
	int entrymode, majornum, minornum, numlinks;
	int hpadding, len;
	int currentyear, fileyear;	
	uid_t uid;
	gid_t gid;
	char sizebuf[6];
	dev_t devicetype;
	time_t currentepochtime, fileepochtime;
	char permbuf[12];
	char timebuf[13];
	char linkbuf[PATH_MAX], fullpath[PATH_MAX];
	struct group *group;
	struct passwd *passwd;
	struct tm *currenttime, *filetime;

	
	entrymode = sb.st_mode;
	(void)strmode(entrymode, permbuf);
	permbuf[11] == '\0';	
	
	numlinks = sb.st_nlink;
	
	uid = sb.st_uid;
	gid = sb.st_gid;
	
	invalidgid = 0;
	invaliduid = 0;
	if ((group = getgrgid(gid)) == NULL) {
		invalidgid = 1;
	} 
	if ((passwd = getpwuid(uid)) == NULL) {
		invaliduid = 1;
	}

	currentepochtime = time(NULL);
	currenttime = localtime(&currentepochtime);
	currentyear = currenttime->tm_year + 1900;
	
	if (c_lastchanged) {
		fileepochtime = sb.st_ctime;
	} else if (u_lastaccess) {
		fileepochtime = sb.st_atime;
	} else {
		fileepochtime = sb.st_mtime;
	}
	filetime = localtime(&fileepochtime);
	fileyear = filetime->tm_year + 1900;	
	
	timeformat = "%b %e %k:%M";
	if (fileyear != currentyear) {
		timeformat = "%b %e  %Y";
	} 
	if (strftime(timebuf, sizeof(timebuf), timeformat, filetime) == 0) {
		(void)fprintf(stderr, "Could not format date of %s.\n", entry); 
		EXIT_STATUS = EXIT_FAILURE;
	}
	timebuf[12] = '\0';

	if (n_numericalids == 1) {
		invalidgid = 1;
		invaliduid = 1;
	}
	
	if (invalidgid == 1 && invaliduid == 0) {
		(void)printf("%s %*d %-*s %-*d", 
		permbuf, paddings.link, numlinks, paddings.user, 
		passwd->pw_name, paddings.group, gid);
	} else if (invalidgid == 0 && invaliduid == 1) {
		(void)printf("%s %*d %-*d %-*s", 
		permbuf, paddings.link, numlinks, paddings.user, uid, 
		paddings.group, group->gr_name);
	} else if (invalidgid == 1 && invaliduid == 1) {
		(void)printf("%s %*d %-*d %-*d", 
		permbuf, paddings.link, numlinks, paddings.user, uid, 
		paddings.group, gid);
	} else {
		(void)printf("%s %*d %-*s %-*s", 
		permbuf, paddings.link, numlinks, paddings.user, passwd->pw_name,
		 paddings.group, group->gr_name);
	}

	devicetype = sb.st_rdev;
	majornum = major(devicetype);
	minornum = minor(devicetype);
	
	if (h_humanreadable == 1) {
		hpadding = sizeof(sizebuf);	
		/* Major/Minor numbers take up more space than size  */
		if (paddings.minor + paddings.major  > 6) {
			hpadding = paddings.minor + paddings.major + 2;	
		} else {  /* Size numbers take up more space than major/minor numbers  */
			paddings.major = hpadding - paddings.minor - paddings.major;
		}
		
		if (S_ISBLK(entrymode) || S_ISCHR(entrymode)) {
			(void)printf(" %*d,%*d ", paddings.major, majornum, paddings.minor, minornum);
		} else {
			humanizeflags = HN_DECIMAL | HN_B | HN_NOSPACE;
			if (humanize_number(sizebuf, sizeof(sizebuf), sb.st_size , " ",  
				HN_AUTOSCALE, humanizeflags) == -1) {
				(void)fprintf(stderr, "Humanize function failed: %s\n", strerror(errno)); 
				EXIT_STATUS = EXIT_FAILURE;
			}
			(void)printf(" %*s", hpadding, sizebuf);	
		}
	} else {
		/* Major/Minor numbers take up more space than size  */
		if (paddings.minor + paddings.major> paddings.size) {
			paddings.size = paddings.minor + paddings.major;
		} else {  /* Size numbers take up more space than major/minor numbers  */
			paddings.major = paddings.size - paddings.minor - paddings.major + 2;
		}
		
		if (S_ISBLK(entrymode) || S_ISCHR(entrymode)) {
			(void)printf(" %*d,%*d ", paddings.major, majornum, paddings.minor, minornum);
		} else { 
			(void)printf(" %*" PRIdMAX " ", (intmax_t)(paddings.size + 1), sb.st_size);
		}
	}
	
	(void)printf("%11s ", timebuf);
	
	if (q_forcenonprintable == 1) {
		printraw_q(entry);
	} else {
		(void)printf("%s ", entry);
	}

	/* Build entry path for readlink(1) */
	len = strlen(path) + 1 + strlen(basename(entry));
	if (len != snprintf(fullpath, PATH_MAX, "%s/%s", path, basename(entry))) {
		fprintf(stderr, "Could not format path off entry: %s\n", entry);
		EXIT_STATUS = EXIT_FAILURE;
	}
	fullpath[len] = '\0';
	
	if (S_ISLNK(entrymode)) {
		if ((len = readlink(fullpath, linkbuf, sizeof(linkbuf) - 1)) != -1) {
			linkbuf[len] = '\0';
			printf(" -> %s", linkbuf);
		} 
	}
	
	printf("\n");
}
