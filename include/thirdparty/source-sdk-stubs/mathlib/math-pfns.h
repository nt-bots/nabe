#ifndef NABENABE_MATH_PFNS_H_
#define NABENABE_MATH_PFNS_H_

// The code in this file is based on the Source 1 SDK, and is used under the SOURCE 1 SDK LICENSE.
// https://github.com/ValveSoftware/source-sdk-2013

/*
				SOURCE 1 SDK LICENSE

Source SDK Copyright(c) Valve Corp.

THIS DOCUMENT DESCRIBES A CONTRACT BETWEEN YOU AND VALVE
CORPORATION ("Valve").  PLEASE READ IT BEFORE DOWNLOADING OR USING
THE SOURCE ENGINE SDK ("SDK"). BY DOWNLOADING AND/OR USING THE
SOURCE ENGINE SDK YOU ACCEPT THIS LICENSE. IF YOU DO NOT AGREE TO
THE TERMS OF THIS LICENSE PLEASE DON'T DOWNLOAD OR USE THE SDK.

  You may, free of charge, download and use the SDK to develop a modified Valve game
running on the Source engine.  You may distribute your modified Valve game in source and
object code form, but only for free. Terms of use for Valve games are found in the Steam
Subscriber Agreement located here: http://store.steampowered.com/subscriber_agreement/

  You may copy, modify, and distribute the SDK and any modifications you make to the
SDK in source and object code form, but only for free.  Any distribution of this SDK must
include this LICENSE file and thirdpartylegalnotices.txt.

  Any distribution of the SDK or a substantial portion of the SDK must include the above
copyright notice and the following:

	DISCLAIMER OF WARRANTIES.  THE SOURCE SDK AND ANY
	OTHER MATERIAL DOWNLOADED BY LICENSEE IS PROVIDED
	"AS IS".  VALVE AND ITS SUPPLIERS DISCLAIM ALL
	WARRANTIES WITH RESPECT TO THE SDK, EITHER EXPRESS
	OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED
	WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT,
	TITLE AND FITNESS FOR A PARTICULAR PURPOSE.

	LIMITATION OF LIABILITY.  IN NO EVENT SHALL VALVE OR
	ITS SUPPLIERS BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
	INDIRECT, OR CONSEQUENTIAL DAMAGES WHATSOEVER
	(INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF
	BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF
	BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
	ARISING OUT OF THE USE OF OR INABILITY TO USE THE
	ENGINE AND/OR THE SDK, EVEN IF VALVE HAS BEEN
	ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.


If you would like to use the SDK for a commercial purpose, please contact Valve at
sourceengine@valvesoftware.com.
*/

//========= Copyright (C) 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#if(1)
#include <immintrin.h>

inline void SSESqrt(float* pOut, float* pIn)
{
	_mm_store_ss(pOut, _mm_sqrt_ss(_mm_load_ss(pIn)));
}

inline float fast_sqr(float x)
{
	float res;
	SSESqrt(&res, &x);
	return res;
}

#else

#ifndef _MATH_PFNS_H_
#define _MATH_PFNS_H_

#if defined( _X360 )
#include <xboxmath.h>
#else
#include <math.h>
#include <cmath>
//#include <ctgmath>
#include <immintrin.h>
#endif

#if !defined( _X360 )

// The following are not declared as macros because they are often used in limiting situations,
// and sometimes the compiler simply refuses to inline them for some reason
#define FastSqrt(x)			my_sse_sqrt(x)
#define	FastRSqrt(x)		my_sse_rsqrt(x)
#define FastRSqrtFast(x)    my_sse_rsqrt(x)
#define FastSinCos(x,s,c)   my_sse_sincos(x,s,c)
#define FastCos(x)			my_sse_cos(x)

float my_sse_sqrt(const float& x)
{
	float res;
	_mm_store_ps(&res, _mm_sqrt_ps(_mm_load_ps(&x)));
	return res;
}

float my_sse_rsqrt(const float& x)
{
	float res;
	_mm_store_ps(&res, _mm_rsqrt_ps(_mm_load_ps(&x)));
	return res;
}

void my_sse_sincos(const float& x, float& sin_out, float& cos_out)
{
	auto sse_x = _mm_load_ps(&x);
	_mm_store_ps(&sin_out, _mm_sin_ps(sse_x));
	_mm_store_ps(&cos_out, _mm_cos_ps(sse_x));
}

float my_sse_cos(const float& x)
{
	float res;
	_mm_store_ps(&res, _mm_cos_ps(_mm_load_ps(&x)));
	return res;
}

#endif // !_X360

#if defined( _X360 )

FORCEINLINE float _VMX_Sqrt( float x )
{
	return __fsqrts( x );
}

FORCEINLINE float _VMX_RSqrt( float x )
{
	float rroot = __frsqrte( x );

	// Single iteration NewtonRaphson on reciprocal square root estimate
	return (0.5f * rroot) * (3.0f - (x * rroot) * rroot);
}

FORCEINLINE float _VMX_RSqrtFast( float x )
{
	return __frsqrte( x );
}

FORCEINLINE void _VMX_SinCos( float a, float *pS, float *pC )
{
	XMScalarSinCos( pS, pC, a );
}

FORCEINLINE float _VMX_Cos( float a )
{
	return XMScalarCos( a );
}

// the 360 has fixed hw and calls directly
#define FastSqrt(x)			_VMX_Sqrt(x)
#define	FastRSqrt(x)		_VMX_RSqrt(x)
#define FastRSqrtFast(x)	_VMX_RSqrtFast(x)
#define FastSinCos(x,s,c)	_VMX_SinCos(x,s,c)
#define FastCos(x)			_VMX_Cos(x)

#endif // _X360

#endif // _MATH_PFNS_H_

#endif

#endif // NABENABE_MATH_PFNS_H_
