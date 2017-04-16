/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "server.h"

/*
===============================================================================
OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                          //
//  UTILITIES                                                                                               //
//                                                                                                          //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// Name        : SV_GetPlayerByParam
// Description : Returns the player with handle read from given input
/////////////////////////////////////////////////////////////////////
client_t *SV_GetPlayerByParam(const char *s) {

    char        name[MAX_NAME_LENGTH];
    int         count = 0;
    int         i, idnum;
    client_t    *cl;
    client_t    *matches[MAX_CLIENTS];

    // make sure server is running
    if (!com_sv_running->integer) {
        return NULL;
    }

    if (!s) {
        Com_Printf("No client specified\n");
        return NULL;
    }

    // check whether this is a numeric player handle
    for (i = 0; s[i] >= '0' && s[i] <= '9'; i++);

    if (!s[i]) {

        // numeric player handle given as input
        idnum = atoi(s);
        if ((idnum < 0) || (idnum >= sv_maxclients->integer)) {
            Com_Printf("Bad client slot: %i\n", idnum);
            return NULL;
        }

        cl = &svs.clients[idnum];

        if (!cl->state) {
            Com_Printf("Client in slot %i is not connected\n", idnum);
            return NULL;
        }

        return cl;

    } else {

        // match given input with player names
        for (i = 0; i < sv_maxclients->integer ; i++) {

            cl = &svs.clients[i];

            // client is not connected
            if (!cl->state) {
                continue;
            }
            
            Q_strncpyz(name, cl->name, sizeof(name));
            Q_CleanStr(name);

            // check for exact match
            if (!Q_stricmp(name, s)) {
                matches[0] = &svs.clients[i];
                count = 1;
                break;
            }  
            #ifdef USE_AUTH
            // check exact match on auth name
            else if (!Q_stricmp(cl->auth, s)) {
                matches[0] = &svs.clients[i];
                count = 1;
                break;
            }
            #endif

            // check for substring match
            if (Q_strisub(name, s)) {
                matches[count] = &svs.clients[i];
                count++;
            }
            #ifdef USE_AUTH
            // check substring match match on auth name
            else if (!Q_stricmp(cl->auth, s)) {
                matches[count] = &svs.clients[i];
                count++;
            }
            #endif

        }

        if (count == 0) {

            // no match found for the given input string
            Com_Printf("No client found matching %s\n", s);
            return NULL;

        } else if (count > 1) {

            // multiple matches found for the given string
            Com_Printf("Multiple clients found matching %s:\n", s);

            for (i = 0; i < count; i++) {
                cl = matches[i];
                strcpy(name, cl->name);
                Com_Printf(" %2d: [%s]\n", (int)(cl - svs.clients), name);
            }

            return NULL;
        }

        // found just 1 match
        return matches[0];

    }

}

/////////////////////////////////////////////////////////////////////
// Name        : SV_GetPlayerByHandle
// Description : Returns the player with handle read from Cmd_Argv(1)
/////////////////////////////////////////////////////////////////////
static client_t *SV_GetPlayerByHandle(void) {

    char        *s;
    char        name[MAX_NAME_LENGTH];
    int         count = 0;
    int         i, idnum;
    client_t    *cl;
    client_t    *matches[MAX_CLIENTS];

    // make sure server is running
    if (!com_sv_running->integer) {
        return NULL;
    }

    if (Cmd_Argc() < 2) {
        Com_Printf("No client specified\n");
        return NULL;
    }

    s = Cmd_Argv(1);

    // check whether this is a numeric player handle
    for (i = 0; s[i] >= '0' && s[i] <= '9'; i++);

    if (!s[i]) {

        // numeric player handle given as input
        idnum = atoi(s);
        if ((idnum < 0) || (idnum >= sv_maxclients->integer)) {
            Com_Printf("Bad client slot: %i\n", idnum);
            return NULL;
        }

        cl = &svs.clients[idnum];

        if (!cl->state) {
            Com_Printf("Client in slot %i is not connected\n", idnum);
            return NULL;
        }

        return cl;

    } else {

        // match given input with player names
        for (i = 0; i < sv_maxclients->integer ; i++) {

            cl = &svs.clients[i];

            // client is not connected
            if (!cl->state) {
                continue;
            }
            
            Q_strncpyz(name, cl->name, sizeof(name));
            Q_CleanStr(name);

            // check for exact match
            if (!Q_stricmp(name, s)) {
                matches[0] = &svs.clients[i];
                count = 1;
                break;
            }  
            #ifdef USE_AUTH
            // check exact match on auth name
            else if (!Q_stricmp(cl->auth, s)) {
                matches[0] = &svs.clients[i];
                count = 1;
                break;
            }
            #endif

            // check for substring match
            if (Q_strisub(name, s)) {
                matches[count] = &svs.clients[i];
                count++;
            }
            #ifdef USE_AUTH
            // check substring match match on auth name
            else if (!Q_stricmp(cl->auth, s)) {
                matches[count] = &svs.clients[i];
                count++;
            }
            #endif
        }

        if (count == 0) {

            // no match found for the given input string
            Com_Printf("No client found matching %s\n", s);
            return NULL;

        } else if (count > 1) {

            // multiple matches found for the given string
            Com_Printf("Multiple clients found matching %s:\n", s);

            for (i = 0; i < count; i++) {
                cl = matches[i];
                strcpy(name, cl->name);
                Com_Printf(" %2d: [%s]\n", (int)(cl - svs.clients), name);
            }

            return NULL;
        }

        // found just 1 match
        return matches[0];

    }

}

/////////////////////////////////////////////////////////////////////
// Name        : SV_SortMaps
// Description : Array sorting comparison function (for qsort)
// Author      : Fenix
/////////////////////////////////////////////////////////////////////
static int QDECL SV_SortMaps(const void *a, const void *b) {
    return strcmp (*(const char **) a, *(const char **) b);
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_GetMapSoundingLike
// Description : Retrieve a full map name given a substring of it
// Author      : Fenix
/////////////////////////////////////////////////////////////////////
void SV_GetMapSoundingLike(char *dest, const char *s, int size) {

    int  i;
    int  len = 0;
    int  count = 0;
    int  mapcount;
    char *search;
    char *matches[MAX_MAPLIST_SIZE];
    char expanded[MAX_QPATH];
    static char maplist[MAX_MAPLIST_STRING];

    Com_sprintf(expanded, sizeof(expanded), "maps/%s.bsp", s);
    if (FS_ReadFile(expanded, NULL) > 0) {
        Q_strncpyz(dest, s, size);
        return;
    }

    // we didn't found an exact name match. Keep iterating
    // through all the available maps matching partial substrings
    if (!(mapcount = FS_GetFileList("maps", ".bsp", maplist, sizeof(maplist)))) {
        Com_Printf("ERROR: Could not retrieve maplist!\n");
        *dest = 0;
        return;
    }

    search = maplist;
    for (i = 0; i < mapcount && count < MAX_MAPLIST_SIZE; i++, search += len + 1) {

        len = (int) strlen(search);
        COM_StripExtension(search, search);

        // check for substring match
        if (Q_strisub(search, s)) {
            matches[count] = search;
            count++;
        }

    }

    if (count == 0) {
        Com_Printf("Could not find any map matching %s%s%s\n", S_COLOR_YELLOW, s, S_COLOR_WHITE);
        *dest = 0;
        return;
    }

    if (count > 1) {

        Com_Printf("Maps found matching %s%s%s:\n", S_COLOR_YELLOW, s, S_COLOR_WHITE);

        // Sort maps before displaying the short list
        qsort(matches, (size_t) count, sizeof(char *), SV_SortMaps);

        for (i = 0; i < count; i++) {
            // Printing a short map list so the user can retry with a more specific name
            Com_Printf(" %2d: [%s]\n", i + 1, matches[i]);
        }

        if (count > MAX_MAPLIST_SIZE) {
            // Tell the user that there are actually more
            // maps matching the given substring, although
            // we are not displaying them....
            Com_Printf("...and more\n");
        }

        *dest = 0;
        return;

    }

    Q_strncpyz(dest, matches[0], size);

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                          //
//  COMMANDS                                                                                                //
//                                                                                                          //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// Name        : SV_Map_f
// Description : Restart the server on a different map
/////////////////////////////////////////////////////////////////////
static void SV_Map_f(void) {

    char      *cmd;
    char      mapname[MAX_QPATH];
    qboolean  killBots, cheat;

    if (Cmd_Argc() < 2) {
        return;
    }

    SV_GetMapSoundingLike(mapname, Cmd_Argv(1), sizeof(mapname));
    if (!mapname[0]) {
        return;
    }

    // force latched values to get set
    Cvar_Get("g_gametype", "0", CVAR_SERVERINFO | CVAR_USERINFO | CVAR_LATCH);

    cmd = Cmd_Argv(0);
    if (Q_stricmpn(cmd, "sp", 2) == 0) {
        Cvar_SetValue("g_gametype", GT_SINGLE_PLAYER);
        Cvar_SetValue("g_doWarmup", 0);
        // may not set sv_maxclients directly, always set latched
        Cvar_SetLatched("sv_maxclients", "8");
        cheat = qfalse;
        killBots = qtrue;
    }
    else {

        if (!Q_stricmp(cmd, "devmap") || !Q_stricmp(cmd, "spdevmap")) {
            cheat = qtrue;
            killBots = qtrue;
        } else {
            cheat = qfalse;
            killBots = qfalse;
        }

        if (sv_gametype->integer == GT_SINGLE_PLAYER) {
            Cvar_SetValue("g_gametype", GT_FFA);
        }

    }
    
    // start up the map
    SV_SpawnServer(mapname, killBots);

    // set the cheat value
    // if the level was started with "map <levelname>", then
    // cheats will not be allowed.  If started with "devmap <levelname>"
    // then cheats will be allowed
    if (cheat) {
        Cvar_Set("sv_cheats", "1");
    } else {
        Cvar_Set("sv_cheats", "0");
    }

}

/////////////////////////////////////////////////////////////////////
// Name        : SV_MapRestart_f
// Description : Completely restarts a level, but doesn't send a new 
//               gamestate to the clients. This allows fair starts 
//               with variable load times.
/////////////////////////////////////////////////////////////////////
static void SV_MapRestart_f(void) {
    
    int         i;
    int         delay;
    client_t    *client;
    char        *denied;
    qboolean    isBot;

    // make sure we aren't restarting twice in the same frame
    if (com_frameTime == sv.serverId) {
        return;
    }

    // make sure server is running
    if (!com_sv_running->integer) {
        Com_Printf("Server is not running\n");
        return;
    }

    if (sv.restartTime) {
        return;
    }

    if (Cmd_Argc() > 1) {
        delay = atoi(Cmd_Argv(1));
    } else {
        delay = 5;
    }
    
    if (delay && !Cvar_VariableValue("g_doWarmup")) {
        sv.restartTime = sv.time + delay * 1000;
        SV_SetConfigstring(CS_WARMUP, va("%i", sv.restartTime));
        return;
    }

    // check for changes in variables that can't just be restarted
    // check for maxclients change
    if (sv_maxclients->modified || sv_gametype->modified) {
        char mapname[MAX_QPATH];
        Com_Printf("variable change -- restarting\n");
        // restart the map the slow way
        Q_strncpyz(mapname, Cvar_VariableString("mapname"), sizeof(mapname));
        SV_SpawnServer(mapname, qfalse);
        return;
    }

    // toggle the server bit so clients can detect that a
    // map_restart has happened
    svs.snapFlagServerBit ^= SNAPFLAG_SERVERCOUNT;

    // generate a new serverid    
    // TTimo - don't update restartedserverId there, otherwise we won't deal correctly with multiple
    // map_restart
    sv.serverId = com_frameTime;
    Cvar_Set("sv_serverid", va("%i", sv.serverId));

    // if a map_restart occurs while a client is changing maps, we need
    // to give them the correct time so that when they finish loading
    // they don't violate the backwards time check in cl_cgame.c
    for (i = 0; i < sv_maxclients->integer; i++) {
        if (svs.clients[i].state == CS_PRIMED) {
            svs.clients[i].oldServerTime = sv.restartTime;
        }
    }

    // reset all the vm data in place without changing memory allocation
    // note that we do NOT set sv.state = SS_LOADING, so configstrings that
    // had been changed from their default values will generate broadcast updates
    sv.state = SS_LOADING;
    sv.restarting = qtrue;

    SV_RestartGameProgs();

    // run a few frames to allow everything to settle
    for (i = 0; i < 3; i++) {
        VM_Call (gvm, GAME_RUN_FRAME, sv.time);
        sv.time += 100;
        svs.time += 100;
    }

    sv.state = SS_GAME;
    sv.restarting = qfalse;

    // connect and begin all the clients
    for (i = 0 ; i < sv_maxclients->integer; i++) {

        client = &svs.clients[i];

        // send the new gamestate to all connected clients
        if (client->state < CS_CONNECTED) {
            continue;
        }

        if (client->netchan.remoteAddress.type == NA_BOT) {
            isBot = qtrue;
        } else {
            isBot = qfalse;
        }

        // add the map_restart command
        SV_AddServerCommand(client, "map_restart\n");

        // connect the client again, without the firstTime flag
        denied = VM_ExplicitArgPtr(gvm, VM_Call(gvm, GAME_CLIENT_CONNECT, i, qfalse, isBot));
        if (denied) {
            // this generally shouldn't happen, because the client
            // was connected before the level change
            SV_DropClient(client, denied);
            Com_Printf("SV_MapRestart_f(%d): dropped client %i - denied!\n", delay, i);
            continue;
        }

        if (client->state == CS_ACTIVE) {
            SV_ClientEnterWorld(client, &client->lastUsercmd);
        } else {
            // If we don't reset client->lastUsercmd and are restarting during map load,
             // the client will hang because we'll use the last Usercmd from the previous map,
             // which is wrong obviously.
             SV_ClientEnterWorld(client, NULL);
        }
    }    

    // run another frame to allow things to look at all the players
    VM_Call (gvm, GAME_RUN_FRAME, sv.time);
    sv.time += 100;
    svs.time += 100;
    
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_Kick_f
// Description : Kick a user off of the server
/////////////////////////////////////////////////////////////////////
static void SV_Kick_f(void) {

    int         i;
    client_t    *cl;
    char        *reason = "was kicked";

    // make sure server is running
    if (!com_sv_running->integer) {
        Com_Printf("Server is not running\n");
        return;
    }

    if ((Cmd_Argc() < 2) || (Cmd_Argc() > 3)) {
        Com_Printf("Usage: kick <client> [<reason>]\n"
                   "       kick all [<reason>] = kick everyone\n"
                   "       kick allbots = kick all bots\n");
        return;
    }

    if (Cmd_Argc() == 3) {
        // If the reason was specified attach it to the disconnect message
        reason = va("was kicked: %s", Cmd_Argv(2));
    }

    if (!Q_stricmp(Cmd_Argv(1), "all")) {

        for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {

            if (!cl->state) {
                continue;
            }

            if (cl->netchan.remoteAddress.type == NA_LOOPBACK) {
                continue;
            }

            SV_DropClient(cl, reason);
            cl->lastPacketTime = svs.time;
        }

    } else if (!Q_stricmp(Cmd_Argv(1), "allbots")) {

        for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {

            if (!cl->state) {
                continue;
            }

            if (cl->netchan.remoteAddress.type != NA_BOT) {
                continue;
            }

            SV_DropClient(cl, reason);
            cl->lastPacketTime = svs.time;

        }

    } else {

        cl = SV_GetPlayerByHandle();

        if (!cl) {
            return;
        }

        if (cl->netchan.remoteAddress.type == NA_LOOPBACK) {
            SV_BroadcastMessageToClient(NULL, "Cannot kick host client");
            return;
        }

        SV_DropClient(cl, reason);
        cl->lastPacketTime = svs.time;

    }

}

/////////////////////////////////////////////////////////////////////
// Name        : SV_Teleport_f
// Description : Teleport a player
// Author      : Fenix
/////////////////////////////////////////////////////////////////////
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuninitialized"
static void SV_Teleport_f(void) {
    
    vec3_t          origin;
    client_t        *cl1, *cl2 = NULL;
    playerState_t   *ps1, *ps2;
    
    // make sure server is running
    if (!com_sv_running->integer) {
        Com_Printf("Server is not running\n");
        return;
    }
    
    // if we are not playing jump mode
    if (sv_gametype->integer != GT_JUMP) {
        return;
    }
    
    // check for correct number of arguments
    if ((Cmd_Argc() != 3) && (Cmd_Argc() != 5)) {
        Com_Printf("Usage: teleport <client> <target>\n"
                   "       teleport <client> <x> <y> <z>\n");
        return;
    }
    
    // get the source client
    cl1 = SV_GetPlayerByHandle();
    if (!cl1) {
        return;
    }
    
    // if in a jumprun
    if (cl1->ready) {
        return;
    }
    
    // get the client playerstate
    ps1 = SV_GameClientNum((int) (cl1 - svs.clients));
    
    if (Cmd_Argc() == 3) {
        
        // get the target client
        cl2 = SV_GetPlayerByParam(Cmd_Argv(2));
        if (!cl2) { 
            return;
        }
        
        // if in a jumprun
        if (cl2->ready) {
            SV_BroadcastMessageToClient(cl1, "%s is currently doing a jump run", cl2->name);
            return;
        }
        
        // get target client world coordinates
        ps2 = SV_GameClientNum((int) (cl2 - svs.clients));
        origin[0] = ps2->origin[0];
        origin[1] = ps2->origin[1];
        origin[2] = ps2->origin[2];

    } else {
        // copy given world coordinates
        origin[0] = (vec_t) atof(Cmd_Argv(2));
        origin[1] = (vec_t) atof(Cmd_Argv(3));
        origin[2] = (vec_t) atof(Cmd_Argv(4));
    }
    
    // teleport the player
    VectorCopy(origin, ps1->origin);
    VectorClear(ps1->velocity);
    
    SV_BroadcastMessageToClient(cl1, "You have been successfully teleported");
    
    if (cl2 != NULL) {
        // inform also target client
        SV_BroadcastMessageToClient(cl2, "%s has been teleported to you", cl1->name);
    }

}
#pragma clang diagnostic pop

/////////////////////////////////////////////////////////////////////
// Name        : SV_Position_f
// Description : Retrieve player positions
// Author      : Fenix
/////////////////////////////////////////////////////////////////////
static void SV_Position_f(void) {
    
    int             i;
    client_t        *cl;
    playerState_t   *ps;
    
    // make sure server is running
    if (!com_sv_running->integer) {
        Com_Printf("Server is not running\n");
        return;
    }
    
    if (Cmd_Argc() != 2) {
        Com_Printf("Usage: position <client-or-all>\n");
        return;
    }
    
    if (!Q_stricmp(Cmd_Argv(1), "all")) {
        
        for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
            
            // if not connected
            if (!cl->state) {
                continue;
            }
            
            // print the position
            ps = SV_GameClientNum(i);
            Com_Printf("%3i - %12f - %12f - %12f\n", i, ps->origin[0], ps->origin[1], ps->origin[2]);
            
        }
        
    } else {
        
        // search the client
        cl = SV_GetPlayerByHandle();
        if (!cl) {
            return;
        }
        
        // print the position
        i = (int) (cl - svs.clients);
        ps = SV_GameClientNum(i);
        Com_Printf("%3i - %12f - %12f - %12f\n", i, ps->origin[0], ps->origin[1], ps->origin[2]);
        
    }

}

/////////////////////////////////////////////////////////////////////
// Name        : SV_SendClientCommand_f
// Description : Send a reliable command as a specific client
// Author      : Fenix
/////////////////////////////////////////////////////////////////////
static void SV_SendClientCommand_f(void) {
    
    char      *cmd;
    client_t  *cl;
    
    // make sure server is running
    if (!com_sv_running->integer) {
        Com_Printf("Server is not running\n");
        return;
    }
    
    // check for correct parameters
    if (Cmd_Argc() < 3 || !strlen(Cmd_Argv(2))) {
        Com_Printf("Usage: sendclientcommand <client> <command>\n"
                   "       sendclientcommand all <command> = send to everyone\n");
        return;
    }
    
    // get the command
    cmd = Cmd_ArgsFromRaw(2);
    
    if (!Q_stricmp(Cmd_Argv(1), "all")) {
        
        // send to everyone
        SV_SendServerCommand(NULL, "%s", cmd);
        
    } else {
        
        // search the client
        cl = SV_GetPlayerByHandle();
        if (!cl) {
            return;
        }
        
        // send the command to the client
        SV_SendServerCommand(cl, "%s", cmd);
        
    }

}

/////////////////////////////////////////////////////////////////////
// Name        : SV_Spoof_f
// Description : Send a game client command as a specific client
// Author      : Fenix
/////////////////////////////////////////////////////////////////////
static void SV_Spoof_f(void) {
    
    char      *cmd;
    client_t  *cl;
    
    // make sure server is running
    if (!com_sv_running->integer) {
        Com_Printf("Server is not running\n");
        return;
    }
    
    // check for correct parameters
    if (Cmd_Argc() < 3 || !strlen(Cmd_Argv(2))) {
        Com_Printf("Usage: spoof <client> <command>\n");
        return;
    }
    
    // search the client
    cl = SV_GetPlayerByHandle();
    if (!cl) {
        return;
    }
    
    // get the command
    cmd = Cmd_ArgsFromRaw(2);
    Cmd_TokenizeString(cmd);
    
    // send the command
    VM_Call(gvm, GAME_CLIENT_COMMAND, cl - svs.clients);

}

/////////////////////////////////////////////////////////////////////
// Name        : SV_ForceCvar_f_helper
// Description : Set a CVAR for a user
/////////////////////////////////////////////////////////////////////
static void SV_ForceCvar_f_helper(client_t *cl) {
    
    int   ret;

    // if the dude is not connected
    if (cl->state < CS_CONNECTED) {
        return;
    }

    // we already check that Cmd_Argv(2) has nonzero length
    // if Cmd_Argv(3) has zero length, the key will just be removed
    ret = Info_SetValueForKey(cl->userinfo, Cmd_Argv(2), Cmd_Argv(3));
    if (ret > 0) {
        // Fenix: previously this was dropping the client
        // I removed the client dropping since it makes no sense: removing
        // the cvar value form the infostring is enough (if the infostring
        // was good before our add, it will be also after the removal)
        Info_SetValueForKey(cl->userinfo, Cmd_Argv(2), "");
        return;  
    }
    
    if (ret < 0) {
        // the admin already saw the error message
        // for illegal characters so just exit
        return;
    }
    
    SV_UserinfoChanged(cl);

    // call prog code to allow overrides
    VM_Call(gvm, GAME_CLIENT_USERINFO_CHANGED, cl - svs.clients);
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_ForceCvar_f
// Description : Set a CVAR for a user
/////////////////////////////////////////////////////////////////////
static void SV_ForceCvar_f(void) {
    
    int       i;
    client_t  *cl;
    
    // make sure server is running
    if (!com_sv_running->integer) {
        Com_Printf("Server is not running\n");
        return;
    }

    if (Cmd_Argc() != 4 || strlen(Cmd_Argv(2)) == 0) {
        Com_Printf("Usage: forcecvar <client> <cvar> <value>\n"
                   "       forcecvar allbots <cvar> <value> = force for all the bots\n"
                   "       forcecvar all <cvar> <value> = force for everyone\n");
        return;
    }
    
    if (!Q_stricmp(Cmd_Argv(1), "all")) {
        
        for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
            
            // if not connected
            if (!cl->state) {
                continue;
            }
            
            // call internal helper
            SV_ForceCvar_f_helper(cl);
            
        }
        
    } else if (!Q_stricmp(Cmd_Argv(1), "allbots")) {
        
        for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
            
            // if not connected
            if (!cl->state) {
                continue;
            }
            
            // if the dude is not a bot
            if (cl->netchan.remoteAddress.type != NA_BOT) {
                continue;
            }
            
            // call internal helper
            SV_ForceCvar_f_helper(cl);
            
        }
        
    } else {
        
        // search the client
        cl = SV_GetPlayerByHandle();
        
        if (!cl) {
            return;
        }
        
        // call internal helper
        SV_ForceCvar_f_helper(cl);
    
    }
    
}

/**
 * SV_Status_f
 * 
 * @description Print server status informations
 */
static void SV_Status_f(void) {

    int            i, j, l;
    client_t       *cl;
    playerState_t  *ps;
    const char     *s;
    int            ping;
    char           name[MAX_NAME_LENGTH];
    
    // make sure server is running
    if (!com_sv_running->integer) {
        Com_Printf("Server is not running\n");
        return;
    }

    Com_Printf("map: %s\n", sv_mapname->string);
    Com_Printf("num score ping name            lastmsg address               qport rate\n");
    Com_Printf("--- ----- ---- --------------- ------- --------------------- ----- -----\n");

    for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {

        if (!cl->state) {
            continue;
        }

        Com_Printf("%3i ", i);
        ps = SV_GameClientNum(i);
        Com_Printf("%5i ", ps->persistant[PERS_SCORE]);

        if (cl->state == CS_CONNECTED) {
            Com_Printf("CNCT ");
        } else if (cl->state == CS_ZOMBIE) {
            Com_Printf("ZMBI ");
        } else {
            ping = cl->ping < 9999 ? cl->ping : 9999;
            Com_Printf("%4i ", ping);
        }
        
        // copy the name locally and remove color codes
        // otherwise they will break the padding 
        Q_strncpyz(name, cl->name, sizeof(name));
        Q_CleanStr(name);
        
        Com_Printf("%s", name);
        
        Com_Printf("^7");
        l = (int) (16 - strlen(name));
        for (j=0 ; j<l ; j++) {
            Com_Printf(" ");
        }

        Com_Printf("%7i ", svs.time - cl->lastPacketTime);
        s = NET_AdrToString(cl->netchan.remoteAddress);
        Com_Printf("%s", s);
        l = (int) (22 - strlen(s));
        for (j = 0; j < l; j++) {
            Com_Printf(" ");
        }

        Com_Printf("%5i", cl->netchan.qport);
        Com_Printf(" %5i", cl->rate);
        Com_Printf("\n");
    }

    Com_Printf("\n");

}

/////////////////////////////////////////////////////////////////////
// Name        : SV_ConSay_f
// Description : Print a message in the chat area
/////////////////////////////////////////////////////////////////////
static void SV_ConSay_f(void) {

    char    *p;
    char    text[1024];

    // make sure server is running
    if (!com_sv_running->integer) {
        Com_Printf("Server is not running\n");
        return;
    }

    if (Cmd_Argc () < 2) {
        return;
    }

    strcpy(text, sv_sayPrefix->string);
    p = Cmd_Args();

    if (*p == '"') {
        p++;
        p[strlen(p) - 1] = 0;
    }

    strcat(text, p);
    SV_SendServerCommand(NULL, "chat \"%s\n\"", text);

}

/////////////////////////////////////////////////////////////////////
// Name        : SV_ConTell_f
// Description : Send a private message to a specific client
/////////////////////////////////////////////////////////////////////
static void SV_ConTell_f(void) {

    client_t  *cl;
    char      *p;
    char      text[1024];

    // make sure server is running
    if (!com_sv_running->integer) {
        Com_Printf("Server is not running\n");
        return;
    }

    if (Cmd_Argc() < 3) {
        Com_Printf("Usage: tell <client> <text>\n");
        return;
    }

    cl = SV_GetPlayerByHandle();

    if (!cl) {
        return;
    }

    strcpy (text, sv_tellPrefix->string);
    p = Cmd_ArgsFrom(2);

    if (*p == '"') {
        p++;
        p[strlen(p) - 1] = 0;
    }

    strcat(text, p);
    SV_SendServerCommand(cl, "chat \"%s\n\"", text);

}

/////////////////////////////////////////////////////////////////////
// Name        : SV_Heartbeat_f
// Description : Send a hearthbeat to the master server. Also called 
//               by SV_DropClient, SV_DirectConnect, SV_SpawnServer
/////////////////////////////////////////////////////////////////////
void SV_Heartbeat_f(void) {
    svs.nextHeartbeatTime = -9999999;
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_Serverinfo_f
// Description : Examine the serverinfo string
/////////////////////////////////////////////////////////////////////
static void SV_Serverinfo_f(void) {
    Com_Printf("Server info settings:\n");
    Info_Print(Cvar_InfoString(CVAR_SERVERINFO));
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_Systeminfo_f
// Description : Examine the systeminfo string
/////////////////////////////////////////////////////////////////////
static void SV_Systeminfo_f(void) {
    Com_Printf("System info settings:\n");
    Info_Print(Cvar_InfoString(CVAR_SYSTEMINFO));
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_DumpUser_f
// Description : Examine a client userinfo string
/////////////////////////////////////////////////////////////////////
static void SV_DumpUser_f(void) {

    client_t *cl;

    // make sure server is running
    if (!com_sv_running->integer) {
        Com_Printf("Server is not running\n");
        return;
    }

    if (Cmd_Argc() != 2) {
        Com_Printf("Usage: dumpuser <client>\n");
        return;
    }

    cl = SV_GetPlayerByHandle();
    if (!cl) {
        return;
    }

    Com_Printf("userinfo\n");
    Com_Printf("--------\n");
    Info_Print(cl->userinfo);

}

/////////////////////////////////////////////////////////////////////
// Name        : SV_KillServer_f
// Description : Shutdown the server
/////////////////////////////////////////////////////////////////////
static void SV_KillServer_f(void) {
    SV_Shutdown("killserver");
}

/////////////////////////////////////////////////////////////////////
// Name        : SVD_StartDemoFile
// Description : Start a server-side demo. This does it all, create 
//               the file and adjust the demo-related stuff in 
//               client_t. This is mostly ripped from 
//               sv_client.c/SV_SendClientGameState and 
//               cl_main.c/CL_Record_f.
/////////////////////////////////////////////////////////////////////
static void SVD_StartDemoFile(client_t *client, const char *path) {

    int             i, len;
    entityState_t   *base, nullstate;
    msg_t           msg;
    byte            buffer[MAX_MSGLEN];
    fileHandle_t    file;
    
    #ifdef USE_DEMO_FORMAT_42
    char            *s;
    int             v, size;
    #endif

    Com_DPrintf("SVD_StartDemoFile\n");
    assert(!client->demo_recording);

    // create the demo file and write the necessary header
    file = FS_FOpenFileWrite(path);
    assert(file != 0);

    /* File_write_header_demo // ADD this fx */
    /* HOLBLIN  entete demo */
    #ifdef USE_DEMO_FORMAT_42
        //@Barbatos: get the mod version from the server
        s = Cvar_VariableString("g_modversion");

        size = strlen(s);
        len = LittleLong(size);
        FS_Write(&len, 4, file);
        FS_Write(s, size, file);

        v = LittleLong(DEMO_VERSION);
        FS_Write (&v, 4, file);

        len = 0;
        len = LittleLong(len);
        FS_Write(&len, 4, file);
        FS_Write(&len, 4, file);
    #endif
    /* END HOLBLIN  entete demo */

    MSG_Init(&msg, buffer, sizeof(buffer));
    MSG_Bitstream(&msg);                            // XXX server code doesn't do this, client code does
    MSG_WriteLong(&msg, client->lastClientCommand); // TODO: or is it client->reliableSequence?
    MSG_WriteByte(&msg, svc_gamestate);
    MSG_WriteLong(&msg, client->reliableSequence);

    for (i = 0; i < MAX_CONFIGSTRINGS; i++) {
        if (sv.configstrings[i][0]) {
            MSG_WriteByte(&msg, svc_configstring);
            MSG_WriteShort(&msg, i);
            MSG_WriteBigString(&msg, sv.configstrings[i]);
        }
    }

    Com_Memset(&nullstate, 0, sizeof(nullstate));
    for (i = 0 ; i < MAX_GENTITIES; i++) {
        base = &sv.svEntities[i].baseline;
        if (!base->number) {
            continue;
        }
        MSG_WriteByte(&msg, svc_baseline);
        MSG_WriteDeltaEntity(&msg, &nullstate, base, qtrue);
    }

    MSG_WriteByte(&msg, svc_EOF);
    MSG_WriteLong(&msg, (int) (client - svs.clients));
    MSG_WriteLong(&msg, sv.checksumFeed);
    MSG_WriteByte(&msg, svc_EOF);                   // XXX server code doesn't do this, 
                                                    // SV_Netchan_Transmit adds it!

    len = LittleLong(client->netchan.outgoingSequence - 1);
    FS_Write(&len, 4, file);

    len = LittleLong (msg.cursize);
    FS_Write(&len, 4, file);
    FS_Write(msg.data, msg.cursize, file);

    #ifdef USE_DEMO_FORMAT_42
    // add size of packet in the end for backward play /* holblin */
    FS_Write(&len, 4, file);
    #endif

    FS_Flush(file);

    // adjust client_t to reflect demo started
    client->demo_recording = qtrue;
    client->demo_file = file;
    client->demo_waiting = qtrue;
    client->demo_backoff = 1;
    client->demo_deltas = 0;
}

/////////////////////////////////////////////////////////////////////
// Name        : SVD_WriteDemoFile
// Description : Write a message to a server-side demo file
/////////////////////////////////////////////////////////////////////
void SVD_WriteDemoFile(const client_t *client, const msg_t *msg) {

    int len;
    msg_t cmsg;
    byte cbuf[MAX_MSGLEN];
    fileHandle_t file = client->demo_file;

    if (*(int *)msg->data == -1) {
        Com_DPrintf("Ignored connectionless packet, not written to demo!\n");
        return;
    }

    // TODO: we only copy because we want to add svc_EOF; can we add it and then
    // "back off" from it, thus avoiding the copy?
    MSG_Copy(&cmsg, cbuf, sizeof(cbuf), (msg_t*) msg);
    MSG_WriteByte(&cmsg, svc_EOF); // XXX server code doesn't do this, SV_Netchan_Transmit adds it!

    // TODO: the headerbytes stuff done in the client seems unnecessary
    // here because we get the packet *before* the netchan has it's way
    // with it; just not sure that's really true :-/
    len = LittleLong(client->netchan.outgoingSequence);
    FS_Write(&len, 4, file);

    len = LittleLong(cmsg.cursize);
    FS_Write(&len, 4, file);
    FS_Write(cmsg.data, cmsg.cursize, file); // XXX don't use len!

    #ifdef USE_DEMO_FORMAT_42
    // add size of packet in the end for backward play /* holblin */
    FS_Write(&len, 4, file);
    #endif

    FS_Flush(file);
}

/////////////////////////////////////////////////////////////////////
// Name        : SVD_StopDemoFile
// Description : Stop a server-side demo. This finishes out the file 
//               and clears the demo-related stuff in client_t again
/////////////////////////////////////////////////////////////////////
static void SVD_StopDemoFile(client_t *client) {

    int marker = -1;
    fileHandle_t file = client->demo_file;

    Com_DPrintf("SVD_StopDemoFile\n");
    assert(client->demo_recording);

    // write the necessary trailer and close the demo file
    FS_Write(&marker, 4, file);
    FS_Write(&marker, 4, file);
    FS_Flush(file);
    FS_FCloseFile(file);

    // adjust client_t to reflect demo stopped
    client->demo_recording = qfalse;
    client->demo_file = -1;
    client->demo_waiting = qfalse;
    client->demo_backoff = 1;
    client->demo_deltas = 0;
}

/////////////////////////////////////////////////////////////////////
// Name        : SVD_CleanPlayerName
// Description : Clean up player name to be suitable as path name
//               Similar to Q_CleanStr() but tweaked
/////////////////////////////////////////////////////////////////////
static void SVD_CleanPlayerName(char *name) {

    char *src = name, *dst = name, c;

    while ((c = *src)) {
        // note Q_IsColorString(src++) won't work since it's a macro
        if (Q_IsColorString(src)) {
            src++;
        } else if (c == ':' || c == '\\' || c == '/' || c == '*' || c == '?') {
            *dst++ = '%';
        } else if (c > ' ' && c < 0x7f) {
            *dst++ = c;
        }
        src++;
    }

    *dst = '\0';
    if (strlen(name) == 0) {
        strcpy(name, "UnnamedPlayer");
    }
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_NameServerDemo
// Description : Generate unique name for a new server demo file.
//               We pretend there are no race conditions.
/////////////////////////////////////////////////////////////////////
static void SV_NameServerDemo(char *filename, int length, const client_t *client, char *fn) {

    qtime_t time;
    char demoName[64];
    char playername[32];
    
    Com_DPrintf("SV_NameServerDemo\n");

    Com_RealTime(&time);
    Q_strncpyz(playername, client->name, sizeof(playername));
    SVD_CleanPlayerName(playername);

    if (fn != NULL) {

        Q_strncpyz(demoName, fn, sizeof(demoName));

        #ifdef USE_DEMO_FORMAT_42
        Q_snprintf(filename, length - 1, "%s/%s.urtdemo", sv_demoFolder->string, demoName);
        if (FS_FileExists(filename)) {
            Q_snprintf(filename, length - 1, "%s/%s_%d.urtdemo", 
                       sv_demoFolder->string, demoName, Sys_Milliseconds());
        }
        #else
        Q_snprintf(filename, length - 1, "%s/%s.dm_%d", sv_demoFolder->string, demoName , PROTOCOL_VERSION);
        if (FS_FileExists(filename)) {
            Q_snprintf(filename, length - 1, "%s/%s_%d.dm_%d", sv_demoFolder->string,
                       demoName, Sys_Milliseconds(), PROTOCOL_VERSION);
        }
        #endif
    
    } else {
        
        #ifdef USE_DEMO_FORMAT_42
        Q_snprintf(filename, length - 1, "%s/%.4d-%.2d-%.2d_%.2d-%.2d-%.2d_%s_%d.urtdemo",
                                         sv_demoFolder->string, time.tm_year+1900, time.tm_mon + 1,
                                         time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec,
                                         playername, Sys_Milliseconds()
        );
        #else
        Q_snprintf(filename, length - 1, "%s/%.4d-%.2d-%.2d_%.2d-%.2d-%.2d_%s_%d.dm_%d",
                                         sv_demoFolder->string, time.tm_year+1900, time.tm_mon + 1,
                                         time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec,
                                         playername, Sys_Milliseconds(), PROTOCOL_VERSION
        );
        #endif
                
        filename[length-1] = '\0';

        if (FS_FileExists(filename)) {
            filename[0] = 0;
            return;
        }
    }
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_StartRecordOne
// Description : Start recording a server-side demo of a client
/////////////////////////////////////////////////////////////////////
static void SV_StartRecordOne(client_t *client, char *filename) {

    char path[MAX_OSPATH];

    Com_DPrintf("SV_StartRecordOne\n");

    if (client->demo_recording) {
        Com_Printf("startserverdemo: %s is already being recorded\n", client->name);
        return;
    }

    if (client->state != CS_ACTIVE) {
        Com_Printf("startserverdemo: %s is not active\n", client->name);
        return;
    }

    if (client->netchan.remoteAddress.type == NA_BOT) {
        Com_Printf("startserverdemo: %s is a bot\n", client->name);
        return;
    }

    SV_NameServerDemo(path, sizeof(path), client, filename);
    SVD_StartDemoFile(client, path);

    if (sv_demoNotice->string) {
        SV_BroadcastMessageToClient(client, sv_demoNotice->string);
    }

    Com_Printf("startserverdemo: recording %s to %s\n", client->name, path);
    
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_StartRecordOne
// Description : Start recording a server-side demo of all the 
//               connected clients
/////////////////////////////////////////////////////////////////////
static void SV_StartRecordAll(void) {

    int slot;
    client_t *client;

    Com_DPrintf("SV_StartRecordAll\n");

    for (slot = 0, client = svs.clients; slot < sv_maxclients->integer; slot++, client++) {
        // filter here to avoid lots of bogus messages from SV_StartRecordOne()
        if (client->netchan.remoteAddress.type == NA_BOT || 
            client->state != CS_ACTIVE || 
            client->demo_recording) {
            continue;
        }
        SV_StartRecordOne(client, NULL);
    }
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_StopRecordOne
// Description : Stop recording a server-side demo of a client
/////////////////////////////////////////////////////////////////////
static void SV_StopRecordOne(client_t *client) {

    Com_DPrintf("SV_StopRecordOne\n");

    if (!client->demo_recording) {
        Com_Printf("stopserverdemo: %s is not being recorded\n", client->name);
        return;
    }

    if (client->state != CS_ACTIVE) { // disconnects are handled elsewhere
        Com_Printf("stopserverdemo: %s is not active\n", client->name);
        return;
    }

    if (client->netchan.remoteAddress.type == NA_BOT) {
        Com_Printf("stopserverdemo: %s is a bot\n", client->name);
        return;
    }

    SVD_StopDemoFile(client);
    Com_Printf("stopserverdemo: stopped recording %s\n", client->name);


}

/////////////////////////////////////////////////////////////////////
// Name        : SV_StartRecordOne
// Description : Stop recording a server-side demo for of the 
//               connected clients
/////////////////////////////////////////////////////////////////////
static void SV_StopRecordAll(void) {

    int slot;
    client_t *client;

    Com_DPrintf("SV_StopRecordAll\n");

    for (slot=0, client=svs.clients; slot < sv_maxclients->integer; slot++, client++) {
        // filter here to avoid lots of bogus messages from SV_StopRecordOne()
        if (client->netchan.remoteAddress.type == NA_BOT || 
            client->state != CS_ACTIVE || // disconnects are handled elsewhere
            !client->demo_recording) {
            continue;
        }
        SV_StopRecordOne(client);
    }

}

/////////////////////////////////////////////////////////////////////
// Name        : SV_StartServerDemo_f
// Description : Record a server-side demo for given player/slot. 
//               The demo will be called 
//               "YYYY-MM-DD_hh-mm-ss_playername_id.urtdemo", in the 
//               "demos" directory under your game directory. Note
//               that "startserverdemo all" will start demos for all 
//               players currently in the server. Players who join 
//               later require a new "startserverdemo" command. If 
//               you are already recording demos for some players, 
//               another "startserverdemo all" will start new demos 
//               only for  players not already recording. Note that 
//               bots will  never be recorded.
//               The server-side demos will stop when  
//               "stopserverdemo" is issued or when the server 
//               restarts for any reason  (such as a new map loading)
/////////////////////////////////////////////////////////////////////
static void SV_StartServerDemo_f(void) {

    client_t *client;

    Com_DPrintf("SV_StartServerDemo_f\n");

    if (!com_sv_running->integer) {
        Com_Printf("startserverdemo: server not running\n");
        return;
    }

    if (Cmd_Argc() < 2) {
        Com_Printf("Usage: startserverdemo <client> [<optional-demo-name>]\n"
                   "       startserverdemo all = start recording all the connected players\n");
        return;
    }

    if (!Q_stricmp(Cmd_Argv(1), "all")) {
        SV_StartRecordAll();
    } else {

        client = SV_GetPlayerByHandle();
        if (!client) {
            return;
        }

        if (Cmd_Argc() > 2) {
            SV_StartRecordOne(client, Cmd_ArgsFrom(2));
        } else {
            SV_StartRecordOne(client, NULL);
        }

    }
    
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_StopServerDemo_f
// Description : Stop a server-side demo for given player/slot. Note 
//               that "stopserverdemo all" will stop demos for all 
//               players in the server.
/////////////////////////////////////////////////////////////////////
static void SV_StopServerDemo_f(void) {
    
    client_t *client;

    Com_DPrintf("SV_StopServerDemo_f\n");

    if (!com_sv_running->integer) {
        Com_Printf("stopserverdemo: server not running\n");
        return;
    }

    if (Cmd_Argc() != 2) {
        Com_Printf("Usage: stopserverdemo <client-or-all>\n");
        return;
    }

    if (!Q_stricmp(Cmd_Argv(1), "all")) {
        SV_StopRecordAll();
    } else {

        client = SV_GetPlayerByHandle();

        if (!client) {
            return;
        }

        SV_StopRecordOne(client);
    }

}

#ifdef USE_AUTH
/////////////////////////////////////////////////////////////////////
// Name        : SV_Auth_Whois_f
// Description : Get user auth info
// Author      : Barbatos, Kalish
/////////////////////////////////////////////////////////////////////
static void SV_Auth_Whois_f(void) {

    int         i;
    client_t    *cl;

    // make sure server is running
    if (!com_sv_running->integer) {
        Com_Printf("Server is not running\n");
        return;
    }

    if (Cmd_Argc() < 2) {
        Com_Printf("Usage: auth-whois <client-or-all>\n");
        return;
    }

    if (Cvar_VariableValue("auth") == 0) {
        Com_Printf("Auth services are disabled\n");
        return;
    }

    if (!Q_stricmp(Cmd_Argv(1), "all")) {

        for (i = 0; i < sv_maxclients->integer; i++) {

            cl = &svs.clients[i];

            if (cl->state != CS_ACTIVE) {
                continue;
            }

            VM_Call(gvm, GAME_AUTH_WHOIS, (int)(cl - svs.clients));
        }

    } else {

        cl = SV_GetPlayerByHandle();

        if (!cl) {
            return;
        }

        VM_Call(gvm, GAME_AUTH_WHOIS, (int)(cl - svs.clients));

    }

}

/////////////////////////////////////////////////////////////////////
// Name        : SV_Auth_Ban_f
// Description : Ban a user from the server and the group
// Author      : Barbatos, Kalish
/////////////////////////////////////////////////////////////////////
static void SV_Auth_Ban_f(void) {

    client_t    *cl;
    char        *d, *h, *m;

    if (!com_sv_running->integer) {
        Com_Printf("Server is not running\n");
        return;
    }

    if (Cvar_VariableValue("auth") == 0) {
        Com_Printf("Auth services are disabled\n");
        return;
    }

    if (Cmd_Argc() < 5) {
        Com_Printf("Usage: auth-ban <client> <days> <hours> <mins>\n");
        return;
    }

    cl = SV_GetPlayerByHandle();

    if (!cl) {
        return;
    }

    if (cl->netchan.remoteAddress.type == NA_LOOPBACK) {
        SV_BroadcastMessageToClient(NULL, "Cannot ban host client");
        return;
    }

    d = Cmd_Argv(2);
    h = Cmd_Argv(3);
    m = Cmd_Argv(4);

    VM_Call(gvm, GAME_AUTH_BAN, (int)(cl - svs.clients), atoi(d), atoi(h), atoi(m));

}
#endif

/////////////////////////////////////////////////////////////////////
// Name        : SV_AddOperatorCommands
// Description : Add the operator commands
/////////////////////////////////////////////////////////////////////
void SV_AddOperatorCommands(void) {

    static qboolean initialized;

    if (initialized) {
        return;
    }

    initialized = qtrue;

    Cmd_AddCommand("heartbeat", SV_Heartbeat_f);
    Cmd_AddCommand("kick", SV_Kick_f);
    Cmd_AddCommand("clientkick", SV_Kick_f);
    Cmd_AddCommand("status", SV_Status_f);
    Cmd_AddCommand("serverinfo", SV_Serverinfo_f);
    Cmd_AddCommand("systeminfo", SV_Systeminfo_f);
    Cmd_AddCommand("dumpuser", SV_DumpUser_f);
    Cmd_AddCommand("map_restart", SV_MapRestart_f);
    Cmd_AddCommand("sectorlist", SV_SectorList_f);
    Cmd_AddCommand("map", SV_Map_f);
    Cmd_AddCommand("teleport", SV_Teleport_f);
    Cmd_AddCommand("position", SV_Position_f);
    Cmd_AddCommand("sendclientcommand", SV_SendClientCommand_f);
    Cmd_AddCommand("forcecvar", SV_ForceCvar_f);
    Cmd_AddCommand("spoof", SV_Spoof_f);
    #ifndef PRE_RELEASE_DEMO
    Cmd_AddCommand("devmap", SV_Map_f);
    Cmd_AddCommand("spmap", SV_Map_f);
    Cmd_AddCommand("spdevmap", SV_Map_f);
    #endif
    Cmd_AddCommand("killserver", SV_KillServer_f);
    if (com_dedicated->integer) {
        Cmd_AddCommand("say", SV_ConSay_f);
        Cmd_AddCommand("tell", SV_ConTell_f);
        Cmd_AddCommand("startserverdemo", SV_StartServerDemo_f);
        Cmd_AddCommand("stopserverdemo", SV_StopServerDemo_f);
        #ifdef USE_AUTH
        Cmd_AddCommand ("auth-whois", SV_Auth_Whois_f);
        Cmd_AddCommand ("auth-ban", SV_Auth_Ban_f);
        #endif
    }
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_RemoveOperatorCommands
// Description : Remove the operator commands
/////////////////////////////////////////////////////////////////////
void SV_RemoveOperatorCommands(void) {
#if 0
    // removing these won't let the server start again
    Cmd_RemoveCommand("heartbeat");
    Cmd_RemoveCommand("kick");
    Cmd_RemoveCommand("status");
    Cmd_RemoveCommand("serverinfo");
    Cmd_RemoveCommand("systeminfo");
    Cmd_RemoveCommand("dumpuser");
    Cmd_RemoveCommand("map_restart");
    Cmd_RemoveCommand("sectorlist");
    Cmd_RemoveCommand("say");
    Cmd_RemoveCommand("tell");
    Cmd_RemoveCommand("startserverdemo");
    Cmd_RemoveCommand("stopserverdemo");
#endif
}
