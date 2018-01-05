/******************************************************************************
*	Copyright (c) 2017, Mack Informatcion Systems, In.  All rights reserved.
*
*	Application used to compute spline for calibration.
*
*	Author: Dan Rockwell
*/

#if !defined(SPLINE_H_)
#define SPLINE_H_


#include <stdlib.h>


typedef struct
{
    size_t              numPts;
    float               *x;
    float               *y;
    float               *b; 
    float               *c;
    float               *d;
} Spline_t;


extern void  Spline_Create( Spline_t *ioSpline, size_t inSize );
extern void  Spline_Setup( Spline_t *ioSpline, size_t inSize, float *inX, float *inY );
extern float Spline_Calc( Spline_t *ioSpline, float inVal );

#endif // SPLINE_H_


