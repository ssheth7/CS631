/*
 * "Advanced Programming in the UNIX Environment" midterm assignment
 * Based on NETBSD's ls(1)
*/

/* Shivam Sheth (ssheth7)
 * ls: list the files in a directory
 * 10/15/2021
*/

#include <sys/stat.h>

#include <errno.h>
#include <fts.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cmp.h"
#include "helpers.h"
#include "print.h"

/* flags  */
int A_allentries;        /* Includes all files but current/previous directory  */
int a_allentries;        /* Includes hidden files  */
int c_lastchanged;       /* Shows last changed  */
int d_directories;       /* Shows directories as plain files */
int F_specialsymbols;    /* Adds symbols to special files */
int f_unsorted;          /* Files are unsorted */
int h_humanreadable;     /* Shows sizes in readable format */
int i_inodes;            /* Shows file inodes */
int k_kilobytes;         /* Shows size in kilobytes*/
int l_longformat;        /* Shows long format */                                   
int n_numericalids;      /* Shows IDs numerically */                               
int q_forcenonprintable; /* Forces nonprintable characters */                      
int R_recurse;           /* Recurses into directories */                           
int r_reverseorder;      /* Reverses output*/                                      
int S_sizesorted;        /* Sorts by size */                                       
int s_systemblocks;      /* Shows number of blocks used  */                        
int t_modifiedsorted;    /* Sort by time modified */                               
int u_lastaccess;        /* Shows time of last access  */                          

long BLOCKSIZE;		 /* BLOCKSIZE env variable  */
int EXIT_STATUS; 
int NUMDIRS; 		 /* Number of directory arguments  */
int NUMNONDIRS; 	 /* Number of non-dirextory arguments */
char **DIRS;   		 /* String array of directory arguments */
char **NONDIRS; 	 /* String array of non-directory arguments */


/*
 * Reads flags and calls corresponding print functions 
*/
void
formatentry(int directentry, char* entry, char* path, 
	struct stat sb, paddings paddings)
{
	int printallflags = A_allentries || a_allentries;
	
	if (directentry == 0 && d_directories == 0 && 
		printallflags == 0 && entry[0] == '.') {
		return;
	}
	
	if (F_specialsymbols == 1) {
		addsymbols_F(&entry, sb);
	}
	if (i_inodes == 1) {
		printinode_i(sb, paddings.inode);
	} 
	if (s_systemblocks == 1) {
		printblocks_s(sb.st_blocks, sb.st_size,  1, paddings.block);
	}
	if (n_numericalids == 1 || l_longformat == 1) {
		printlong_l(entry, path, sb, paddings);
	}  else if (q_forcenonprintable == 1) {
		printraw_q(entry);
	} else {
		printdefault(entry);
	} 
}

/*
 * Reads contents of directory using set sort flags and sends directory 
 * entries to formatentry
*/
void
formatdir(char* dir) 
{
	char *dirptr[2];
	int blocksum, currentlevel, fileinfo;
	int ftsopenflags, ignoredflag, outputflag, printallflags;
	static const paddings resetpaddings;
	paddings paddings;
	FTSENT *blockchildren, *entry, *immediatechildren;
	FTS *directory;

	
	int (*comparefunction)(const FTSENT **, const FTSENT **);
	
	paddings = resetpaddings;

	comparefunction = &fts_lexicosort;
	
	if (r_reverseorder == 1) {
		comparefunction = &fts_rlexicosort;
	}
	
	if (S_sizesorted == 1) {
		comparefunction = &fts_sizesort;
		if (r_reverseorder == 1) {
			comparefunction = &fts_rsizesort;
		}
	}

	if (t_modifiedsorted == 1) {
		comparefunction = &fts_timemodifiedsort;
		if (r_reverseorder == 1) {
			comparefunction = &fts_rtimemodifiedsort;
		}
	}
	
	if (t_modifiedsorted == 1 && u_lastaccess == 1) {
		comparefunction = &fts_lastaccesssort;
		if (r_reverseorder == 1) {
			comparefunction = &fts_rlastaccesssort;
		}
	}
	if (t_modifiedsorted == 1 && c_lastchanged == 1) {
		comparefunction = &fts_statuschangesort;
		if (r_reverseorder == 1) { 
			comparefunction = &fts_rstatuschangesort;
		}
	}
	if (f_unsorted == 1) {
		comparefunction = NULL;	
	}
	
	ftsopenflags = FTS_COMFOLLOW;
	if (a_allentries == 1) {
		ftsopenflags |= FTS_SEEDOT;
	}
	
	if ((dirptr[0] = malloc(strlen(dir) + 1)) == NULL) {
		(void)fprintf(stderr, "Could not allocate memory.\n");
		exit(EXIT_FAILURE);
	}	
	dirptr[0] = dir;
	dirptr[1] = NULL;
	
	if ((directory = fts_open(dirptr, ftsopenflags, comparefunction)) == NULL) {
		(void)fprintf(stderr, "Cannot open %s.\n", dir);
		EXIT_STATUS = EXIT_FAILURE;
		return;
	}
	
	outputflag = 0;
	printallflags = A_allentries || a_allentries;

	while((entry = fts_read(directory)) != NULL) {
		fileinfo = entry->fts_info;
		currentlevel = entry->fts_level;
		blocksum = 0;
		ignoredflag = 0;
	
		/* Only gets children of pre-order visits  */	
		if (fileinfo != FTS_D) {
			continue;
		}
		
		/* Prevents recursing into subdirectories*/
		if (R_recurse == 0) {
			if (fts_set(directory, entry, FTS_SKIP) != 0) {
				(void)fprintf(stderr, "Could not set fts options.\n");
				EXIT_STATUS = 1;
				continue;
			}
		}
	
		/* Ignore hidden directories by default  */	
		if (printallflags == 0 && strncmp(entry->fts_name, ".", 1) == 0 && 
			currentlevel != 0) { 
			ignoredflag = 1;
		}
		
		if (ignoredflag == 0 && outputflag == 1) {
			printf("\n%s:\n", entry->fts_path);
		}
		paddings = resetpaddings;
		outputflag = 1;
	
		/* Counts and prints blocks if relevant flags are set  */	
		if (ignoredflag == 0 && 
			(l_longformat == 1|| n_numericalids == 1 || s_systemblocks == 1 || isatty(1) == 0)) {
			if ((blockchildren = fts_children(directory, 0)) != NULL) {
				blocksum = countblocks(blockchildren, &paddings); 
				printblocks_s(blocksum, blocksum,  0, paddings.block);
			}
		}
		/* If current entry isn't ignored, get all of its children  */
		if (ignoredflag == 0 && (immediatechildren = fts_children(directory, 0)) != NULL) {	
			getimmediatechildren(immediatechildren, currentlevel, &paddings); 
		}
	}	
	
	fts_close(directory);
	free(dirptr[0]);
	free(dirptr[1]);
}

/*
 * Frees memory from global data structures
*/
void
cleanup() {
	int i;
	for (i = 0; i < NUMDIRS; i++) {
		free(DIRS[i]);
	}
	free(DIRS);
	for (i = 0; i < NUMNONDIRS; i++) {
		free(NONDIRS[i]);
	}
	free(NONDIRS);
}

/*
 * Program takes in a set of arguments and optionally a directory. If no directory is
 * provided, the program defaults to the current shell's directory. The program prints the
 * contents of the directory based on the arguments given. 
*/
int
main(int argc, char **argv) 
{
	int i, argoffset, outputflag;
	struct stat sb;
	static const paddings resetpaddings;
	paddings paddings;
	
	setprogname(argv[0]);
 	
	EXIT_STATUS = 0;	
	NUMDIRS = 0;
	NUMNONDIRS = 0;
	argoffset = 0;
	
	if (parseargs(argc, argv) == 1) {
		argoffset = 1;
	}
	 
 	/* Counts the number of nondirs and dirs arguments given */		
	for (i = 1 + argoffset; i < argc; i++){
		if (lstat(argv[i], &sb) == 0) {
			if (S_ISDIR(sb.st_mode) && d_directories == 0) {
				NUMDIRS++;
			} else {
				NUMNONDIRS++;
			}
		} else {
			(void)fprintf(stderr, "%s: %s: No such file or directory\n"
				, getprogname(), argv[i]);
			EXIT_STATUS = EXIT_FAILURE;
			if (i == argc - 1 && i == 1 + argoffset) {
				exit(EXIT_STATUS);
			}
		}
	}
	
	if (NUMDIRS > 0 &&  (DIRS = malloc(NUMDIRS * sizeof(char*))) == NULL) {
		(void)fprintf(stderr, "Memory could not be allocated for DIRS");
		exit(EXIT_FAILURE);
	}


	if (NUMNONDIRS > 0 && (NONDIRS = malloc(NUMNONDIRS * sizeof(char*))) == NULL) {
		(void)fprintf(stderr, "Memory could not be allocated NONDIRS");
		exit(EXIT_FAILURE);
	}

	splitargs(argc, argv, argoffset);
	
	qsort(NONDIRS, NUMNONDIRS, sizeof(char*), lexicosort);	
	qsort(DIRS, NUMDIRS, sizeof(char*), lexicosort);


	if (l_longformat  == 1|| n_numericalids == 1) {
		paddings = resetpaddings;
		for (i = 0; i < NUMNONDIRS; i++) {
			if (lstat(NONDIRS[i], &sb) == 0) {
				getpaddingsizes(sb, &paddings);	
			}
		}					
	}
	
	outputflag = 0;
	/* Formats non directory entries and then directory entries */	
	for (i = 0; i < NUMNONDIRS; i++) {
		if (lstat(NONDIRS[i], &sb) != 0) {
			(void)fprintf(stderr, "%s: cannot access %s: No such file or directory.\n"
				, getprogname(), NONDIRS[i]);
			continue;
		}
		formatentry(1, NONDIRS[i], dirname(NONDIRS[i]), sb, paddings);
		outputflag = 1;
	}

	for (i = 0; i < NUMDIRS; i++) {
		
		if (stat(DIRS[i], &sb) != 0) {
			(void)fprintf(stderr, "%s: cannot access %s: No such file or directory.\n"
				, getprogname(), DIRS[i]);
			continue;
		}
		if (NUMDIRS + NUMNONDIRS > 1) {
			if (outputflag == 1) {
				(void)printf("\n");
			}
			(void)printf("%s:\n", DIRS[i]);
		}
			formatdir(DIRS[i]); 
			outputflag = 1;
	}

	cleanup(DIRS, NONDIRS);
	exit(EXIT_STATUS);
}
