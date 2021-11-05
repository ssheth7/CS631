/*
 * "Advanced Programming in the UNIX Environment" homework assignment 2
 *  Example of the command function, which passes a command to the shell
 *  10/27/20 
*/

/*
 * Usage:
 * cc -Werror -Wextra -Wall -DCOMM='"ls -la"' ssheth-command.c
 * ./a.out 
*/

#include <sys/wait.h>

#include <err.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int command(const char *, char*, int, char*, int);
int main(int, char*[]);

#ifndef COMM
#define COMM "ls -l"
#endif

/*
 * Provided a command and an err buffer and an out buffer, 
 * splits the output into both buffers up to the given length
*/
int
command(const char* comm, char* outbuf, int outlen, char *errbuf, int errlen)
{
	sigset_t nmask, omask;
	char *args[4] = {"sh", "-c", NULL, NULL};
	int commlen, readlen, status, errpipe[2], outpipe[2];
	pid_t pid;
	
	/* Checking for null pointer and shell interpreter 
	 * availability based on system(3) source 
	 */	
	if (comm == NULL) {
		if (access("/bin/sh", X_OK) == 0) {
			return EXIT_FAILURE;	
		}
		return EXIT_SUCCESS;	
	}
	
	commlen = strlen(comm);
	if ((args[2] = malloc(commlen + 1)) == NULL) {
		err(EXIT_FAILURE, "malloc");
		/* NOTREACHED  */
	}
	(void)strncpy(args[2], comm, commlen);
	args[2][commlen] = '\0';

	if (pipe2(outpipe, O_NONBLOCK) < 0) {
		err(EXIT_FAILURE, "pipe");
		/* NOTREACHED  */
	}
	if (pipe2(errpipe, O_NONBLOCK) < 0) {
		err(EXIT_FAILURE, "pipe");
		/* NOTREACHED  */
	}
	if (signal(SIGQUIT, SIG_IGN) == SIG_ERR) {
		err(EXIT_FAILURE, "signal");
		/* NOTREACHED  */
	}
	if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
		err(EXIT_FAILURE, "signal");
		/* NOTREACHED  */
	}
	if (sigemptyset(&nmask) < 0) {
		err(EXIT_FAILURE, "sigemptyset");
		/* NOTREACHED  */
	}
	if (sigaddset(&nmask, SIGCHLD) < 0) {
		err(EXIT_FAILURE, "sigaddset");
		/* NOTREACHED  */
	}

	if (sigprocmask(SIG_BLOCK, &nmask, &omask) < 0) {
		err(EXIT_FAILURE, "sigprocmask");
	}
	
	if ((pid = fork()) < 0) {
		err(EXIT_FAILURE, "fork");
		/* NOTREACHED  */
	}
	
	/* Parent process reads from pipe  */
	if (pid > 0) {
		(void)close(outpipe[1]);
		(void)close(errpipe[1]);
		
		if (waitpid(pid, &status, 0) < 0) {
			err(EXIT_FAILURE, "waitpid");
			/* NOTREACHED  */
		}
		if ((readlen = read(outpipe[0], outbuf, outlen)) == -1 ) {
			err(EXIT_FAILURE, "read");
			/* NOTREACHED  */
		}
		if (readlen < outlen) {
			outbuf[readlen] = '\0';
		}

		if ((readlen = read(errpipe[0], errbuf, errlen)) == -1 ) {
			err(EXIT_FAILURE, "read");
			/* NOTREACHED  */
		}
		if (readlen < errlen) {
			errbuf[readlen] = '\0';
		}
		(void)close(outpipe[0]);
		(void)close(errpipe[0]);

	/* Child process execs command and writes to pipe  */
	} else {
		(void)close(outpipe[0]);
		(void)close(errpipe[0]);
		if (dup2(outpipe[1], STDOUT_FILENO) == -1) {
			err(EXIT_FAILURE, "dup2");
			/* NOTREACHED  */
	
		}
		if (dup2(errpipe[1], STDERR_FILENO) == -1) {
			err(EXIT_FAILURE, "dup2");
			/* NOTREACHED  */
		}
		if (execvp("sh", args) == -1) {
			err(EXIT_FAILURE, "execvp");
			/* NOTREACHED  */
		}
		(void)close(errpipe[1]);
		(void)close(outpipe[1]);
		_exit(127);	
	}
	if (sigprocmask(SIG_SETMASK, &nmask, NULL) < 0) {
		err(EXIT_FAILURE, "sigprocmask");
	}
	free(args[2]);
	return status;
}

/*
 * This program exemplifies the usage of the command() function, which 
 * takes a command and passes it to a shell and returns stdout and stderr
*/
int 
main(int argc, char **argv)
{
	int err; 
	char outbuf[BUFSIZ], errbuf[BUFSIZ];
	
	(void)argc;
	(void)argv;
	
	err = command(COMM, outbuf, BUFSIZ - 1, errbuf, BUFSIZ - 1);
	if (err == -1) {
		perror("command");
		exit(EXIT_FAILURE);
	}
	outbuf[BUFSIZ - 1] = '\0';
	errbuf[BUFSIZ - 1] = '\0';
	printf("stdout:\n%s\n", outbuf);
	printf("stderr:\n%s\n", errbuf);
	return EXIT_SUCCESS;
}

