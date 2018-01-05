/******************************************************************************
*	Copyright (c) 2017, Mack Informatcion Systems, In.  All rights reserved.
*
*	Application used to compute spline for calibration.
*
*	Author: Dan Rockwell
*/

#include <string.h>
#include "spline.h"


/**
 *  Create a spline
 */
void Spline_Create( Spline_t *ioSpline, size_t inSize )
{
    float   *ptr;
    ptr = calloc( inSize * 5, sizeof(float) );
    ioSpline->numPts = inSize;
    ioSpline->x = ptr + inSize * 0;
    ioSpline->y = ptr + inSize * 1;
    ioSpline->b = ptr + inSize * 2;
    ioSpline->c = ptr + inSize * 3;
    ioSpline->d = ptr + inSize * 4;
}


/**
 *  Setup a spline
 */
void Spline_Setup( Spline_t *ioSpline, size_t inSize, float *inX, float *inY )
{
    int     i, j, n;
    float   temp_real;
    
    // Copy over the new data
    ioSpline->numPts = inSize;
    memcpy( ioSpline->x, inX, inSize * sizeof(float) );
    memcpy( ioSpline->y, inY, inSize * sizeof(float) );

    // Setup for zero based arrays
    n = ioSpline->numPts - 1;

    // Sort the spline points on the X coordinate
    // (Using insertion sort here, so I assume we won't have many pts.)
    for( i = 0; i < n; ++i )
    {
        float x, y;
        x = ioSpline->x[i];
        y = ioSpline->y[i];
        for( j = i - 1; j >= 0 && ioSpline->x[j] > x; --j )
        {
            ioSpline->x[j + 1] = ioSpline->x[j];
            ioSpline->y[j + 1] = ioSpline->y[j];
        }
        ioSpline->x[j + 1] = x;
        ioSpline->y[j + 1] = y;
    }

    // For single point offset, apply delta across the range
    if( 1 == ioSpline->numPts )
    {
    	ioSpline->b[0] = 0.0f;
        ioSpline->c[0] = 0.0f;
        ioSpline->d[0] = 0.0f;
        ioSpline->b[1] = 0.0f;
        ioSpline->c[1] = 0.0f;
        ioSpline->d[1] = 0.0f;
    }

    // For 2 points set up a linear equation
    else if( 2 == ioSpline->numPts )
    {
		ioSpline->b[0] = 0.0f;
		if ((ioSpline->x[1] - ioSpline->x[0]) != 0.0)
		{
			ioSpline->b[0] = (ioSpline->y[1] - ioSpline->y[0]) / (ioSpline->x[1] - ioSpline->x[0]);
		}

		ioSpline->c[0] = 0.0f;
        ioSpline->d[0] = 0.0f;
        ioSpline->b[1] = ioSpline->b[0];
        ioSpline->c[1] = 0.0f;
        ioSpline->d[1] = 0.0f;
    }

    // 3 or more points
    else if( 2 < ioSpline->numPts )
    {
        // Set up for tridiagonal system where ioSpline->b = diagonal,
        //	ioSpline->d = off diagnonal, and ioSpline->c = right hand side.
        ioSpline->d[0] = ioSpline->x[1] - ioSpline->x[0];
		ioSpline->c[1] = 0.0;
		if (ioSpline->d[0] != 0.0)
		{
            ioSpline->c[1] = (ioSpline->y[1] - ioSpline->y[0])/ioSpline->d[0];
		}

        for( i = 1; i < n; ++i )
        {
			ioSpline->c[i + 1] = 0.0;
			ioSpline->d[i] = ioSpline->x[i + 1] - ioSpline->x[i];
            ioSpline->b[i] = 2.0f * (ioSpline->d[i-1] + ioSpline->d[i]);
			if( ioSpline->d[i] != 0.0 )
			{
				ioSpline->c[i + 1] = (ioSpline->y[i + 1] - ioSpline->y[i]) / (ioSpline->d[i]);
			}
            ioSpline->c[i] = ioSpline->c[i+1] - ioSpline->c[i];
        }

        // End Conditions -- third derivatives of ioSpline->x(0) and
        //	ioSpline->x(n) obtained from divided difference.
        ioSpline->b[0] = -ioSpline->d[0];
        ioSpline->b[n] = -ioSpline->d[n-1];
        ioSpline->c[0] = 0.0f;
        ioSpline->c[n] = 0.0f;

        if( 3 < ioSpline->numPts )
        {
			if (((ioSpline->x[3] - ioSpline->x[1]) != 0.0) && ((ioSpline->x[2] - ioSpline->x[0]) != 0.0))
			{
				ioSpline->c[0] = ioSpline->c[2] / (ioSpline->x[3] - ioSpline->x[1])
							   - ioSpline->c[1] / (ioSpline->x[2] - ioSpline->x[0]);
			}
			else
			{
				ioSpline->c[0] = 0.0;
			}

			if (((ioSpline->x[n] - ioSpline->x[n - 2]) != 0.0) && ((ioSpline->x[n - 1] - ioSpline->x[n - 3]) != 0.0))
			{
				ioSpline->c[n] = ioSpline->c[n - 1] / (ioSpline->x[n] - ioSpline->x[n - 2])
							   - ioSpline->c[n - 2] / (ioSpline->x[n - 1] - ioSpline->x[n - 3]);
			}
			else
			{
				ioSpline->c[n] = 0.0;
			}

			if ((ioSpline->x[3] - ioSpline->x[0]) != 0.0)
			{
				ioSpline->c[0] = (ioSpline->c[0] * ioSpline->d[0] * ioSpline->d[0])
							   / (ioSpline->x[3] - ioSpline->x[0]);
			}
			else
			{
				ioSpline->c[0] = 0.0;
			}

			if ((ioSpline->x[n] - ioSpline->x[n - 3]) != 0.0)
			{
				ioSpline->c[n] = (-ioSpline->c[n] * ioSpline->d[n - 1] * ioSpline->d[n - 1])
							   / (ioSpline->x[n] - ioSpline->x[n - 3]);
			}
			else
			{
				ioSpline->c[n] = 0.0;
			}
        }

        // Forward elimination
        for( i = 1; i <= n; i++ )
        {
			temp_real = 0.0; 
			if( ioSpline->b[i - 1] != 0.0 )
			{
				temp_real = ioSpline->d[i - 1] / ioSpline->b[i - 1];
			}

			ioSpline->b[i] = ioSpline->b[i] - temp_real * ioSpline->d[i-1];
            ioSpline->c[i] = ioSpline->c[i] - temp_real * ioSpline->c[i-1];
        }

        // Back substitution
		ioSpline->c[n] = 0.0;
		if( ioSpline->b[n] != 0.0 )
		{
			ioSpline->c[n] = ioSpline->c[n] / ioSpline->b[n];
		}

        for (i = (short int)(n - 1); i >= 0; i--)
        {
			ioSpline->c[i] = 0.0;
			if( ioSpline->b[i] != 0.0 )
			{
				ioSpline->c[i] = (ioSpline->c[i] - ioSpline->d[i] * ioSpline->c[i + 1]) / ioSpline->b[i];
			}
        }

        // Compute coefficents
		ioSpline->b[n] = 0.0;
		if (ioSpline->d[n - 1] != 0.0)
		{
			ioSpline->b[n] = (ioSpline->y[n] - ioSpline->y[n - 1]) / ioSpline->d[n - 1]
						   + ioSpline->d[n - 1] * (ioSpline->c[n - 1] + 2.0f * ioSpline->c[n]);
		}

        for( i = 0; i < n; i++ )
        {
            if (ioSpline->d[i] != 0.0)
            {
                ioSpline->b[i] = (ioSpline->y[i+1] - ioSpline->y[i]) / ioSpline->d[i]
                               - ioSpline->d[i] * (ioSpline->c[i+1] + 2.0f * ioSpline->c[i]);
                ioSpline->d[i] = (ioSpline->c[i+1] - ioSpline->c[i]) / ioSpline->d[i];
            }
            else
            {
                ioSpline->b[i] = 0.0;
                ioSpline->d[i] = 0.0;
            }
            ioSpline->c[i] = 3.0f * ioSpline->c[i];
        }

        ioSpline->c[n] = 3.0f * ioSpline->c[n];
        ioSpline->d[n] = ioSpline->d[n-1];
    }
}


/**
 *  Calculate a spline
 */
float Spline_Calc( Spline_t *inSpline, float inVal )
{
    float       dx;         // X-axis interpolation distance
    unsigned    i,j,k,n;
    float       retVal;
    
    n = inSpline->numPts - 1;

    if( 1 == inSpline->numPts )
	{
        retVal = inSpline->y[0] ;
	}
    else
    {
        // Binary search
        i = 0;
        j = n + 1;
        while( 1 )
        {
            k = (i + j) / 2;

			if (inVal < inSpline->x[k])
			{
				j = k;
			}
			else
			{
				i = k;
			}
			if (j == (i + 1))
			{
				break;
			}
        }

        dx = inVal - inSpline->x[i];
        retVal = (inSpline->y[i]) + dx * (inSpline->b[i] + dx * (inSpline->c[i] + dx * inSpline->d[i]));
    }
    
    return( retVal );
}


