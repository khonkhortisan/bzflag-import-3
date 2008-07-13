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

/*
 * HUDuiServerList:
 *	FILL THIS IN
 */

#ifndef	__HUDUISERVERLIST_H__
#define	__HUDUISERVERLIST_H__

// ancestor class
#include "HUDuiScrollList.h"

#include "HUDuiServerListItem.h"
#include "ServerItem.h"

#include <string>
#include <vector>
#include <list>

#include "BzfEvent.h"

class HUDuiServerList : public HUDuiScrollList {
  public:
      HUDuiServerList();
      HUDuiServerList(bool paged);
      ~HUDuiServerList();

    void addItem(ServerItem item);
    void addItem(HUDuiControl* item);
    void addItem(HUDuiServerListItem* item); // Temporary

    void sortByDomain();
    void sortByServerName();
    void sortByPlayerCount();
    void sortByPing();

  protected:
    static bool compare_by_domain(HUDuiControl* first, HUDuiControl* second);
    static bool compare_by_name(HUDuiControl* first, HUDuiControl* second);
    static bool compare_by_players(HUDuiControl* first, HUDuiControl* second);
    static bool compare_by_ping(HUDuiControl* first, HUDuiControl* second);
};

#endif // __HUDuiServerList_H__

// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8