#ifndef _MATH_OPT_H
#define _MATH_OPT_H

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef TI_DSP
#include <ti/mathlib/mathlib.h>
#endif


#ifdef TI_DSP

	#define my_log10f log10sp
	#define my_powf	powsp 
	#define my_expf expsp
	#define my_fabs  fabs
#else
	#define my_log10f  log10f
	#define my_powf   powf
	#define my_expf  expf
	#define my_fabs   fabs
#endif
	#define my_min(x, y)  (x) < (y) ? (x):(y)
	#define my_max(x,y)  (x) > (y) ? (x):(y)
#endif

