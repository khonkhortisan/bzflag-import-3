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

// logDetail.cpp : Plugin module for logging server events to stdout
//

#include <iostream>
#include <sstream>
#include "bzfsAPI.h"
#include "plugin_utils.h"

BZ_GET_PLUGIN_VERSION

enum action {join , auth , part};

class LogDetail:public bz_EventHandler
{
public:
  LogDetail();
  virtual ~LogDetail();
  virtual void process(bz_EventData *eventData);
private:
  std::string displayPlayerPrivs(const int playerID);
  std::string displayCallsign(const bz_ApiString &callsign);
  std::string displayCallsign(const int playerID);
  std::string displayBZid(const int playerID);
  std::string displayTeam(const bz_eTeamType team);
  virtual void listPlayers(action act, bz_PlayerJoinPartEventData_V1 *data);
};

LogDetail logDetailHandler;

BZF_PLUGIN_CALL int bz_Load(const char * /*commandLine */ )
{
  bz_registerEvent(bz_eSlashCommandEvent, &logDetailHandler);
  bz_registerEvent(bz_eRawChatMessageEvent, &logDetailHandler);
  bz_registerEvent(bz_eServerMsgEvent, &logDetailHandler);
  bz_registerEvent(bz_ePlayerJoinEvent, &logDetailHandler);
  bz_registerEvent(bz_ePlayerPartEvent, &logDetailHandler);
  bz_registerEvent(bz_ePlayerAuthEvent, &logDetailHandler);
  bz_registerEvent(bz_eMessageFilteredEvent, &logDetailHandler);
  bz_debugMessage(4, "logDetail plugin loaded");
  return 0;
}

BZF_PLUGIN_CALL int bz_Unload(void)
{
  bz_removeEvent(bz_eSlashCommandEvent, &logDetailHandler);
  bz_removeEvent(bz_eRawChatMessageEvent, &logDetailHandler);
  bz_removeEvent(bz_eServerMsgEvent, &logDetailHandler);
  bz_removeEvent(bz_ePlayerJoinEvent, &logDetailHandler);
  bz_removeEvent(bz_ePlayerPartEvent, &logDetailHandler);
  bz_removeEvent(bz_ePlayerAuthEvent, &logDetailHandler);
  bz_removeEvent(bz_eMessageFilteredEvent, &logDetailHandler);
  bz_debugMessage(4, "logDetail plugin unloaded");
  return 0;
}

LogDetail::LogDetail()
{
  bz_debugMessage(0, "SERVER-STATUS Running");
  bz_debugMessagef(0, "SERVER-MAPNAME %s", bz_getPublicDescription().c_str());
  listPlayers(join, NULL);
}

LogDetail::~LogDetail()
{
  listPlayers(part, NULL);
  bz_debugMessage(0, "SERVER-STATUS Stopped");
}

void LogDetail::process(bz_EventData *eventData)
{
  bz_ChatEventData_V1 *chatData = (bz_ChatEventData_V1 *) eventData;
  bz_ServerMsgEventData_V1 *serverMsgData = (bz_ServerMsgEventData_V1 *) eventData;
  bz_SlashCommandEventData_V1 *cmdData = (bz_SlashCommandEventData_V1 *) eventData;
  bz_PlayerJoinPartEventData_V1 *joinPartData = (bz_PlayerJoinPartEventData_V1 *) eventData;
  bz_PlayerAuthEventData_V1 *authData = (bz_PlayerAuthEventData_V1 *) eventData;
  bz_MessageFilteredEventData_V1 *filteredData = (bz_MessageFilteredEventData_V1 *) eventData;
  char temp[9] = { 0 };

  if (eventData) {
    switch (eventData->eventType) {
    case bz_eSlashCommandEvent:
      // Slash commands are case insensitive
      // Tokenize the stream and check the first word
      // /report -> MSG-REPORT
      // anything -> MSG-COMMAND

      strncpy(temp, cmdData->message.c_str(), 8);

      if (strcasecmp(temp, "/REPORT ") == 0) {
	bz_debugMessagef(0, "MSG-REPORT %s %s",
			 displayCallsign(cmdData->from).c_str(),
			 cmdData->message.c_str()+8);
      } else {
	bz_debugMessagef(0, "MSG-COMMAND %s %s",
			 displayCallsign(cmdData->from).c_str(),
			 cmdData->message.c_str()+1);
      }
      break;
    case bz_eRawChatMessageEvent:
      if ((chatData->to == BZ_ALLUSERS) && (chatData->team == eNoTeam)) {
	bz_debugMessagef(0, "MSG-BROADCAST %s %s",
			 displayCallsign(chatData->from).c_str(),
			 chatData->message.c_str());
      } else if (chatData->to == BZ_NULLUSER) {
	if (chatData->team == eAdministrators) {
	  bz_debugMessagef(0, "MSG-ADMIN %s %s",
			   displayCallsign(chatData->from).c_str(),
			   chatData->message.c_str());
	} else {
	  bz_debugMessagef(0, "MSG-TEAM %s %s %s",
			   displayCallsign(chatData->from).c_str(),
			   displayTeam(chatData->team).c_str(),
			   chatData->message.c_str());
	}
      } else {
	  bz_debugMessagef(0, "MSG-DIRECT %s %s %s",
			   displayCallsign(chatData->from).c_str(),
			   displayCallsign(chatData->to).c_str(),
			   chatData->message.c_str());
      }
      break;
    case bz_eMessageFilteredEvent:
      bz_debugMessagef(0, "MSG-FILTERED %s %s",
		       displayCallsign(filteredData->playerID).c_str(),
		       filteredData->filteredMessage.c_str());
      break;
    case bz_eServerMsgEvent:
      if ((serverMsgData->to == BZ_ALLUSERS) && (serverMsgData->team == eNoTeam)) {
	bz_debugMessagef(0, "MSG-BROADCAST 6:SERVER %s",
			 serverMsgData->message.c_str());
      } else if (serverMsgData->to == BZ_NULLUSER) {
	if (serverMsgData->team == eAdministrators) {
	  bz_debugMessagef(0, "MSG-ADMIN 6:SERVER %s",
			   serverMsgData->message.c_str());
	} else {
	  bz_debugMessagef(0, "MSG-TEAM 6:SERVER %s %s",
			   displayTeam(serverMsgData->team).c_str(),
			   serverMsgData->message.c_str());
	}
      } else {
	bz_debugMessagef(0, "MSG-DIRECT 6:SERVER %s %s",
			 displayCallsign(serverMsgData->to ).c_str(),
			 serverMsgData->message.c_str());
      }
      break;
    case bz_ePlayerJoinEvent:
      {
	if (joinPartData->record) {
	  bz_debugMessagef(0, "PLAYER-JOIN %s #%d%s %s %s",
			   displayCallsign(joinPartData->record->callsign).c_str(),
			   joinPartData->playerID,
			   displayBZid(joinPartData->playerID).c_str(),
			   displayTeam(joinPartData->record->team).c_str(),
			   displayPlayerPrivs(joinPartData->playerID).c_str());
	  listPlayers(join, joinPartData);
	}
      }
      break;
    case bz_ePlayerPartEvent:
      bz_debugMessagef(0, "PLAYER-PART %s #%d%s %s",
		       displayCallsign(joinPartData->playerID).c_str(),
		       joinPartData->playerID,
		       displayBZid(joinPartData->playerID).c_str(),
		       joinPartData->reason.c_str());
      listPlayers(part, joinPartData);
      break;
    case bz_ePlayerAuthEvent:
      bz_debugMessagef(0, "PLAYER-AUTH %s %s",
		       displayCallsign(authData->playerID).c_str(),
		       displayPlayerPrivs(authData->playerID).c_str()),
	listPlayers(join, joinPartData);
      break;
    default:
      break;
    }
  }
}

std::string LogDetail::displayBZid(const int playerID)
{
  std::ostringstream bzid;

  bz_BasePlayerRecord *player = bz_getPlayerByIndex(playerID);
  if (player) {
    if (player->globalUser)
      bzid << " BZid:" << player->bzID.c_str();
    bz_freePlayerRecord( player );
  }

  return bzid.str();
}

std::string LogDetail::displayPlayerPrivs(const int playerID)
{
  std::ostringstream playerPrivs;

  bz_BasePlayerRecord *player = bz_getPlayerByIndex(playerID);
  if (player) {
    playerPrivs << "IP:" << player->ipAddress.c_str();
    if (player->verified) playerPrivs << " VERIFIED";
    if (player->globalUser) playerPrivs << " GLOBALUSER";
    if (player->admin) playerPrivs << " ADMIN";
    if (player->op) playerPrivs << " OPERATOR";
    bz_freePlayerRecord(player);
  } else {
    playerPrivs << "IP:0.0.0.0";
  }

  return playerPrivs.str();
}

std::string LogDetail::displayCallsign(const bz_ApiString &callsign)
{
  std::ostringstream result;

  result << (unsigned int)strlen(callsign.c_str()) << ":" << callsign.c_str();

  return result.str();
}

std::string LogDetail::displayCallsign(const int playerID)
{
  std::ostringstream callsign;
  bz_BasePlayerRecord *player = bz_getPlayerByIndex(playerID);
  if (player) {
    callsign << (unsigned int)strlen(player->callsign.c_str()) << ":";
    callsign << player->callsign.c_str();
    bz_freePlayerRecord(player);
  } else {
    callsign << "7:UNKNOWN";
  }
  return callsign.str();
}


std::string LogDetail::displayTeam(const bz_eTeamType team)
{
  return toupper(bzu_GetTeamName(team),std::string());
}

void LogDetail::listPlayers(action act, bz_PlayerJoinPartEventData_V1 * data)
{
  bz_APIIntList *playerList = bz_newIntList();
  bz_BasePlayerRecord *player;
  std::ostringstream msg;
  char playerStatus;
  int numPlayers;

  bz_getPlayerIndexList(playerList);

  bz_debugMessage(4, "Players:");
  //
  // Count number of players
  //
  numPlayers = 0;
  for (unsigned int i = 0; i < playerList->size(); i++) {
    player = bz_getPlayerByIndex(playerList->get(i));
    if (player) {
      if ((player->callsign != "") && (act == join || act == auth || (data && (player->playerID != data->playerID))))
	numPlayers++;
      bz_freePlayerRecord(player);
    }
  }

  //
  // Display number of players and callsign in the following format:
  //
  // PLAYERS (nn) [G]cc:callsign
  // nn - number of players
  // G  - global auth identifier (+|-| |@)
  // cc - count of characters in player callsign
  // callsign - player callsign
  //
  // eg.
  // PLAYERS (2) [@]7:Thumper() [ ]3:xxx()
  // Extra data goes in between the parenthesis (currently empty)
  //
  msg.str("");
  msg << "PLAYERS (" << numPlayers << ") ";
  for (unsigned int i = 0; i < playerList->size(); i++) {
    player = bz_getPlayerByIndex(playerList->get(i));
    if (player) {
      if ((player->callsign != "") && (act == join || act == auth || (data && (player->playerID != data->playerID)))) {
	playerStatus = ' ';
	if (player->globalUser)
	  playerStatus = '+';
	if (player->verified)
	  playerStatus = '+';
	if (player->admin && ! bz_hasPerm(player->playerID, bz_perm_hideAdmin))
	  playerStatus = '@';
	msg << "[" << playerStatus << "]";
	msg << player->callsign.size() << ':';
	msg << player->callsign.c_str();
	msg << "() ";
      }
      bz_freePlayerRecord(player);
    }
  }
  bz_debugMessage(0, msg.str().c_str());

  bz_deleteIntList(playerList);
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8
