/*
 * 10/14/2021
*/
#ifndef _LS_H_
#define _LS_H_

#include <sys/stat.h>

#include "helpers.h"

void formatentry(int, char*, char*, struct stat, struct paddings);
void formatdir(char*);
void cleanup();
int main(int, char*[]);

#endif /* !_LS_H_  */
