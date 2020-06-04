#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#define bug printf

#ifdef DEBUG
#define D(x) (x);
#else
#define D(x) ;
#endif

#endif
