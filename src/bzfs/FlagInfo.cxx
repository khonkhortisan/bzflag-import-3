/* bzflag
 * Copyright (c) 1993 - 2004 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named LICENSE that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* no header other than FlagInfo.h should be included here */

#ifdef _MSC_VER
#pragma warning( 4:4786)
#endif

#include "FlagInfo.h"


/* private */

/* protected */

/* public */

// flags list
FlagInfo *FlagInfo::flagList = NULL;

FlagInfo::FlagInfo()
{
  // prep flag
  flag.type               = Flags::Null;
  flag.status             = FlagNoExist;
  flag.endurance          = FlagNormal;
  flag.owner              = NoPlayer;
  flag.position[0]        = 0.0f;
  flag.position[1]        = 0.0f;
  flag.position[2]        = 0.0f;
  flag.launchPosition[0]  = 0.0f;
  flag.launchPosition[1]  = 0.0f;
  flag.launchPosition[2]  = 0.0f;
  flag.landingPosition[0] = 0.0f;
  flag.landingPosition[1] = 0.0f;
  flag.landingPosition[2] = 0.0f;
  flag.flightTime         = 0.0f;
  flag.flightEnd          = 0.0f;
  flag.initialVelocity    = 0.0f;
  player                  = -1;
  grabs                   = 0;
}

void FlagInfo::setSize(int numFlags)
{
  delete[] flagList;
  flagList = new FlagInfo[numFlags];
  for (int i = 0; i < numFlags; i++)
    flagList[i].flagIndex = i;
}

void FlagInfo::setRequiredFlag(FlagType *desc)
{
  required = true;
  flag.type = desc;
}

void FlagInfo::addFlag()
{
  const float flagAltitude = BZDB.eval(StateDatabase::BZDB_FLAGALTITUDE);
  const float gravity      = BZDB.eval(StateDatabase::BZDB_GRAVITY);
  const float maxGrabs     = BZDB.eval(StateDatabase::BZDB_MAXFLAGGRABS);

  // flag in now entering game
  flag.status          = FlagComing;

  // compute drop time
  const float flightTime = 2.0f * sqrtf(-2.0f * flagAltitude / gravity);
  flag.flightTime        = 0.0f;
  flag.flightEnd         = flightTime;
  flag.initialVelocity   = -0.5f * gravity * flightTime;
  dropDone               = TimeKeeper::getCurrent();
  dropDone              += flightTime;
	
  // decide how sticky the flag will be
  if (flag.type->flagQuality == FlagBad)
    flag.endurance = FlagSticky;
  else
    flag.endurance = FlagUnstable;

  // how times will it stick around
  if ((flag.endurance == FlagSticky) || (flag.type == Flags::Thief))
    grabs = 1;
  else
    grabs = int(floor(maxGrabs * (float)bzfrand())) + 1;
}

void *FlagInfo::pack(void *buf)
{
  buf = nboPackUShort(buf, flagIndex);
  buf = FlagInfo::flagList[flagIndex].flag.pack(buf);
  return buf;
}

// Local Variables: ***
// mode:C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8

