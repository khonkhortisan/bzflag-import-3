/* bzflag
 * Copyright (c) 1993 - 2008 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef __BZWORLD_H__
#define __BZWORLD_H__

// The following are only here to define them in the documentation

/// This is the top level namespace of libBZWorld
namespace BZW
{
  /// This is the Parser namespace. If a user desires control over specific
  //parsing functions, this namespace keeps things organized for them.
  namespace Parser
  {

  }
}

// Headers in order of level-ness (low to high)

#include "BZW/WorldObject.h"
#include "BZW/World.h"

#endif // __BZWORLD_H__

// Local Variables: ***
// Mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8
