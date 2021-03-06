/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "g_local.h"


/*
=======================================================================

  SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
*/

/*
================
G_WriteClientSessionData

Called on game shutdown
================
*/
void G_WriteClientSessionData( gclient_t *client ) {
	const char  *s;
	const char  *var;

	// OSPx - Reset stats
	if (level.fResetStats) {
		G_deleteStats(client - level.clients);
	}

	s = va( "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",       // DHM - Nerve
			client->sess.sessionTeam,
			client->sess.spectatorTime,
			client->sess.spectatorState,
			client->sess.spectatorClient,
			client->sess.wins,
			client->sess.losses,
			client->sess.playerType,        // DHM - Nerve
			client->sess.playerWeapon,      // DHM - Nerve
			client->sess.playerItem,        // DHM - Nerve
			client->sess.playerSkin,        // DHM - Nerve
			client->sess.spawnObjectiveIndex, // DHM - Nerve
			client->sess.latchPlayerType,   // DHM - Nerve
			client->sess.latchPlayerWeapon, // DHM - Nerve
			client->sess.latchPlayerItem,   // DHM - Nerve
			client->sess.latchPlayerSkin,	// DHM - Nerve
			// OSPx 
			client->sess.uci,				// Country Flags
			client->sess.ip[0],
			client->sess.ip[1],
			client->sess.ip[2],
			client->sess.ip[3],
			client->sess.admin,
			client->sess.incognito,
			client->sess.ignored,
			client->sess.specInvited,
			client->sess.specLocked
			);

	var = va( "session%i", client - level.clients );

	trap_Cvar_Set( var, s );
}

/*
================
OSPx - G_ClientSwap

Client swap handling
================
*/
void G_ClientSwap(gclient_t *client) {
	int flags = 0;

	if (client->sess.sessionTeam == TEAM_RED) {
		client->sess.sessionTeam = TEAM_BLUE;
	}
	else if (client->sess.sessionTeam == TEAM_BLUE) {
		client->sess.sessionTeam = TEAM_RED;
	}

	// Swap spec invites as well
	if (client->sess.specInvited & TEAM_RED) {
		flags |= TEAM_BLUE;
	}
	if (client->sess.specInvited & TEAM_BLUE) {
		flags |= TEAM_RED;
	}

	client->sess.specInvited = flags;

	// Swap spec follows as well
	flags = 0;
	if (client->sess.specLocked & TEAM_RED) {
		flags |= TEAM_BLUE;
	}
	if (client->sess.specLocked & TEAM_BLUE) {
		flags |= TEAM_RED;
	}

	client->sess.specLocked = flags;
}

/*
================
G_ReadSessionData

Called on a reconnect
================
*/
void G_ReadSessionData( gclient_t *client ) {
	char s[MAX_STRING_CHARS];
	const char  *var;
	qboolean test;

	var = va( "session%i", client - level.clients );
	trap_Cvar_VariableStringBuffer( var, s, sizeof( s ) );

	sscanf( s, "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",       // DHM - Nerve
			(int *)&client->sess.sessionTeam,
			&client->sess.spectatorTime,
			(int *)&client->sess.spectatorState,
			&client->sess.spectatorClient,
			&client->sess.wins,
			&client->sess.losses,
			&client->sess.playerType,       // DHM - Nerve
			&client->sess.playerWeapon,     // DHM - Nerve
			&client->sess.playerItem,       // DHM - Nerve
			&client->sess.playerSkin,       // DHM - Nerve
			&client->sess.spawnObjectiveIndex, // DHM - Nerve
			&client->sess.latchPlayerType,  // DHM - Nerve
			&client->sess.latchPlayerWeapon,// DHM - Nerve
			&client->sess.latchPlayerItem,  // DHM - Nerve
			&client->sess.latchPlayerSkin,	// DHM - Nerve
			&client->sess.uci,
			&client->sess.ip[0],
			&client->sess.ip[1],
			&client->sess.ip[2],
			&client->sess.ip[3],
			(int *)&client->sess.admin,
			(int *)&client->sess.incognito,
			(int *)&client->sess.ignored,
			&client->sess.specInvited,
			&client->sess.specLocked
			);

	// OSPx - Pull and parse weapon stats
	*s = 0;
	trap_Cvar_VariableStringBuffer(va("wstats%i", (int)(client - level.clients)), s, sizeof(s));
	if (*s) {
		G_parseStats(s);
		if (g_gamestate.integer == GS_PLAYING) {
			client->sess.rounds++;
		}
	}

	// NERVE - SMF
	if (g_altStopwatchMode.integer) {
		test = qtrue;
	}
	else {
		test = g_currentRound.integer == 1;
	}

// OSPx
	if (g_gametype.integer == GT_WOLF_STOPWATCH && level.warmupTime > 0 && test) {
		G_ClientSwap(client);
	}

	if (g_swapteams.integer) {
		trap_Cvar_Set("g_swapteams", "0");
		G_ClientSwap(client);
	}
// -OSPx
}

/*
================
G_InitSessionData

Called on a first-time connect
================
*/
void G_InitSessionData( gclient_t *client, char *userinfo ) {
	clientSession_t *sess;
	const char      *value;

	sess = &client->sess;

	// initial team determination
	if ( g_gametype.integer >= GT_TEAM ) {
		// always spawn as spectator in team games
		sess->sessionTeam = TEAM_SPECTATOR;
	} else {
		value = Info_ValueForKey( userinfo, "team" );
		if ( value[0] == 's' ) {
			// a willing spectator, not a waiting-in-line
			sess->sessionTeam = TEAM_SPECTATOR;
		} else {
			switch ( g_gametype.integer ) {
			default:
			case GT_FFA:
			case GT_SINGLE_PLAYER:
				if ( g_maxGameClients.integer > 0 &&
					 level.numNonSpectatorClients >= g_maxGameClients.integer ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			case GT_TOURNAMENT:
				// if the game is full, go into a waiting mode
				if ( level.numNonSpectatorClients >= 2 ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			}
		}
	}

	sess->spectatorState = SPECTATOR_FREE;
	sess->spectatorTime = level.time;

	// DHM - Nerve
	sess->latchPlayerType = sess->playerType = 0;
	sess->latchPlayerWeapon = sess->playerWeapon = 0;
	sess->latchPlayerItem = sess->playerItem = 0;
	sess->latchPlayerSkin = sess->playerSkin = 0;

	sess->spawnObjectiveIndex = 0;
	// dhm - end

	// OSPx
	sess->uci = 255;
	sess->ip[0] = 0;
	sess->ip[1] = 0;
	sess->ip[2] = 0;
	sess->ip[3] = 0;
	sess->admin = USER_REGULAR;
	sess->incognito = qfalse;
	sess->ignored = qfalse;
	G_deleteStats(client - level.clients);
	sess->specInvited = 0;
	sess->specLocked = 0;
	// -OSPx

	G_WriteClientSessionData( client );
}


/*
==================
G_InitWorldSession

==================
*/
void G_InitWorldSession( void ) {
	char s[MAX_STRING_CHARS];
	int gt;

	trap_Cvar_VariableStringBuffer( "session", s, sizeof( s ) );
	gt = atoi( s );

	// if the gametype changed since the last session, don't use any
	// client sessions
	if ( g_gametype.integer != gt ) {
		level.newSession = qtrue;
		level.fResetStats = qtrue; // OSPx - Stats
		G_Printf( "Gametype changed, clearing session data.\n" );
	}
	// OSPx - Stats
	else {
		char *tmp = s;
		qboolean test = (g_altStopwatchMode.integer != 0 || g_currentRound.integer == 1);

#define GETVAL( x ) if ( ( tmp = strchr( tmp, ' ' ) ) == NULL ) {return; \
		} x = atoi(++tmp);

		// Get team lock stuff
		GETVAL(gt);
		teamInfo[TEAM_RED].spec_lock = (gt & TEAM_RED) ? qtrue : qfalse;
		teamInfo[TEAM_BLUE].spec_lock = (gt & TEAM_BLUE) ? qtrue : qfalse;

		if ((tmp = strchr(va("%s", tmp), ' ')) != NULL) {
			tmp++;
			trap_GetServerinfo(s, sizeof(s));
			if (Q_stricmp(tmp, Info_ValueForKey(s, "mapname"))) {
				level.fResetStats = qtrue;
				G_Printf("Map changed, clearing player stats.\n");
			}
		}

		// Make sure spec locks follow the right teams
		if (g_gametype.integer == GT_WOLF_STOPWATCH && g_gamestate.integer != GS_PLAYING && test) {
			G_swapTeamLocks();
		}

		if (g_swapteams.integer) {
			G_swapTeamLocks();
		}

	} // -OSPx
}

/*
==================
G_WriteSessionData

==================
*/
void G_WriteSessionData( void ) {
	int i;

	// OSPx - Speclock
	trap_Cvar_Set("session",
		va("%i %i", 
			g_gametype.integer,
			(teamInfo[TEAM_RED].spec_lock * TEAM_RED | teamInfo[TEAM_BLUE].spec_lock * TEAM_BLUE)
		)
	);

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			G_WriteClientSessionData( &level.clients[i] );
		}
	}

	// OSPx - Keep stats for all players in sync
	for (i = 0; !level.fResetStats && i < level.numConnectedClients; i++) {
		if ((g_gamestate.integer == GS_WARMUP_COUNTDOWN &&
			((g_gametype.integer == GT_WOLF_STOPWATCH && level.clients[level.sortedClients[i]].sess.rounds >= 2) ||
			(g_gametype.integer != GT_WOLF_STOPWATCH && level.clients[level.sortedClients[i]].sess.rounds >= 1)))) {
			level.fResetStats = qtrue;
		}
	}
}
