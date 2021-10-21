/*
 * 10/15/2021
*/
#ifndef _CMP_H_
#define _CMP_H_

int lexicosort(const void*, const void*);

int fts_lexicosort(const FTSENT** file1, const FTSENT** file2);
int fts_rlexicosort(const FTSENT** file1, const FTSENT** file2);

int fts_sizesort(const FTSENT** file1, const FTSENT** file2);
int fts_rsizesort(const FTSENT** file1, const FTSENT** file2);

int fts_timemodifiedsort(const FTSENT** file1, const FTSENT** file2);
int fts_rtimemodifiedsort(const FTSENT** file1, const FTSENT** file2);

int fts_lastaccesssort(const FTSENT** file1, const FTSENT** file2);
int fts_rlastaccesssort(const FTSENT** file1, const FTSENT** file2);

int fts_statuschangesort(const FTSENT** file1, const FTSENT** file2);
int fts_rstatuschangesort(const FTSENT** file1, const FTSENT** file2);

#endif /* !_CMP_H_  */
