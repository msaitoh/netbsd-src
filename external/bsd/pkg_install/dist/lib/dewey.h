/* $NetBSD: dewey.h,v 1.3 2021/04/10 19:49:59 nia Exp $ */

#ifndef _INST_LIB_DEWEY_H_
#define _INST_LIB_DEWEY_H_

int dewey_cmp(const char *, int, const char *);
int dewey_match(const char *, const char *);
int dewey_mktest(int *, const char *);

enum {
	DEWEY_LT,
	DEWEY_LE,
	DEWEY_EQ,
	DEWEY_GE,
	DEWEY_GT,
	DEWEY_NE
};

#endif				/* _INST_LIB_DEWEY_H_ */
