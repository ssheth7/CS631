/*
 * 10/15/2021
*/

#ifndef _PRINT_H_
#define _PRINT_H_

void addsymbols_F(char**, struct stat);
void printdefault(char*);
void printall(char*);
void printraw_q(char*);
void printinode_i(struct stat, int);
void printblocks_s(int, int, int, int);
void printlong_l(char*, char*, struct stat, struct paddings);

#endif /* !_PRINT_H_ */
