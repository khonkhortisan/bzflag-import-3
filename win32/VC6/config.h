/* bzflag
 * Copyright (c) 1993 - 2003 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* win32/config.h.  Generated by hand by Jeff Myers 6-12-03 */
/* this config is just for visual C++ since it donsn't use automake*/

/* This is a really really fugly hack to get around winsock sillyness
 * The newer versions of winsock have a socken_t typedef, and there
 * doesn't seem to be any way to tell the versions apart. However,
 * VC++ helps us out here by treating typedef as #define
 * If we've got a socklen_t typedefed, define HAVE_SOCKLEN_T to
 * avoid #define'ing it in common.h */

// IF YOU HAVE THE NEW PLAT SDK COMMOENT THIS OUT
// VC5 and VC6 standard need it
// AND David don't you DARE COMMIT a change to put this back.
#define socklen_t int

/* Time Bomb expiration */
/* #undef TIME_BOMB */

/* Debug Rendering */
/* #undef DEBUG_RENDERING */

/* Enabling Robots */
#define ROBOT 1

/* Enabling Snapping */
#define SNAPPING 1

/* On windows, strcasecmp is really stricmp */
#define HAVE_STRICMP 1

// define our OS
#ifndef BZ_BUILD_OS
 	#if defined(_MSC_VER) && (_MSC_VER == 1100)
 		#ifdef _DEBUG
 			#define BZ_BUILD_OS			"W32VC5D"
 		#else
 			#define BZ_BUILD_OS			"W32VC5"
 		#endif //_DEBUG
 	#endif //_MSC_VER == 1100
 	#if defined(_MSC_VER) && (_MSC_VER == 1200)
 		#ifdef _DEBUG
 			#define BZ_BUILD_OS			"W32VC6D"
 		#else
 			#define BZ_BUILD_OS			"W32VC6"
 		#endif //_DEBUG
 	#endif //_MSC_VER == 1200
#endif
