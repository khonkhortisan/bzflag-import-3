/* bzflag
* Copyright (c) 1993 - 2006 Tim Riker
*
* This package is free software;  you can redistribute it and/or
* modify it under the terms of the license found in the file
* named COPYING that should have accompanied this file.
*
* THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*/

// bzflag global header
#include "bzfsClientMessages.h"
#include "BZDBCache.h"
#include "bzfsMessages.h"

bool handleClientEnter(void **buf, GameKeeper::Player *playerData)
{
	if (!playerData)
		return false;

	uint16_t rejectCode;
	char     rejectMsg[MessageLen];

	if (!playerData->player.unpackEnter(*buf, rejectCode, rejectMsg))
	{
		rejectPlayer(playerData->getIndex(), rejectCode, rejectMsg);
		return false;
	}

	playerData->accessInfo.setName(playerData->player.getCallSign());
	std::string timeStamp = TimeKeeper::timestamp();
	std::string playerIP = "local.player";
	if ( playerData->netHandler )
		playerIP = playerData->netHandler->getTargetIP();

	DEBUG1("Player %s [%d] has joined from %s at %s with token \"%s\"\n",
		playerData->player.getCallSign(),
		playerData->getIndex(), playerIP.c_str(), timeStamp.c_str(),
		playerData->player.getToken());

	if (!clOptions->publicizeServer)
		playerData->_LSAState = GameKeeper::Player::notRequired;
	else if (strlen(playerData->player.getCallSign()))
		playerData->_LSAState = GameKeeper::Player::required;

	return true;
}

void handleClientExit ( GameKeeper::Player *playerData )
{
	// data: <none>
	removePlayer(playerData->getIndex(), "left", false);
}

void handleSetVar ( NetHandler *netHandler )
{
	if (!netHandler)
		return;

	void *bufStart = getDirectMessageBuffer();
	PackVars pv(bufStart, netHandler);
	BZDB.iterate(PackVars::packIt, &pv);
}

void handleFlagNegotiation( NetHandler *handler, void **buf, int len )
{
	if (!handler)
		return;

	void *bufStart;
	FlagTypeMap::iterator it;
	FlagSet::iterator m_it;
	FlagOptionMap hasFlag;
	FlagSet missingFlags;
	unsigned short numClientFlags = len/2;

	/* Unpack incoming message containing the list of flags our client supports */
	for (int i = 0; i < numClientFlags; i++)
	{
		FlagType *fDesc;
		*buf = FlagType::unpack(*buf, fDesc);
		if (fDesc != Flags::Null)
			hasFlag[fDesc] = true;
	}

	/* Compare them to the flags this game might need, generating a list of missing flags */
	for (it = FlagType::getFlagMap().begin(); it != FlagType::getFlagMap().end(); ++it)
	{
		if (!hasFlag[it->second])
		{
			if (clOptions->flagCount[it->second] > 0)
				missingFlags.insert(it->second);
			if ((clOptions->numExtraFlags > 0) && !clOptions->flagDisallowed[it->second])
				missingFlags.insert(it->second);
		}
	}

	/* Pack a message with the list of missing flags */
	void *buf2 = bufStart = getDirectMessageBuffer();
	for (m_it = missingFlags.begin(); m_it != missingFlags.end(); ++m_it)
	{
		if ((*m_it) != Flags::Null)
			buf2 = (*m_it)->pack(buf2);
	}
	directMessage(handler, MsgNegotiateFlags, (char*)buf2-(char*)bufStart, bufStart);
}

void handleWorldChunk( NetHandler *handler, void *buf )
{
	uint32_t ptr;	// data: count (bytes read so far)
	buf = nboUnpackUInt(buf, ptr);

	sendWorldChunk(handler, ptr);
}

void handlePlayerUpdate ( void **buf, uint16_t &code, GameKeeper::Player *playerData, const void *rawbuf, int len )
{
	if (!playerData)
		return;

	float timestamp;
	PlayerState state;

	*buf = nboUnpackFloat(*buf, timestamp);
	*buf = state.unpack(*buf,code);

	// observer updates are not relayed
	if (playerData->player.isObserver())
	{
		// skip all of the checks
		playerData->setPlayerState(state, timestamp);
		return;
	}

	// silently drop old packet
	if (state.order <= playerData->lastState.order)
		return;

	// Don't kick players if the world param is still changing,
	if (!worldStateChanging())
	{

		// see if the player is too high
		if (cheatProtectionOptions.doHeightChecks) {

			static const float heightFudge = 1.10f; /* 10% */

			float wingsGravity = BZDB.eval(StateDatabase::BZDB_WINGSGRAVITY);
			float normalGravity = BZDBCache::gravity;
			if ((wingsGravity < 0.0f) && (normalGravity < 0.0f)) {

				float wingsMaxHeight = BZDB.eval(StateDatabase::BZDB_WINGSJUMPVELOCITY);
				wingsMaxHeight *= wingsMaxHeight;
				wingsMaxHeight *= (1 + BZDB.eval(StateDatabase::BZDB_WINGSJUMPCOUNT));
				wingsMaxHeight /= (-wingsGravity * 0.5f);

				float normalMaxHeight = BZDB.eval(StateDatabase::BZDB_JUMPVELOCITY);
				normalMaxHeight *= normalMaxHeight;
				normalMaxHeight /= (-normalGravity * 0.5f);

				float maxHeight;
				if (wingsMaxHeight > normalMaxHeight) {
					maxHeight = wingsMaxHeight;
				} else {
					maxHeight = normalMaxHeight;
				}

				// final adjustments
				maxHeight *= heightFudge;
				maxHeight += maxWorldHeight;

				if (state.pos[2] > maxHeight) {
					DEBUG1("Kicking Player %s [%d] jumped too high [max: %f height: %f]\n",
						playerData->player.getCallSign(), playerData->getIndex(), maxHeight, state.pos[2]);
					sendMessage(ServerPlayer, playerData->getIndex(), "Autokick: Player location was too high.");
					removePlayer(playerData->getIndex(), "too high", true);
					return;
				}
			}
		}

		// make sure the player is still in the map
		// test all the map bounds + some fudge factor, just in case
		static const float positionFudge = 10.0f; /* linear distance */
		bool InBounds = true;
		float worldSize = BZDBCache::worldSize;
		if ( (state.pos[1] >= worldSize*0.5f + positionFudge) || (state.pos[1] <= -worldSize*0.5f - positionFudge)) {
			std::cout << "y position (" << state.pos[1] << ") is out of bounds (" << worldSize * 0.5f << " + " << positionFudge << ")" << std::endl;
			InBounds = false;
		} else if ( (state.pos[0] >= worldSize*0.5f + positionFudge) || (state.pos[0] <= -worldSize*0.5f - positionFudge)) {
			std::cout << "x position (" << state.pos[0] << ") is out of bounds (" << worldSize * 0.5f << " + " << positionFudge << ")" << std::endl;
			InBounds = false;
		}

		static const float burrowFudge = 1.0f; /* linear distance */
		if (state.pos[2]<BZDB.eval(StateDatabase::BZDB_BURROWDEPTH) - burrowFudge) {
			std::cout << "z depth (" << state.pos[2] << ") is less than burrow depth (" << BZDB.eval(StateDatabase::BZDB_BURROWDEPTH) << " - " << burrowFudge << ")" << std::endl;
			InBounds = false;
		}

		// kick em cus they are most likely cheating or using a buggy client
		if (!InBounds)
		{
			DEBUG1("Kicking Player %s [%d] Out of map bounds at position (%.2f,%.2f,%.2f)\n",
				playerData->player.getCallSign(), playerData->getIndex(),
				state.pos[0], state.pos[1], state.pos[2]);
			sendMessage(ServerPlayer, playerData->getIndex(), "Autokick: Player location was outside the playing area.");
			removePlayer(playerData->getIndex(), "Out of map bounds", true);
		}

		// Speed problems occur around flag drops, so don't check for
		// a short period of time after player drops a flag. Currently
		// 2 second, adjust as needed.
		if (playerData->player.isFlagTransitSafe()) {

			// we'll be checking against the player's flag type
			int pFlag = playerData->player.getFlag();

			// check for highspeed cheat; if inertia is enabled, skip test for now
			if (BZDB.eval(StateDatabase::BZDB_INERTIALINEAR) == 0.0f) {
				// Doesn't account for going fast backwards, or jumping/falling
				float curPlanarSpeedSqr = state.velocity[0]*state.velocity[0] +
					state.velocity[1]*state.velocity[1];

				float maxPlanarSpeed = BZDBCache::tankSpeed;

				bool logOnly = false;

				// if tank is not driving cannot be sure it didn't toss
				// (V) in flight

				// if tank is not alive cannot be sure it didn't just toss
				// (V)
				if (pFlag >= 0) {
					FlagInfo &flag = *FlagInfo::get(pFlag);
					if (flag.flag.type == Flags::Velocity)
						maxPlanarSpeed *= BZDB.eval(StateDatabase::BZDB_VELOCITYAD);
					else if (flag.flag.type == Flags::Thief)
						maxPlanarSpeed *= BZDB.eval(StateDatabase::BZDB_THIEFVELAD);
					else if (flag.flag.type == Flags::Agility)
						maxPlanarSpeed *= BZDB.eval(StateDatabase::BZDB_AGILITYADVEL);
					else if ((flag.flag.type == Flags::Burrow) &&
						(playerData->lastState.pos[2] == state.pos[2]) &&
						(playerData->lastState.velocity[2] == state.velocity[2]) &&
						(state.pos[2] <= BZDB.eval(StateDatabase::BZDB_BURROWDEPTH)))
						// if we have burrow and are not actively burrowing
						// You may have burrow and still be above ground. Must
						// check z in ground!!
						maxPlanarSpeed *= BZDB.eval(StateDatabase::BZDB_BURROWSPEEDAD);
				}
				float maxPlanarSpeedSqr = maxPlanarSpeed * maxPlanarSpeed;

				// If player is moving vertically, or not alive the speed checks
				// seem to be problematic. If this happens, just log it for now,
				// but don't actually kick
				if ((playerData->lastState.pos[2] != state.pos[2])
					||  (playerData->lastState.velocity[2] != state.velocity[2])
					||  ((state.status & PlayerState::Alive) == 0)) {
						logOnly = true;
					}

					// allow a 10% tolerance level for speed if -speedtol is not sane
					if (cheatProtectionOptions.doSpeedChecks) {
						float realtol = 1.1f;
						if (speedTolerance > 1.0f)
							realtol = speedTolerance;
						maxPlanarSpeedSqr *= realtol;
						if (curPlanarSpeedSqr > maxPlanarSpeedSqr) {
							if (logOnly) {
								DEBUG1("Logging Player %s [%d] tank too fast (tank: %f, allowed: %f){Dead or v[z] != 0}\n",
									playerData->player.getCallSign(), playerData->getIndex(),
									sqrt(curPlanarSpeedSqr), sqrt(maxPlanarSpeedSqr));
							} else {
								DEBUG1("Kicking Player %s [%d] tank too fast (tank: %f, allowed: %f)\n",
									playerData->player.getCallSign(), playerData->getIndex(),
									sqrt(curPlanarSpeedSqr), sqrt(maxPlanarSpeedSqr));
								sendMessage(ServerPlayer, playerData->getIndex(), "Autokick: Player tank is moving too fast.");
								removePlayer(playerData->getIndex(), "too fast");
							}
							return;
						}
					}
			}
		}
	}

	playerData->setPlayerState(state, timestamp);

	// Player might already be dead and did not know it yet (e.g. teamkill)
	// do not propogate
	if (!playerData->player.isAlive() && (state.status & short(PlayerState::Alive)))
	return;

	// observer shouldn't send bulk messages anymore, they used to
	// when it was a server-only hack; but the check does not hurt,
	// either
	if (playerData->player.isObserver())
	return;

	searchFlag(*playerData);

	//TODO for server bot, send the update to the handler, since this will skip users
	relayPlayerPacket(playerData->getIndex(), len, rawbuf, code);
	return;
}



// Local Variables: ***
// mode:C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8
