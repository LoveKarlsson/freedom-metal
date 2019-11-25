/* Copyright 2019 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#ifndef METAL__TIME_H
#define METAL__TIME_H

#include <time.h>

#ifdef __ICCRISCV__
/* This definitions are not part of standard C and
   are defined here only to make time.c compilable */
#define	_TIMER_T_	unsigned long
typedef	_TIMER_T_	__timer_t;
typedef	long		__suseconds_t;	/* microseconds (signed) */
typedef	unsigned long	__useconds_t;	/* microseconds (unsigned) */

#ifndef _SUSECONDS_T_DECLARED
typedef	__suseconds_t	suseconds_t;
#define	_SUSECONDS_T_DECLARED
#endif

struct timeval {
	time_t		tv_sec;		/* seconds */
	suseconds_t	tv_usec;	/* and microseconds */
};
#endif

/*!
 * @file time.h
 * @brief API for dealing with time
 */

int metal_gettimeofday(struct timeval *tp, void *tzp);

time_t metal_time(void);

#endif
