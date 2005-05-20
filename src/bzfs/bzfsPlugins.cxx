/* bzflag
* Copyright (c) 1993 - 2005 Tim Riker
*
* This package is free software;  you can redistribute it and/or
* modify it under the terms of the license found in the file
* named COPYING that should have accompanied this file.
*
* THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
* WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "WorldEventManager.h"

#ifdef _WIN32

#include <windows.h>

std::vector<HINSTANCE>	vLibHandles;

void loadPlugin ( std::string plugin, std::string config )
{
	int (*lpProc)(const char*);

	HINSTANCE	hLib = LoadLibrary(plugin.c_str());
	if (hLib)
	{
		lpProc = (int (__cdecl *)(const char*))GetProcAddress(hLib, "bz_Load");
		if (lpProc)
		{
			int ret =lpProc(config.c_str()); 
			vLibHandles.push_back(hLib);
		}
	}
}

void unloadPlugins ( void )
{
	int (*lpProc)(void);
	for (unsigned int i = 0; i < vLibHandles.size();i++)
	{
		lpProc = (int (__cdecl *)(void))GetProcAddress(vLibHandles[i], "bz_Unload");
		if (lpProc)
		{
			int ret =lpProc(); 
		}
	}
	vLibHandles.clear();
}

#else
// the other OSs need some DLL type loving!!!!
void loadPlugin ( std::string plugin, std::string config )
{
}

void unloadPlugins ( void )
{
}
#endif 


// Local Variables: ***
// mode:C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8
