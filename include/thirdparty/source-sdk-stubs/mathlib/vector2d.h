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
// $NoKeywords: $
//
//=============================================================================//

#ifndef VECTOR2D_H
#define VECTOR2D_H

#ifdef _WIN32
#pragma once
#endif

#include <math.h>
#include <float.h>

// For vec_t, put this somewhere else?
#include "thirdparty/source-sdk-stubs/mathlib/basetypes.h"

// For rand(). We really need a library!
#include <stdlib.h>

#include "thirdparty/source-sdk-stubs/mathlib/math-pfns.h"

#include "thirdparty/source-sdk-stubs/platform.h"

//=========================================================
// 2D Vector2D
//=========================================================

class Vector2D					
{
public:
	// Members
	vec_t x, y;

	// Construction/destruction
	Vector2D(void);
	Vector2D(vec_t X, vec_t Y);
	Vector2D(const float *pFloat);

	// Initialization
	void Init(vec_t ix=0.0f, vec_t iy=0.0f);

	// Got any nasty NAN's?
	bool IsValid() const;

	// array access...
	vec_t operator[](int i) const;
	vec_t& operator[](int i);

	// Base address...
	vec_t* Base();
	vec_t const* Base() const;

	// Initialization methods
	void Random( float minVal, float maxVal );

	// equality
	bool operator==(const Vector2D& v) const;
	bool operator!=(const Vector2D& v) const;	

	// arithmetic operations
	Vector2D&	operator+=(const Vector2D &v);			
	Vector2D&	operator-=(const Vector2D &v);		
	Vector2D&	operator*=(const Vector2D &v);			
	Vector2D&	operator*=(float s);
	Vector2D&	operator/=(const Vector2D &v);		
	Vector2D&	operator/=(float s);					

	// negate the Vector2D components
	void	Negate(); 

	// Get the Vector2D's magnitude.
	vec_t	Length() const;

	// Get the Vector2D's magnitude squared.
	vec_t	LengthSqr(void) const;

	// return true if this vector is (0,0) within tolerance
	bool IsZero( float tolerance = 0.01f ) const
	{
		return (x > -tolerance && x < tolerance &&
				y > -tolerance && y < tolerance);
	}

	// Normalize in place and return the old length.
	vec_t	NormalizeInPlace();

	// Compare length.
	bool	IsLengthGreaterThan( float val ) const;
	bool	IsLengthLessThan( float val ) const;

	// Get the distance from this Vector2D to the other one.
	vec_t	DistTo(const Vector2D &vOther) const;

	// Get the distance from this Vector2D to the other one squared.
	vec_t	DistToSqr(const Vector2D &vOther) const;		

	// Copy
	void	CopyToArray(float* rgfl) const;	

	// Multiply, add, and assign to this (ie: *this = a + b * scalar). This
	// is about 12% faster than the actual Vector2D equation (because it's done per-component
	// rather than per-Vector2D).
	void	MulAdd(const Vector2D& a, const Vector2D& b, float scalar);	

	// Dot product.
	vec_t	Dot(const Vector2D& vOther) const;			

	// assignment
	Vector2D& operator=(const Vector2D &vOther);

#ifndef VECTOR_NO_SLOW_OPERATIONS
	// copy constructors
	Vector2D(const Vector2D &vOther);

	// arithmetic operations
	Vector2D	operator-(void) const;
				
	Vector2D	operator+(const Vector2D& v) const;	
	Vector2D	operator-(const Vector2D& v) const;	
	Vector2D	operator*(const Vector2D& v) const;	
	Vector2D	operator/(const Vector2D& v) const;	
	Vector2D	operator*(float fl) const;
	Vector2D	operator/(float fl) const;			
	
	// Cross product between two vectors.
	Vector2D	Cross(const Vector2D &vOther) const;		

	// Returns a Vector2D with the min or max in X, Y, and Z.
	Vector2D	Min(const Vector2D &vOther) const;
	Vector2D	Max(const Vector2D &vOther) const;

#else

private:
	// No copy constructors allowed if we're in optimal mode
	Vector2D(const Vector2D& vOther);
#endif
};

//-----------------------------------------------------------------------------

const Vector2D vec2_origin(0,0);
const Vector2D vec2_invalid( FLT_MAX, FLT_MAX );

//-----------------------------------------------------------------------------
// Vector2D related operations
//-----------------------------------------------------------------------------

// Vector2D clear
void Vector2DClear( Vector2D& a );

// Copy
void Vector2DCopy( const Vector2D& src, Vector2D& dst );

// Vector2D arithmetic
void Vector2DAdd( const Vector2D& a, const Vector2D& b, Vector2D& result );
void Vector2DSubtract( const Vector2D& a, const Vector2D& b, Vector2D& result );
void Vector2DMultiply( const Vector2D& a, vec_t b, Vector2D& result );
void Vector2DMultiply( const Vector2D& a, const Vector2D& b, Vector2D& result );
void Vector2DDivide( const Vector2D& a, vec_t b, Vector2D& result );
void Vector2DDivide( const Vector2D& a, const Vector2D& b, Vector2D& result );
void Vector2DMA( const Vector2D& start, float s, const Vector2D& dir, Vector2D& result );

// Store the min or max of each of x, y, and z into the result.
void Vector2DMin( const Vector2D &a, const Vector2D &b, Vector2D &result );
void Vector2DMax( const Vector2D &a, const Vector2D &b, Vector2D &result );

#define Vector2DExpand( v ) (v).x, (v).y

// Normalization
vec_t Vector2DNormalize( Vector2D& v );

// Length
vec_t Vector2DLength( const Vector2D& v );

// Dot Product
vec_t DotProduct2D(const Vector2D& a, const Vector2D& b);

// Linearly interpolate between two vectors
void Vector2DLerp(const Vector2D& src1, const Vector2D& src2, vec_t t, Vector2D& dest );


//-----------------------------------------------------------------------------
//
// Inlined Vector2D methods
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// constructors
//-----------------------------------------------------------------------------

inline Vector2D::Vector2D(void)									
{
}

inline Vector2D::Vector2D(vec_t X, vec_t Y)						
{ 
	x = X; y = Y;
}

inline Vector2D::Vector2D(const float *pFloat)					
{
	x = pFloat[0]; y = pFloat[1];	
}


//-----------------------------------------------------------------------------
// copy constructor
//-----------------------------------------------------------------------------

inline Vector2D::Vector2D(const Vector2D &vOther)					
{ 
	x = vOther.x; y = vOther.y;
}

//-----------------------------------------------------------------------------
// initialization
//-----------------------------------------------------------------------------

inline void Vector2D::Init( vec_t ix, vec_t iy )    
{ 
	x = ix; y = iy;
}

inline void Vector2D::Random( float minVal, float maxVal )
{
	x = minVal + ((float)rand() / RAND_MAX) * (maxVal - minVal);
	y = minVal + ((float)rand() / RAND_MAX) * (maxVal - minVal);
}

inline void Vector2DClear( Vector2D& a )
{
	a.x = a.y = 0.0f;
}

//-----------------------------------------------------------------------------
// assignment
//-----------------------------------------------------------------------------

inline Vector2D& Vector2D::operator=(const Vector2D &vOther)	
{
	x=vOther.x; y=vOther.y;
	return *this; 
}

//-----------------------------------------------------------------------------
// Array access
//-----------------------------------------------------------------------------

inline vec_t& Vector2D::operator[](int i)
{
	return ((vec_t*)this)[i];
}

inline vec_t Vector2D::operator[](int i) const
{
	return ((vec_t*)this)[i];
}

//-----------------------------------------------------------------------------
// Base address...
//-----------------------------------------------------------------------------

inline vec_t* Vector2D::Base()
{
	return (vec_t*)this;
}

inline vec_t const* Vector2D::Base() const
{
	return (vec_t const*)this;
}

//-----------------------------------------------------------------------------
// IsValid?
//-----------------------------------------------------------------------------

inline bool Vector2D::IsValid() const
{
	return IsFinite(x) && IsFinite(y);
}

//-----------------------------------------------------------------------------
// comparison
//-----------------------------------------------------------------------------

inline bool Vector2D::operator==( const Vector2D& src ) const
{
	Assert( src.IsValid() && IsValid() );
	return (src.x == x) && (src.y == y);
}

inline bool Vector2D::operator!=( const Vector2D& src ) const
{
	Assert( src.IsValid() && IsValid() );
	return (src.x != x) || (src.y != y);
}


//-----------------------------------------------------------------------------
// Copy
//-----------------------------------------------------------------------------

inline void Vector2DCopy( const Vector2D& src, Vector2D& dst )
{
	Assert( src.IsValid() );
	dst.x = src.x;
	dst.y = src.y;
}

inline void	Vector2D::CopyToArray(float* rgfl) const		
{ 
	Assert( IsValid() );
	Assert( rgfl );
	rgfl[0] = x; rgfl[1] = y; 
}

//-----------------------------------------------------------------------------
// standard math operations
//-----------------------------------------------------------------------------

inline void Vector2D::Negate()
{ 
	Assert( IsValid() );
	x = -x; y = -y;
} 

inline Vector2D& Vector2D::operator+=(const Vector2D& v)	
{ 
	Assert( IsValid() && v.IsValid() );
	x+=v.x; y+=v.y;	
	return *this;
}

inline Vector2D& Vector2D::operator-=(const Vector2D& v)	
{ 
	Assert( IsValid() && v.IsValid() );
	x-=v.x; y-=v.y;	
	return *this;
}

inline Vector2D& Vector2D::operator*=(float fl)	
{
	x *= fl;
	y *= fl;
	Assert( IsValid() );
	return *this;
}

inline Vector2D& Vector2D::operator*=(const Vector2D& v)	
{ 
	x *= v.x;
	y *= v.y;
	Assert( IsValid() );
	return *this;
}

inline Vector2D& Vector2D::operator/=(float fl)	
{
	Assert( fl != 0.0f );
	float oofl = 1.0f / fl;
	x *= oofl;
	y *= oofl;
	Assert( IsValid() );
	return *this;
}

inline Vector2D& Vector2D::operator/=(const Vector2D& v)	
{ 
	Assert( v.x != 0.0f && v.y != 0.0f );
	x /= v.x;
	y /= v.y;
	Assert( IsValid() );
	return *this;
}

inline void Vector2DAdd( const Vector2D& a, const Vector2D& b, Vector2D& c )
{
	Assert( a.IsValid() && b.IsValid() );
	c.x = a.x + b.x;
	c.y = a.y + b.y;
}

inline void Vector2DSubtract( const Vector2D& a, const Vector2D& b, Vector2D& c )
{
	Assert( a.IsValid() && b.IsValid() );
	c.x = a.x - b.x;
	c.y = a.y - b.y;
}

inline void Vector2DMultiply( const Vector2D& a, vec_t b, Vector2D& c )
{
	Assert( a.IsValid() && IsFinite(b) );
	c.x = a.x * b;
	c.y = a.y * b;
}

inline void Vector2DMultiply( const Vector2D& a, const Vector2D& b, Vector2D& c )
{				  
	Assert( a.IsValid() && b.IsValid() );
	c.x = a.x * b.x;
	c.y = a.y * b.y;
}


inline void Vector2DDivide( const Vector2D& a, vec_t b, Vector2D& c )
{
	Assert( a.IsValid() );
	Assert( b != 0.0f );
	vec_t oob = 1.0f / b;
	c.x = a.x * oob;
	c.y = a.y * oob;
}

inline void Vector2DDivide( const Vector2D& a, const Vector2D& b, Vector2D& c )
{
	Assert( a.IsValid() );
	Assert( (b.x != 0.0f) && (b.y != 0.0f) );
	c.x = a.x / b.x;
	c.y = a.y / b.y;
}

inline void Vector2DMA( const Vector2D& start, float s, const Vector2D& dir, Vector2D& result )
{
	Assert( start.IsValid() && IsFinite(s) && dir.IsValid() );
	result.x = start.x + s*dir.x;
	result.y = start.y + s*dir.y;
}

// FIXME: Remove
// For backwards compatability
inline void	Vector2D::MulAdd(const Vector2D& a, const Vector2D& b, float scalar)
{
	x = a.x + b.x * scalar;
	y = a.y + b.y * scalar;
}

inline void Vector2DLerp(const Vector2D& src1, const Vector2D& src2, vec_t t, Vector2D& dest )
{
	dest[0] = src1[0] + (src2[0] - src1[0]) * t;
	dest[1] = src1[1] + (src2[1] - src1[1]) * t;
}

//-----------------------------------------------------------------------------
// dot, cross
//-----------------------------------------------------------------------------
inline vec_t DotProduct2D(const Vector2D& a, const Vector2D& b) 
{ 
	Assert( a.IsValid() && b.IsValid() );
	return( a.x*b.x + a.y*b.y ); 
}

// for backwards compatability
inline vec_t Vector2D::Dot( const Vector2D& vOther ) const
{
	return DotProduct2D( *this, vOther );
}


//-----------------------------------------------------------------------------
// length
//-----------------------------------------------------------------------------
inline vec_t Vector2DLength( const Vector2D& v )
{
	Assert( v.IsValid() );
	return (vec_t)fast_sqr(v.x*v.x + v.y*v.y);
}

inline vec_t Vector2D::LengthSqr(void) const	
{ 
	Assert( IsValid() );
	return (x*x + y*y);		
}

inline vec_t Vector2D::NormalizeInPlace()
{
	return Vector2DNormalize( *this );
}

inline bool Vector2D::IsLengthGreaterThan( float val ) const
{
	return LengthSqr() > val*val;
}

inline bool Vector2D::IsLengthLessThan( float val ) const
{
	return LengthSqr() < val*val;
}

inline vec_t Vector2D::Length(void) const	
{
	return Vector2DLength( *this );
}


inline void Vector2DMin( const Vector2D &a, const Vector2D &b, Vector2D &result )
{
	result.x = (a.x < b.x) ? a.x : b.x;
	result.y = (a.y < b.y) ? a.y : b.y;
}


inline void Vector2DMax( const Vector2D &a, const Vector2D &b, Vector2D &result )
{
	result.x = (a.x > b.x) ? a.x : b.x;
	result.y = (a.y > b.y) ? a.y : b.y;
}


//-----------------------------------------------------------------------------
// Normalization
//-----------------------------------------------------------------------------
inline vec_t Vector2DNormalize( Vector2D& v )
{
	Assert( v.IsValid() );
	vec_t l = v.Length();
	if (l != 0.0f)
	{
		v /= l;
	}
	else
	{
		v.x = v.y = 0.0f; 
	}
	return l;
}


//-----------------------------------------------------------------------------
// Get the distance from this Vector2D to the other one 
//-----------------------------------------------------------------------------
inline vec_t Vector2D::DistTo(const Vector2D &vOther) const
{
	Vector2D delta;
	Vector2DSubtract( *this, vOther, delta );
	return delta.Length();
}

inline vec_t Vector2D::DistToSqr(const Vector2D &vOther) const
{
	Vector2D delta;
	Vector2DSubtract( *this, vOther, delta );
	return delta.LengthSqr();
}


//-----------------------------------------------------------------------------
// Computes the closest point to vecTarget no farther than flMaxDist from vecStart
//-----------------------------------------------------------------------------
inline void ComputeClosestPoint2D( const Vector2D& vecStart, float flMaxDist, const Vector2D& vecTarget, Vector2D *pResult )
{
	Vector2D vecDelta;
	Vector2DSubtract( vecTarget, vecStart, vecDelta );
	float flDistSqr = vecDelta.LengthSqr();
	if ( flDistSqr <= flMaxDist * flMaxDist )
	{
		*pResult = vecTarget;
	}
	else
	{
		vecDelta /= fast_sqr( flDistSqr );
		Vector2DMA( vecStart, flMaxDist, vecDelta, *pResult );
	}
}



//-----------------------------------------------------------------------------
//
// Slow methods
//
//-----------------------------------------------------------------------------

#ifndef VECTOR_NO_SLOW_OPERATIONS

//-----------------------------------------------------------------------------
// Returns a Vector2D with the min or max in X, Y, and Z.
//-----------------------------------------------------------------------------

inline Vector2D Vector2D::Min(const Vector2D &vOther) const
{
	return Vector2D(x < vOther.x ? x : vOther.x, 
		y < vOther.y ? y : vOther.y);
}

inline Vector2D Vector2D::Max(const Vector2D &vOther) const
{
	return Vector2D(x > vOther.x ? x : vOther.x, 
		y > vOther.y ? y : vOther.y);
}


//-----------------------------------------------------------------------------
// arithmetic operations
//-----------------------------------------------------------------------------

inline Vector2D Vector2D::operator-(void) const
{ 
	return Vector2D(-x,-y);				
}

inline Vector2D Vector2D::operator+(const Vector2D& v) const	
{ 
	Vector2D res;
	Vector2DAdd( *this, v, res );
	return res;	
}

inline Vector2D Vector2D::operator-(const Vector2D& v) const	
{ 
	Vector2D res;
	Vector2DSubtract( *this, v, res );
	return res;	
}

inline Vector2D Vector2D::operator*(float fl) const	
{ 
	Vector2D res;
	Vector2DMultiply( *this, fl, res );
	return res;	
}

inline Vector2D Vector2D::operator*(const Vector2D& v) const	
{ 
	Vector2D res;
	Vector2DMultiply( *this, v, res );
	return res;	
}

inline Vector2D Vector2D::operator/(float fl) const	
{ 
	Vector2D res;
	Vector2DDivide( *this, fl, res );
	return res;	
}

inline Vector2D Vector2D::operator/(const Vector2D& v) const	
{ 
	Vector2D res;
	Vector2DDivide( *this, v, res );
	return res;	
}

inline Vector2D operator*(float fl, const Vector2D& v)	
{ 
	return v * fl; 
}

#endif //slow

#endif // VECTOR2D_H

