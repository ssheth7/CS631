/*
* Shivam Sheth (ssheth7)
* CS 631 hw2: bare-bones copy a file.
* 09/19/2021
*/

#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXPATHSIZE	256
#define BUFSIZE		32000

void readfifo(char[BUFSIZE], char *, char*, struct stat);
int main(int, char*[]);

/*
 * This function does a blocking read if the source file provided to the program 
 * is a fifo and the destination is a regular file. The fifo will be continuously 
 * read until the program is interrupted.
 */
void readfifo(char buf[BUFSIZE], char* source, char* dest, struct stat destsb)
{
	int bytesread; /* bytes read from fifo */
	FILE *destfile, *sourcefile; /* source/destination file streams  */
	//char buf[BUFSIZE]; /* fifo contents buffer */

	if (stat(dest, &destsb) == -1) {
		fprintf(stderr, "Could not stat destination file: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (!S_ISREG(destsb.st_mode)) {
		fprintf(stderr, "Cannot copy fifo into non regular file.\n");
		exit(EXIT_FAILURE);
	}
	
	if ((sourcefile = fopen(source, "rb")) ==  NULL) {
		fprintf(stderr, "Could not get source file descriptor: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	if ((destfile = fopen(dest, "wb")) == NULL) {
		perror("Could not open dest file.\n");
		exit(EXIT_FAILURE);
	}
	
	while (1){
		sleep(1);
		if ((bytesread = fread(buf, 1, BUFSIZE, sourcefile)) > 0) {
			if (fwrite(buf, 1, BUFSIZE, destfile) == 0) {
				fprintf(stderr, "Could not write to file: %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}		
		}
	}

	(void)fclose(sourcefile);
	(void)fclose(destfile);
	exit(EXIT_SUCCESS);
}

/*
 * Given a source file and a destination file or directory, this program will
 * copy the contents of the source file into the destination. 
 */
int 
main(int argc, char **argv) 
{
	int bytesread; /* bytes read from source */
	int destfd, sourcefd; /* source/destination file descriptors */
	int len; /* snprintf return value  */
	struct stat destsb, sourcesb; /* source/destination stat buffer */
	char destinationfile[MAXPATHSIZE], sourcefile[MAXPATHSIZE]; /* source/destination file path */
	char buf[BUFSIZE]; /* file content buffer */
	
	setprogname(argv[0]);
	
	if (argc != 3) {
		fprintf(stderr, "Incorrect number of arguments, expected 2.\n");
		exit(EXIT_FAILURE);
	}

	if (stat(argv[1], &sourcesb) == -1) {
		fprintf(stderr, "Could not stat source file: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	} 
	
	if (S_ISFIFO(sourcesb.st_mode)) {
		readfifo(buf, argv[1], argv[2], destsb);
	}	
	
	if (!S_ISREG(sourcesb.st_mode)) {
		fprintf(stderr, "Source file is not a regular file.\n");
		exit(EXIT_FAILURE);
	}
	
	if ((sourcefd = open(argv[1], O_RDONLY)) < 0){
		fprintf(stderr, "Could not get source file descriptor: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	/* Build source/destination file path to ensure they differ */
	strcpy(destinationfile, argv[2]);
	if (stat(argv[2], &destsb) == 0) {
		
		if (S_ISDIR(destsb.st_mode)) {
			
			len = snprintf(destinationfile, MAXPATHSIZE,"%s/%s", argv[2], basename(argv[1]));
			if (len < 0 || len >= MAXPATHSIZE) {
				fprintf(stderr, "Destination path size reached MAXPATHSIZE: %d\n", MAXPATHSIZE);
			}

			len = snprintf(sourcefile, MAXPATHSIZE, "%s/%s", dirname(argv[1]), basename(argv[1]));
			if (len < 0 || len >= MAXPATHSIZE) {
				fprintf(stderr, "Source path size reached MAXPATHSIZE: %d\n", MAXPATHSIZE);
			}

		} else if (!S_ISREG(destsb.st_mode)){
			fprintf(stderr, "Destination is not a file or directory.\n");
			exit(EXIT_FAILURE);
		}
	} 

	if (strcmp(sourcefile, destinationfile) == 0) {
		fprintf(stderr, "Source and destination file are the same.\n");
		exit(EXIT_FAILURE);
	}

	if (sourcesb.st_ino == destsb.st_ino) {
		fprintf(stderr, "Source and destination file have the same inode.\n");
		exit(EXIT_FAILURE);
	}

	if ((destfd = open(destinationfile, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) {	
		fprintf(stderr, "Could not open dest file: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	while ((bytesread = read(sourcefd, buf, BUFSIZE - 1)) != -1 && bytesread != 0) {	
		if (write(destfd, buf, bytesread) < 0) {
			fprintf(stderr, "Could not write to file: %s\n.", strerror(errno));
			exit(EXIT_FAILURE);
		}		
	}
	
	(void)close(sourcefd);
	(void)close(destfd);
	exit(EXIT_SUCCESS);
}
