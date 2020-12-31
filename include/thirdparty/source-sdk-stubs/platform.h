#ifndef NABENABE_SDK_STUBS_PLATFORM_H
#define NABENABE_SDK_STUBS_PLATFORM_H

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

#define abstract_class class

// Used for standard calling conventions
#if defined( _WIN32 ) && !defined( _X360 )
#define  STDCALL				__stdcall
#define  FASTCALL				__fastcall
#define  FORCEINLINE			__forceinline
// GCC 3.4.1 has a bug in supporting forced inline of templated functions
// this macro lets us not force inlining in that case
#define  FORCEINLINE_TEMPLATE		__forceinline
#elif defined( _X360 )
#define  STDCALL				__stdcall
#ifdef FORCEINLINE
#undef FORCEINLINE
#endif
#define  FORCEINLINE			__forceinline
#define  FORCEINLINE_TEMPLATE		__forceinline
#else
#define  STDCALL
#define  FASTCALL
#ifdef __linux___DEBUGGABLE
#define  FORCEINLINE
#else
#define  FORCEINLINE inline
#endif
// GCC 3.4.1 has a bug in supporting forced inline of templated functions
// this macro lets us not force inlining in that case
#define  FORCEINLINE_TEMPLATE inline
#define  __stdcall			__attribute__ ((__stdcall__))
#endif

// decls for aligning data
#ifdef _WIN32
        #define DECL_ALIGN(x) __declspec(align(x))

#elif __linux__
	#define DECL_ALIGN(x) __attribute__((aligned(x)))
#endif

#ifndef _WIN32
#ifndef __linux__
#define DECL_ALIGN(x) /* */
#endif
#endif

#define ALIGN8 DECL_ALIGN(8)
#define ALIGN16 DECL_ALIGN(16)
#define ALIGN32 DECL_ALIGN(32)
#define ALIGN128 DECL_ALIGN(128)

#ifndef Assert
// Just do nothing on the SDK asserts, so we can get this to compile.
#define Assert(x)
#endif

#define uint64 UINT64

#endif // NABENABE_SDK_STUBS_PLATFORM_H
