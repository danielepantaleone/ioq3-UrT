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

// sv_main.c

#include "server.h"

serverStatic_t  svs;                        // persistant server info
server_t        sv;                         // local server
vm_t            *gvm = NULL;                // game virtual machine

cvar_t    *sv_fps;                          // time rate for running non-clients
cvar_t    *sv_timeout;                      // seconds without any message
cvar_t    *sv_zombietime;                   // seconds to sink messages after disconnect
cvar_t    *sv_rconPassword;                 // password for remote server commands
cvar_t    *sv_rconRecoveryPassword;         // password for recovery the password remote server commands
cvar_t    *sv_rconAllowedSpamIP;            // this ip is allowed to do spam on rcon
cvar_t    *sv_privatePassword;              // password for the privateClient slots
cvar_t    *sv_allowDownload;
cvar_t    *sv_maxclients;

cvar_t    *sv_privateClients;               // number of clients reserved for password
cvar_t    *sv_hostname;
cvar_t    *sv_master[MAX_MASTER_SERVERS];   // master server ip address
cvar_t    *sv_reconnectlimit;               // minimum seconds between connect messages
cvar_t    *sv_showloss;                     // report when usercmds are lost
cvar_t    *sv_padPackets;                   // add nop bytes to messages
cvar_t    *sv_killserver;                   // menu system can set to 1 to shut server down
cvar_t    *sv_mapname;
cvar_t    *sv_mapChecksum;
cvar_t    *sv_serverid;
cvar_t    *sv_minRate;
cvar_t    *sv_maxRate;
cvar_t    *sv_minPing;
cvar_t    *sv_maxPing;
cvar_t    *sv_gametype;
cvar_t    *sv_pure;
cvar_t    *sv_floodProtect;
cvar_t    *sv_lanForceRate;                 // dedicated 1 (LAN) server forces local client rates to 99999
cvar_t    *sv_strictAuth;
cvar_t    *sv_clientsPerIp;
cvar_t    *sv_demoNotice;                   // notice to print to a client being recorded server-side
cvar_t    *sv_tellPrefix;
cvar_t    *sv_sayPrefix;
cvar_t    *sv_demoFolder;

#ifdef USE_AUTH
cvar_t    *sv_authServerIP;
cvar_t    *sv_auth_engine;
#endif

cvar_t    *sv_disableRadio;
cvar_t    *sv_callvoteWaitTime;
cvar_t    *sv_ghostRadius;
cvar_t    *sv_hideChatCmd;
cvar_t    *sv_autoDemo;
cvar_t    *sv_noStamina;
cvar_t    *sv_noKnife;
cvar_t    *sv_dropSuffix;
cvar_t    *sv_dropSignature;
cvar_t    *sv_checkClientGuid;

#define SV_OUTPUTBUF_LENGTH (1024 - 16)

/////////////////////////////////////////////////////////////////////
// Name        : SV_BroadcastMessageToClient
// Description : Send a server message to a specific client
// Author      : Fenix
/////////////////////////////////////////////////////////////////////
void SV_BroadcastMessageToClient(client_t *cl, const char *fmt, ...) {
    
    char    str[MAX_STRING_CHARS];
    va_list ap;
    
    va_start(ap, fmt);
    vsprintf(str, fmt, ap);
    va_end(ap);
    
    SV_SendServerCommand(cl, "print \"%s\n\"", str);
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_LogPrintf
// Description : Print in the log file
// Author      : Fenix
/////////////////////////////////////////////////////////////////////
void QDECL SV_LogPrintf(const char *fmt, ...) {
    
    va_list       argptr;
    fileHandle_t  file;
    fsMode_t      mode;
    char          *logfile;
    char          buffer[MAX_STRING_CHARS];
    int           min, tens, sec;
    int           logsync;
    
    // retrieve the logfile name
    logfile = Cvar_VariableString("g_log");
    if (!logfile[0]) {
        return;
    }
    
    // retrieve the writing mode
    logsync = Cvar_VariableIntegerValue("g_logSync");
    mode = logsync ? FS_APPEND_SYNC : FS_APPEND;
    
    // opening the log file
    FS_FOpenFileByMode(logfile, &file, mode);
    if (!file) {
        return;
    }
    
    // get current level time
    sec  = sv.time / 1000;
    min  = sec / 60;
    sec -= min * 60;
    tens = sec / 10;
    sec -= tens * 10;
    
    // prepend current level time
    Com_sprintf(buffer, sizeof(buffer), "%3i:%i%i ", min, tens, sec);

    // get the arguments
    va_start(argptr, fmt);
    vsprintf(buffer + 7, fmt, argptr);
    va_end(argptr);
    
    // write in the log file
    FS_Write(buffer, (int) strlen(buffer), file);
    FS_FCloseFile(file);
        
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_LoadPositionFromFile
// Description : Load the client position from a file
// Author      : Fenix
/////////////////////////////////////////////////////////////////////
void SV_LoadPositionFromFile(client_t *cl, char *mapname) {
    
    fileHandle_t   file;
    char           buffer[MAX_STRING_CHARS];
    char           *guid;
    char           *qpath;
    int            len;
    
    // if we are not playing jump mode
    if (sv_gametype->integer != GT_JUMP) {
        return;
    }
    
    // if we are not supposed to save the position on a file
    if ((Cvar_VariableIntegerValue("g_allowPosSaving") <= 0) ||
        (Cvar_VariableIntegerValue("g_persistentPositions") <= 0)) {
        return;
    }

    guid = Info_ValueForKey(cl->userinfo, "cl_guid");
    if (!guid || !guid[0]) { 
        return;
    }

    qpath = va("positions/%s/%s.pos", mapname, guid);
    FS_FOpenFileByMode(qpath, &file, FS_READ);
    
    // if not valid
    if (!file) {
        return;
    }

    len = FS_Read(buffer, sizeof(buffer), file);
    if (len > 0) {
        sscanf(buffer, "%f,%f,%f,%f,%f,%f", &cl->savedPosition[0], &cl->savedPosition[1], &cl->savedPosition[2],
                                            &cl->savedPositionAngle[0], &cl->savedPositionAngle[1],
                                            &cl->savedPositionAngle[2]);
    }   

    FS_FCloseFile(file);  
    
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_SavePositionToFile
// Description : Save the client position to a file
// Author      : Fenix
/////////////////////////////////////////////////////////////////////
void SV_SavePositionToFile(client_t *cl, char *mapname) {
    
    fileHandle_t   file;
    char           buffer[MAX_STRING_CHARS];
    char           *guid;
    char           *qpath;
    
    // if we are not playing jump mode
    if (sv_gametype->integer != GT_JUMP) {
        return;
    }
    
    // if we are not supposed to save the position on a file
    if ((Cvar_VariableIntegerValue("g_allowPosSaving") <= 0) ||
        (Cvar_VariableIntegerValue("g_persistentPositions") <= 0)) {
        return;
    }
    
    // get the client guid from the userinfo string
    guid = Info_ValueForKey(cl->userinfo, "cl_guid");
    if (!guid || !guid[0] || (!cl->savedPosition[0] && !cl->savedPosition[1] && !cl->savedPosition[2])) { 
        return;
    }

    qpath = va("positions/%s/%s.pos", mapname, guid);
    FS_FOpenFileByMode(qpath, &file, FS_WRITE);

    if (!file) {
        return;
    }

    Com_sprintf(buffer, sizeof(buffer), "%f,%f,%f,%f,%f,%f", cl->savedPosition[0], cl->savedPosition[1],
                                                             cl->savedPosition[2], cl->savedPositionAngle[0],
                                                             cl->savedPositionAngle[1], cl->savedPositionAngle[2]);

    FS_Write(buffer, (int) strlen(buffer), file);
    FS_FCloseFile(file);  
    
}

typedef struct {
    char    *name;
    int     flags;
    int     params;
} callvote_t;

callvote_t callvotes[] = { 
    { "reload",                 1 << 0,  2 },
    { "restart",                1 << 1,  2 },
    { "map",                    1 << 2,  3 },
    { "nextmap",                1 << 3,  3 },
    { "kick",                   1 << 4,  3 },
    { "clientkick",             1 << 4,  3 },
    { "swapteams",              1 << 5,  2 },
    { "shuffleteams",           1 << 6,  2 },
    { "g_friendlyfire",         1 << 7,  3 },
    { "g_followstrict",         1 << 8,  3 },
    { "g_gametype",             1 << 9,  3 },
    { "g_waverespawns",         1 << 10, 3 },
    { "timelimit",              1 << 11, 3 },
    { "fraglimit",              1 << 12, 3 },
    { "capturelimit",           1 << 13, 3 },
    { "g_respawndelay",         1 << 14, 3 },
    { "g_redwave",              1 << 15, 3 },
    { "g_bluewave",             1 << 16, 3 },
    { "g_bombexplodetime",      1 << 17, 3 },
    { "g_bombdefusetime",       1 << 18, 3 },
    { "g_roundtime",            1 << 19, 3 },
    { "g_cahtime",              1 << 20, 3 },
    { "g_warmup",               1 << 21, 3 },
    { "g_matchmode",            1 << 22, 3 },
    { "g_timeouts",             1 << 23, 3 },
    { "g_timeoutlength",        1 << 24, 3 },
    { "exec",                   1 << 25, 3 },
    { "g_swaproles",            1 << 26, 3 },
    { "g_maxrounds",            1 << 27, 3 },
    { "g_gear",                 1 << 28, 3 },
    { "cyclemap",               1 << 29, 2 },
    { NULL }
};

/////////////////////////////////////////////////////////////////////
// Name        : SV_CallvoteEnabled
// Description : Tells whether a certain type of callvote is
//               enabled on the server or not
// Author      : Fenix
/////////////////////////////////////////////////////////////////////
qboolean SV_CallvoteEnabled(char *text) {
    int val;
    callvote_t *p;
    val = Cvar_VariableIntegerValue("g_allowvote");
    for (p = callvotes; p->name; p++) {
        if (!Q_stricmp(text, p->name)) {
            return val & p->flags ? qtrue : qfalse;
        }
    }
    return qfalse;
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_CheckCallvoteArgs
// Description : Check correct parameters for the given callvote.
//               Will return qtrue if the callvote is legit, qfalse
//               if it's going to be blocked by the game module
// Author      : Fenix
/////////////////////////////////////////////////////////////////////
qboolean SV_CheckCallvoteArgs(void) {
    
    int val;
    callvote_t *p;
    char mapname[MAX_QPATH];
    
    // check for minimum parameters to be specified
    for (p = callvotes; p->name; p++) {
        if (!Q_stricmp(Cmd_Argv(1), p->name)) {
            if (Cmd_Argc() < p->params)
                return qfalse;
            break;
        }
    }
    
    // additional checks for map/nextmap callvotes
    if (!Q_stricmp(Cmd_Argv(1), "map") || !Q_stricmp(Cmd_Argv(1), "nextmap")) {
        SV_GetMapSoundingLike(mapname, Cmd_Argv(2), sizeof(mapname));
        if (!mapname[0]) {
            return qfalse;
        }
    } 
    // additional checks for kick/clientkick callvotes
    else if (!Q_stricmp(Cmd_Argv(1), "kick") || !Q_stricmp(Cmd_Argv(1), "clientkick")) {
        // invalid client supplied
        if (!SV_GetPlayerByParam(Cmd_Argv(2))) {
            return qfalse;
        }
    }
    // additional checks for g_gametype callvotes
    else if (!Q_stricmp(Cmd_Argv(1), "g_gametype")) {
        val = atoi(Cmd_Argv(2));
        if ((val < GT_FFA) || (val == GT_SINGLE_PLAYER) || (val > GT_FREEZE)) {
            return qfalse;
        }
    }
    
    return qtrue;

}

/**
 * SV_GetClientTeam
 * 
 * @description Retrieve the given client team
 * @param slot The client slot number
 * @return The team the given client belongs to
 */
int SV_GetClientTeam(int slot) {
    playerState_t *ps;
    ps = SV_GameClientNum(slot);
    return ps->persistant[PERS_TEAM];
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_GetMatchState
// Description : Retrieve the match state
// Author      : Fenix
/////////////////////////////////////////////////////////////////////
int SV_GetMatchState(void) {
    int state = atoi(sv.configstrings[1005]);
    return state;
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_IsClientGhost
// Description : Tells whether a client has ghosting activated
// Author      : Fenix
/////////////////////////////////////////////////////////////////////
qboolean SV_IsClientGhost(client_t *cl) {
    
    int ghost;
    
    // if we are not playing jump mode
    if (sv_gametype->integer != GT_JUMP) {
        return qfalse;
    }
    
    // get the ghosting flag from the userinfo string
    ghost = atoi(Info_ValueForKey(cl->userinfo, "cg_ghost"));
    return ghost > 0 ? qtrue : qfalse;

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                          //
//  EVENTS MESSAGES                                                                                         //
//                                                                                                          //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// Name        : SV_ExpandNewlines
// Description : Converts newlines to "\n" so a line prints nicer
/////////////////////////////////////////////////////////////////////
char *SV_ExpandNewlines(char *in) {
    
    int         l;
    static char string[1024];

    l = 0;
    while (*in && l < sizeof(string) - 3) {
        
        if (*in == '\n') {
            string[l++] = '\\';
            string[l++] = 'n';
        } else {
            string[l++] = *in;
        }
        in++;
    
    }
    
    string[l] = 0;
    return string;
    
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_AddServerCommand
// Description : The given command will be transmitted to the client, 
//               and is guaranteed to not have future snapshot_t 
//               executed before it is executed
/////////////////////////////////////////////////////////////////////
void SV_AddServerCommand(client_t *client, const char *cmd) {
    
    int index, i;

    // do not send commands until the gamestate has been sent
    if (client->state < CS_PRIMED) {
        return;
    }
    
    client->reliableSequence++;
    
    // if we would be losing an old command that hasn't been acknowledged,
    // we must drop the connection
    // we check == instead of >= so a broadcast print added by SV_DropClient()
    // doesn't cause a recursive drop client
    if (client->reliableSequence - client->reliableAcknowledge == MAX_RELIABLE_COMMANDS + 1) {
        
        Com_Printf("===== pending server commands =====\n");
        for (i = client->reliableAcknowledge + 1 ; i <= client->reliableSequence ; i++) {
            Com_Printf("cmd %5d: %s\n", i, client->reliableCommands[ i & (MAX_RELIABLE_COMMANDS-1) ]);
        }
        
        Com_Printf("cmd %5d: %s\n", i, cmd);
        SV_DropClient(client, "Server command overflow");
        return;
    }
    
    index = client->reliableSequence & (MAX_RELIABLE_COMMANDS - 1);
    Q_strncpyz(client->reliableCommands[ index ], cmd, sizeof(client->reliableCommands[ index ]));

}

/////////////////////////////////////////////////////////////////////
// Name        : SV_SendServerCommand
// Description : Sends a reliable command string to be interpreted by
//               the client game module: "cp", "print", "chat", etc
//               A NULL client will broadcast to all clients
/////////////////////////////////////////////////////////////////////
void QDECL SV_SendServerCommand(client_t *cl, const char *fmt, ...) {
    
    va_list     argptr;
    byte        message[MAX_MSGLEN];
    client_t    *client;
    int         j;
    
    va_start (argptr,fmt);
    Q_vsnprintf ((char *)message, sizeof(message), fmt,argptr);
    va_end (argptr);

    // Fix to http://aluigi.altervista.org/adv/q3msgboom-adv.txt
    // The actual cause of the bug is probably further downstream
    // and should maybe be addressed later, but this certainly
    // fixes the problem for now
    if (strlen ((char *)message) > 1022) {
        return;
    }

    if (cl != NULL) {
        SV_AddServerCommand(cl, (char *)message);
        return;
    }

    // hack to echo broadcast prints to console
    if (com_dedicated->integer && !strncmp((char *)message, "print", 5)) {
        Com_Printf ("broadcast: %s\n", SV_ExpandNewlines((char *)message));
    }

    // send the data to all relevent clients
    for (j = 0, client = svs.clients; j < sv_maxclients->integer ; j++, client++) {
        SV_AddServerCommand(client, (char *)message);
    }

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                          //
//  MASTER SERVER FUNCTIONS                                                                                 //
//                                                                                                          //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define HEARTBEAT_MSEC 300 * 1000
#define HEARTBEAT_GAME "QuakeArena-1"

/////////////////////////////////////////////////////////////////////
// Name        : SV_MasterHeartbeat
// Description : Send a message to the masters every few minutes to
//               let it know we are alive, and log information.
//               We will also have a heartbeat sent when a server
//               changes from empty to non-empty, and full to
//               non-full but not on every player enter or exit.
/////////////////////////////////////////////////////////////////////
void SV_MasterHeartbeat(void) {
    
    int             i;
    static netadr_t adr[MAX_MASTER_SERVERS];

    // "dedicated 1" is for lan play, "dedicated 2" is for inet public play
    if (!com_dedicated || com_dedicated->integer != 2) {
        return; // only dedicated servers send heartbeats
    }

    // if not time yet, don't send anything
    if (svs.time < svs.nextHeartbeatTime) {
        return;
    }
    
    svs.nextHeartbeatTime = svs.time + HEARTBEAT_MSEC;

    #ifdef USE_AUTH
    VM_Call(gvm, GAME_AUTHSERVER_HEARTBEAT);
    #endif
    
    // send to group masters
    for (i = 0 ; i < MAX_MASTER_SERVERS ; i++) {
        
        if (!sv_master[i]->string[0]) {
            continue;
        }

        // see if we haven't already resolved the name
        // resolving usually causes hitches on win95, so only
        // do it when needed
        if (sv_master[i]->modified) {
            
            sv_master[i]->modified = qfalse;

            Com_Printf("Resolving %s\n", sv_master[i]->string);
            if (!NET_StringToAdr(sv_master[i]->string, &adr[i])) {
                // if the address failed to resolve, clear it
                // so we don't take repeated dns hits
                Com_Printf("Couldn't resolve address: %s\n", sv_master[i]->string);
                Cvar_Set(sv_master[i]->name, "");
                sv_master[i]->modified = qfalse;
                continue;
            }
            
            if (!strchr(sv_master[i]->string, ':')) {
                adr[i].port = (unsigned short) BigShort(PORT_MASTER);
            }
            
            Com_Printf("%s resolved to %i.%i.%i.%i:%i\n", sv_master[i]->string, adr[i].ip[0], adr[i].ip[1],
                                                          adr[i].ip[2], adr[i].ip[3], BigShort(adr[i].port));
        }

        Com_Printf ("Sending heartbeat to %s\n", sv_master[i]->string);
        // this command should be changed if the server info / status format
        // ever incompatably changes
        NET_OutOfBandPrint(NS_SERVER, adr[i], "heartbeat %s\n", HEARTBEAT_GAME);
        
    }
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_MasterShutdown
// Description : Informs all masters that this server is going down
/////////////////////////////////////////////////////////////////////
void SV_MasterShutdown(void) {
    
    // send a hearbeat right now
    svs.nextHeartbeatTime = -9999;
    SV_MasterHeartbeat();

    // send it again to minimize chance of drops
    svs.nextHeartbeatTime = -9999;
    SV_MasterHeartbeat();

    // when the master tries to poll the server, it won't respond, so
    // it will be removed from the list
    
    #ifdef USE_AUTH
    VM_Call(gvm, GAME_AUTHSERVER_SHUTDOWN);
    #endif
    
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                          //
//  CONNECTIONLESS COMMANDS                                                                                 //
//                                                                                                          //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// Name        : SVC_Status
// Description : Responds with all the info that qplug or qspy can 
//               see about the server and all connected players.  
//               Used for getting detailed information after the 
//               simple info query.
/////////////////////////////////////////////////////////////////////
void SVC_Status(netadr_t from) {
    
    char            player[1024];
    char            status[MAX_MSGLEN];
    int             i;
    client_t        *cl;
    playerState_t   *ps;
    int             statusLength;
    int             playerLength;
    char            infostring[MAX_INFO_STRING];

    // ignore if we are in single player
    if (Cvar_VariableValue("g_gametype") == GT_SINGLE_PLAYER) {
        return;
    }

    strcpy(infostring, Cvar_InfoString(CVAR_SERVERINFO));

    // echo back the parameter to status. so master servers can use it as a challenge
    // to prevent timed spoofed reply packets that add ghost servers
    Info_SetValueForKey(infostring, "challenge", Cmd_Argv(1));

    status[0] = 0;
    statusLength = 0;

    for (i = 0 ; i < sv_maxclients->integer; i++) {
        
        cl = &svs.clients[i];
        if (cl->state >= CS_CONNECTED) {
            ps = SV_GameClientNum(i);
            Com_sprintf (player, sizeof(player), "%i %i \"%s\"\n", ps->persistant[PERS_SCORE], 
                                                                   cl->ping, cl->name);
            playerLength = (int) strlen(player);
            if (statusLength + playerLength >= sizeof(status)) {
                break; // can't hold any more
            }
            
            strcpy (status + statusLength, player);
            statusLength += playerLength;
        
        }
    }

    NET_OutOfBandPrint(NS_SERVER, from, "statusResponse\n%s\n%s", infostring, status);
}

/////////////////////////////////////////////////////////////////////
// Name        : SVC_Info
// Description : Responds with a short info message that should be 
//               enough to determine if a user is interested in a 
//               server to do a full status
/////////////////////////////////////////////////////////////////////
void SVC_Info(netadr_t from) {
    
    int     i, count, bots;
    char    *gamedir;
    char    infostring[MAX_INFO_STRING];

    // ignore if we are in single player
    if (Cvar_VariableValue("g_gametype") == GT_SINGLE_PLAYER || 
        Cvar_VariableValue("ui_singlePlayerActive")) {
        return;
    }

    // Check whether Cmd_Argv(1) has a sane length. 
    // This was not done in the original Quake3 version which led 
    // to the Infostring bug discovered by Luigi Auriemma. 
    // See http://aluigi.altervista.org/ for the advisory.

    // A maximum challenge length of 128 should be more than plenty.
    if (strlen(Cmd_Argv(1)) > 128) {
        return;
    }
    
    // don't count privateclients
    count = 0;
    bots = 0;
    for (i = sv_privateClients->integer ; i < sv_maxclients->integer ; i++) {
        if (svs.clients[i].state >= CS_CONNECTED) {
            count++;
            if (svs.clients[i].netchan.remoteAddress.type == NA_BOT) {
                bots++;
            }
        }
    }

    infostring[0] = 0;

    // echo back the parameter to status. so servers can use it as a challenge
    // to prevent timed spoofed reply packets that add ghost servers
    Info_SetValueForKey(infostring, "challenge", Cmd_Argv(1));
    Info_SetValueForKey(infostring, "protocol", va("%i", PROTOCOL_VERSION));
    Info_SetValueForKey(infostring, "hostname", sv_hostname->string);
    Info_SetValueForKey(infostring, "mapname", sv_mapname->string);
    Info_SetValueForKey(infostring, "clients", va("%i", count));
    Info_SetValueForKey( infostring, "bots", va("%i", bots));
    Info_SetValueForKey(infostring, "sv_maxclients", va("%i", sv_maxclients->integer - sv_privateClients->integer));
    Info_SetValueForKey(infostring, "gametype", va("%i", sv_gametype->integer));
    Info_SetValueForKey(infostring, "pure", va("%i", sv_pure->integer));
    
    #ifdef USE_AUTH
    Info_SetValueForKey(infostring, "auth", Cvar_VariableString("auth"));
    #endif

    if (Cvar_VariableValue("g_needpass") == 1) {
        Info_SetValueForKey(infostring, "password", va("%i", 1));
    }
    
    if (sv_minPing->integer) {
        Info_SetValueForKey(infostring, "minPing", va("%i", sv_minPing->integer));
    }
    
    if(sv_maxPing->integer) {
        Info_SetValueForKey(infostring, "maxPing", va("%i", sv_maxPing->integer));
    }
    
    gamedir = Cvar_VariableString("fs_game");
    if (*gamedir) {
        Info_SetValueForKey(infostring, "game", gamedir);
    }

    Info_SetValueForKey(infostring, "modversion", Cvar_VariableString("g_modversion"));
    NET_OutOfBandPrint(NS_SERVER, from, "infoResponse\n%s", infostring);

}

/////////////////////////////////////////////////////////////////////
// Name : SVC_FlushRedirect
/////////////////////////////////////////////////////////////////////
void SV_FlushRedirect(char *outputbuf) {
    NET_OutOfBandPrint(NS_SERVER, svs.redirectAddress, "print\n%s", outputbuf);
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_ParseGameRemoteCommand
// Description : Parse game RCON commands allowing us to perform
//               some operations before letting the game QVM module
//               handle the command
// Author      : Fenix
/////////////////////////////////////////////////////////////////////
void SV_ParseGameRemoteCommand(char *text) {
    
    int val;
    
    Cmd_TokenizeString(text);	
    
    // if we got no tokens	
    if (!Cmd_Argc()) {
        return;
    }
    
    // if it's a rcon veto command
    if (!Q_stricmp(Cmd_Argv(0), "veto")) {
        val = sv_callvoteWaitTime->integer * 1000;
        sv.lastVoteTime = svs.time - val;
    }
    
}

/////////////////////////////////////////////////////////////////////
// Name        : SVC_RconRecoveryRemoteCommand
// Description : An rcon packet arrived from the network.
//               Shift down the remaining args
//               Redirect all printfs
/////////////////////////////////////////////////////////////////////
void SVC_RconRecoveryRemoteCommand(netadr_t from, msg_t *msg) {
    
    qboolean    valid;
    unsigned int time;
    
    // TTimo - scaled down to accumulate, but not overflow anything 
    // network wise, print wise etc. (OOB messages are the bottleneck here)
    char sv_outputbuf[SV_OUTPUTBUF_LENGTH];
    static unsigned int lasttime = 0;

    // TTimo - https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=534
    time = (unsigned int) Com_Milliseconds();
    
    if (!strlen(sv_rconRecoveryPassword->string) || strcmp (Cmd_Argv(1), sv_rconRecoveryPassword->string)) {
        
        // MaJ - If the rconpassword is bad and one just 
        // happened recently, don't spam the log file, just die.
        if ((time - lasttime) < 600u) {
            return;
        }
        
        valid = qfalse;
        Com_Printf("Bad rcon recovery from %s:\n%s\n", NET_AdrToString (from), Cmd_Argv(2));
    
    } else {
        
        // MaJ - If the rconpassword is good, 
        // allow it much sooner than a bad one
        if (time - lasttime < 180u) {
            return;
        }
        
        valid = qtrue;
        Com_Printf("Rcon recovery from %s:\n%s\n", NET_AdrToString (from), Cmd_Argv(2));

    }
    
    lasttime = time;

    // start redirecting all print outputs to the packet
    svs.redirectAddress = from;
    Com_BeginRedirect (sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);

    if (!strlen(sv_rconPassword->string)) {
        Com_Printf ("No rcon recovery password set on the server.\n");
    } else if (!valid) {
        Com_Printf("Bad rcon recovery password.\n");
    } else {
        Com_Printf("rconpassword: %s\n" , sv_rconPassword->string);
    }

    Com_EndRedirect();
    
}

/////////////////////////////////////////////////////////////////////
// Name        : SVC_RemoteCommand
// Description : An rcon packet arrived from the network.
//               Shift down the remaining args
//               Redirect all printfs
/////////////////////////////////////////////////////////////////////
void SVC_RemoteCommand(netadr_t from, msg_t *msg) {

    qboolean     valid;
    char         remaining[1024];
    netadr_t     allowedSpamIPAdress;
    unsigned int time;
    
    // TTimo - scaled down to accumulate, but not overflow anything 
    // network wise, print wise etc. (OOB messages are the bottleneck here)
    char sv_outputbuf[SV_OUTPUTBUF_LENGTH];
    static unsigned int lasttime = 0;
    char *cmd_aux;

    // TTimo - https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=534
    time = (unsigned int) Com_Milliseconds();

    NET_StringToAdr(sv_rconAllowedSpamIP->string , &allowedSpamIPAdress);
    
    // if there is no rconpassword set or the rconpassword given
    // is not valid and the guy sending the command is not a RCON user
    if ((!strlen(sv_rconPassword->string) || strcmp(Cmd_Argv(1), sv_rconPassword->string))) {
        
        // let's the sv_rconAllowedSpamIP do spam rcon
        if ((!strlen(sv_rconAllowedSpamIP->string) || 
             !NET_CompareBaseAdr(from , allowedSpamIPAdress)) && 
             !NET_IsLocalAddress(from)){
            // MaJ - If the rconpassword is bad and one just happened recently, 
            // don't spam the log file, just die.
            if (time - lasttime < 600u) {
                return;
            }   
        }
        
        valid = qfalse;
        Com_Printf("Bad rcon from %s:\n%s\n", NET_AdrToString (from), Cmd_Argv(2));
        
    } else {
    
        // let's the sv_rconAllowedSpamIP do spam rcon
        if ((!strlen(sv_rconAllowedSpamIP->string) || 
             !NET_CompareBaseAdr(from , allowedSpamIPAdress)) && 
             !NET_IsLocalAddress(from)){
            // MaJ - If the rconpassword is good, 
            // allow it much sooner than a bad one.
            if ((time - lasttime) < 180u) {
                return;
            }
        }
        
        valid = qtrue;
        Com_Printf("Rcon from %s:\n%s\n", NET_AdrToString (from), Cmd_Argv(2));
    }
    
    lasttime = time;

    // start redirecting all print outputs to the packet
    svs.redirectAddress = from;
    Com_BeginRedirect(sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);

    if (!strlen(sv_rconPassword->string)) {
        Com_Printf("No rconpassword set on the server.\n");
    } else if (!valid) {
        Com_Printf("Bad rconpassword.\n");
    } else {
        
        remaining[0] = 0;
        // https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=543
        // get the command directly, "rcon <pass> <command>" to avoid quoting issues
        // extract the command by walking since the cmd formatting can fuckup (amount of spaces), 
        // using a dumb step by step parsing
        cmd_aux = Cmd_Cmd();
        cmd_aux += 4;                                 // rcon
        while(cmd_aux[0] == ' ') {                    // spaces
            cmd_aux++;
        }

        while(cmd_aux[0] && cmd_aux[0] != ' ') {
            cmd_aux++;  // password
        }
        while(cmd_aux[0] == ' ') {
            cmd_aux++;  // spaces
        }

        Q_strcat(remaining, sizeof(remaining), cmd_aux);
        
        // additional parse for game module commands
        // will let us perform certain operations when a
        // game module rcon command is detected
        SV_ParseGameRemoteCommand(remaining);
        
        // execute the command
        Cmd_ExecuteString(remaining);

    }

    Com_EndRedirect();
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_CheckDRDoS
// Description : DRDoS stands for "Distributed Reflected Denial of 
//               Service". Returns qfalse if we're good. A qtrue 
//               return value means we need to block. If the address 
//               isn't NA_IP, it's automatically denied.
// Author      : Rambetter
/////////////////////////////////////////////////////////////////////
qboolean SV_CheckDRDoS(netadr_t from) {
    
    int          i;
    int          globalCount;
    int          specificCount;
    receipt_t    *receipt;
    netadr_t     exactFrom;
    int          oldest;
    int          oldestTime;
    static int   lastGlobalLogTime = 0;
    static int   lastSpecificLogTime = 0;

    // Usually the network is smart enough to not allow incoming UDP packets
    // with a source address being a spoofed LAN address.  Even if that's not
    // the case, sending packets to other hosts in the LAN is not a big deal.
    // NA_LOOPBACK qualifies as a LAN address.
    if (Sys_IsLANAddress(from)) { return qfalse; }

    exactFrom = from;
    
    if (from.type == NA_IP) {
        from.ip[3] = 0; // xx.xx.xx.0
    } else {
        // So we got a connectionless packet but it's not IPv4, so
        // what is it?  I don't care, it doesn't matter, we'll just block it.
        // This probably won't even happen.
        return qtrue;
    }

    // Count receipts in last 2 seconds.
    globalCount = 0;
    specificCount = 0;
    receipt = &svs.infoReceipts[0];
    oldest = 0;
    oldestTime = 0x7fffffff;
    
    for (i = 0; i < MAX_INFO_RECEIPTS; i++, receipt++) {
        
        if (receipt->time + 2000 > svs.time) {
            if (receipt->time) {
                // When the server starts, all receipt times are at zero.  Furthermore,
                // svs.time is close to zero.  We check that the receipt time is already
                // set so that during the first two seconds after server starts, queries
                // from the master servers don't get ignored.  As a consequence a potentially
                // unlimited number of getinfo+getstatus responses may be sent during the
                // first frame of a server's life.
                globalCount++;
            }
            if (NET_CompareBaseAdr(from, receipt->adr)) {
                specificCount++;
            }
            
        }
        
        if (receipt->time < oldestTime) {
            oldestTime = receipt->time;
            oldest = i;
        }
        
    }

    if (globalCount == MAX_INFO_RECEIPTS) { // All receipts happened in last 2 seconds.
        if (lastGlobalLogTime + 1000 <= svs.time){ // Limit one log every second.
            Com_Printf("Detected flood of getinfo/getstatus connectionless packets.\n");
            lastGlobalLogTime = svs.time;
        }
        return qtrue;
    }
    
    if (specificCount >= 3) {                           // Already sent 3 to this IP in last 2 seconds.
        
        if (lastSpecificLogTime + 1000 <= svs.time) {   // Limit one log every second.
            Com_DPrintf("Possible DRDoS attack to address %i.%i.%i.%i, "
                        "ignoring getinfo/getstatus connectionless packet\n",
                        exactFrom.ip[0], exactFrom.ip[1], exactFrom.ip[2], exactFrom.ip[3]);
            lastSpecificLogTime = svs.time;
        }
    
        return qtrue;
    
    }

    receipt = &svs.infoReceipts[oldest];
    receipt->adr = from;
    receipt->time = svs.time;
    return qfalse;
    
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_ConnectionlessPacket
// Description : A connectionless packet has four leading 0xff
//               characters to distinguish it from a game channel.
//               Clients that are in the game can still send
//               connectionless packets.
/////////////////////////////////////////////////////////////////////
void SV_ConnectionlessPacket(netadr_t from, msg_t *msg) {
    
    char        *s;
    char        *c;
    #ifdef USE_AUTH
    netadr_t    authServerIP;
    #endif

    MSG_BeginReadingOOB(msg);
    MSG_ReadLong(msg);        // skip the -1 marker

    if (!Q_strncmp("connect", (char *) &msg->data[4], 7)) {
        Huff_Decompress(msg, 12);
    }

    s = MSG_ReadStringLine(msg);
    Cmd_TokenizeString(s);

    c = Cmd_Argv(0);
    Com_DPrintf("SV packet %s : %s\n", NET_AdrToString(from), c);

    if (!Q_stricmp(c, "getstatus")) {
    
        if (SV_CheckDRDoS(from)) { 
            return; 
        }
        SVC_Status(from );
    
    } else if (!Q_stricmp(c, "getinfo")) {
        
        if (SV_CheckDRDoS(from)) { 
            return; 
        }
        SVC_Info(from);
        
    } else if (!Q_stricmp(c, "getchallenge")) {
        SV_GetChallenge(from);
    } else if (!Q_stricmp(c, "connect")) {
        SV_DirectConnect(from);
    } else if (!Q_stricmp(c, "ipauthorize")) {
        SV_AuthorizeIpPacket(from);
    }
    #ifdef USE_AUTH
    else if ((!Q_stricmp(c, "AUTH:SV"))) {
        NET_StringToAdr(sv_authServerIP->string, &authServerIP);
        if (!NET_CompareBaseAdr(from, authServerIP)) {
            Com_Printf("AUTH not from the Auth Server\n");
            return;
        }
        VM_Call(gvm, GAME_AUTHSERVER_PACKET);
    }
    #endif
    else if (!Q_stricmp(c, "rcon")) {
        SVC_RemoteCommand(from, msg);
    }else if (!Q_stricmp(c, "rconrecovery")) {
        SVC_RconRecoveryRemoteCommand(from, msg);
    } else if (!Q_stricmp(c, "disconnect")) {
        // if a client starts up a local server, we may see some spurious
        // server disconnect messages when their new server sees our final
        // sequenced messages to the old client
    } else {
        Com_DPrintf ("bad connectionless packet from %s:\n%s\n", NET_AdrToString (from), s);
    }
    
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_ReadPackets
// Description : Read incoming packets
/////////////////////////////////////////////////////////////////////
void SV_PacketEvent(netadr_t from, msg_t *msg) {
    
    int         i;
    int         qport;
    client_t    *cl;

    // check for connectionless packet (0xffffffff) first
    if (msg->cursize >= 4 && *(int *)msg->data == -1) {
        SV_ConnectionlessPacket(from, msg);
        return;
    }

    // read the qport out of the message so we can fix up
    // stupid address translating routers
    MSG_BeginReadingOOB(msg);
    MSG_ReadLong(msg);
    qport = MSG_ReadShort(msg) & 0xffff;

    // find which client the message is from
    for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++,cl++) {
        
        if (cl->state == CS_FREE) {
            continue;
        }
        
        if (!NET_CompareBaseAdr(from, cl->netchan.remoteAddress)) {
            continue;
        }
        
        // it is possible to have multiple clients from a single IP
        // address, so they are differentiated by the qport variable
        if (cl->netchan.qport != qport) {
            continue;
        }

        // the IP port can't be used to differentiate them, because
        // some address translating routers periodically change UDP
        // port assignments
        if (cl->netchan.remoteAddress.port != from.port) {
            Com_Printf("SV_PacketEvent: fixing up a translated port\n");
            cl->netchan.remoteAddress.port = from.port;
        }

        // make sure it is a valid, in sequence packet
        if (SV_Netchan_Process(cl, msg)) {
            // zombie clients still need to do the Netchan_Process
            // to make sure they don't need to retransmit the final
            // reliable message, but they don't do any other processing
            if (cl->state != CS_ZOMBIE) {
                cl->lastPacketTime = svs.time;    // don't timeout
                SV_ExecuteClientMessage(cl, msg);
            }
        }
        return;
    
    }
    
    // if we received a sequenced packet from an address we don't recognize,
    // send an out of band disconnect packet to it
    NET_OutOfBandPrint(NS_SERVER, from, "disconnect");
    
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_CalcPings
// Description : Updates the cl->ping variables
/////////////////////////////////////////////////////////////////////
void SV_CalcPings(void) {
    
    int            i, j;
    client_t       *cl;
    int            total, count;
    int            delta;
    playerState_t  *ps;

    for (i = 0 ; i < sv_maxclients->integer ; i++) {
        
        cl = &svs.clients[i];
        if (cl->state != CS_ACTIVE) {
            cl->ping = 999;
            continue;
        }
        
        if (!cl->gentity) {
            cl->ping = 999;
            continue;
        }
        
        if (cl->gentity->r.svFlags & SVF_BOT) {
            cl->ping = 0;
            continue;
        }

        total = 0;
        count = 0;
        for (j = 0; j < PACKET_BACKUP ; j++) {
            
            if (cl->frames[j].messageAcked <= 0) {
                continue;
            }
            
            delta = cl->frames[j].messageAcked - cl->frames[j].messageSent;
            count++;
            total += delta;
            
        }
        
        if (!count) {
            cl->ping = 999;
        } else {
            
            cl->ping = total/count;
            if (cl->ping > 999) {
                cl->ping = 999;
            }
            
        }

        // let the game dll know about the ping
        ps = SV_GameClientNum(i);
        ps->ping = cl->ping;
        
    }

}

/////////////////////////////////////////////////////////////////////
// Name        : SV_CheckTimeouts
// Description : If a packet has not been received from a client 
//               for timeout->integer seconds, drop the conneciton.  
//               Server time is used instead of realtime to avoid 
//               dropping the local client while debugging.
//               When a client is normally dropped, the client_t goes 
//               into a zombie state for a few seconds to make sure 
//               any final reliable message gets resent if necessary
/////////////////////////////////////////////////////////////////////
void SV_CheckTimeouts(void) {
    
    int        i;
    int        droppoint;
    int        zombiepoint;
    client_t   *cl;

    droppoint = svs.time - 1000 * sv_timeout->integer;
    zombiepoint = svs.time - 1000 * sv_zombietime->integer;

    for (i = 0, cl=svs.clients; i < sv_maxclients->integer; i++,cl++) {
        
        // message times may be wrong across a changelevel
        if (cl->lastPacketTime > svs.time) {
            cl->lastPacketTime = svs.time;
        }

        if (cl->state == CS_ZOMBIE && 
            cl->lastPacketTime < zombiepoint) {
            // using the client id cause the cl->name is empty at this point
            Com_DPrintf("Going from CS_ZOMBIE to CS_FREE for client %d\n", i);
            cl->state = CS_FREE; // can now be reused
            continue;
        }
        
        if (cl->state >= CS_CONNECTED && cl->lastPacketTime < droppoint) {    
            // wait several frames so a debugger session doesn't
            // cause a timeout
            if (++cl->timeoutCount > 5) {
                SV_DropClient (cl, "timed out");
                cl->state = CS_FREE; // don't bother with zombie state
            }
        } else {
            cl->timeoutCount = 0;
        }
    }
}

/////////////////////////////////////////////////////////////////////
// Name        : SV_CheckPaused
// Description : Check if the game is paused
/////////////////////////////////////////////////////////////////////
qboolean SV_CheckPaused(void) {
    
    int        i;
    int        count;
    client_t   *cl;
    
    if (!cl_paused->integer) {
        return qfalse;
    }

    // only pause if there is just a single client connected
    count = 0;
    for (i = 0, cl=svs.clients; i < sv_maxclients->integer; i++,cl++) {
        if (cl->state >= CS_CONNECTED && cl->netchan.remoteAddress.type != NA_BOT) {
            count++;
        }
    }

    if (count > 1) {
        // don't pause
        if (sv_paused->integer) {
            Cvar_Set("sv_paused", "0");
        }
        return qfalse;
    }

    if (!sv_paused->integer) {
        Cvar_Set("sv_paused", "1"); 
    }
    
    return qtrue;

}

/////////////////////////////////////////////////////////////////////
// Name        : SV_CheckDemoRecording
// Description : Check whether we are recording online players
//               if the feature is activated and we are in match mode
// Author      : Fenix
/////////////////////////////////////////////////////////////////////
void SV_CheckDemoRecording(void) { 
    
    int        i;
    int        state;
    client_t   *cl;
    
    // if the feature is disabled
    if (sv_autoDemo->integer <= 0) {
        return;
    }
    
    // match mode works only in team game modes
    if ((sv_gametype->integer < GT_TEAM && sv_gametype->integer != GT_FREEZE) || 
         sv_gametype->integer == GT_JUMP) { 
        return;
    }
    
    // get the match mode state
    state = SV_GetMatchState();
    
    // if we are not in match state
    if (!(state & MATCH_ON) || !(state & MATCH_RR) || !(state & MATCH_BR)) {
        return;
    }
    
    for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
        
        // see if we have to skip 
        if ((cl->state != CS_ACTIVE) ||                         // client not active  
            (cl->netchan.remoteAddress.type == NA_BOT) ||       // client is a bot
            (cl->demo_recording)) {                             // client is already being recorded
            continue;
        }
        
        // start the server side demo
        Cbuf_ExecuteText(EXEC_NOW, va("startserverdemo %d", (int)(cl - svs.clients)));
        
    }

}

/////////////////////////////////////////////////////////////////////
// Name        : SV_Frame
// Description : Player movement occurs as a result of packet events, 
//               which happen before SV_Frame is called
/////////////////////////////////////////////////////////////////////
void SV_Frame(int msec) {
    
    int frameMsec;
    int startTime;

    // the menu kills the server with this cvar
    if (sv_killserver->integer) {
        SV_Shutdown("Server was killed");
        Cvar_Set("sv_killserver", "0");
        return;
    }

    if (!com_sv_running->integer) {
    
        if(com_dedicated->integer) {
            // Block indefinitely until something 
            // interesting happens on STDIN.
            NET_Sleep(-1);
        }
        
        return;
    }

    // allow pause if only the local client is connected
    if (SV_CheckPaused()) {
        return;
    }

    // if it isn't time for the next frame, do nothing
    if (sv_fps->integer < 1) {
        Cvar_Set("sv_fps", "10");
    }

    frameMsec = (int) (1000 / sv_fps->integer * com_timescale->value);
    // don't let it scale below 1ms
    if (frameMsec < 1) {
        Cvar_Set("timescale", va("%f", sv_fps->integer / 1000.0f));
        frameMsec = 1;
    }

    sv.timeResidual += msec;

    if (!com_dedicated->integer) {
        SV_BotFrame (sv.time + sv.timeResidual);
    }

    if (com_dedicated->integer && sv.timeResidual < frameMsec) {
        // NET_Sleep will give the OS time slices until either get a packet
        // or time enough for a server frame has gone by
        NET_Sleep(frameMsec - sv.timeResidual);
        return;
    }

    // if time is about to hit the 32nd bit, kick all clients
    // and clear sv.time, rather
    // than checking for negative time wraparound everywhere.
    // 2giga-milliseconds = 23 days, so it won't be too often
    if (svs.time > 0x70000000) {
        SV_Shutdown("Restarting server due to time wrapping");
        Cbuf_AddText(va("map %s\n", Cvar_VariableString("mapname")));
        return;
    }
    
    // this can happen considerably earlier when lots of clients play and the map doesn't change
    if (svs.nextSnapshotEntities >= 0x7FFFFFFE - svs.numSnapshotEntities) {
        SV_Shutdown("Restarting server due to numSnapshotEntities wrapping");
        Cbuf_AddText(va("map %s\n", Cvar_VariableString("mapname")));
        return;
    }

    if(sv.restartTime && sv.time >= sv.restartTime) {
        sv.restartTime = 0;
        Cbuf_AddText("map_restart 0\n");
        return;
    }

    // update infostrings if anything has been changed
    if (cvar_modifiedFlags & CVAR_SERVERINFO) {
        SV_SetConfigstring(CS_SERVERINFO, Cvar_InfoString(CVAR_SERVERINFO));
        cvar_modifiedFlags &= ~CVAR_SERVERINFO;
    }
    
    if (cvar_modifiedFlags & CVAR_SYSTEMINFO) {
        SV_SetConfigstring(CS_SYSTEMINFO, Cvar_InfoString_Big(CVAR_SYSTEMINFO));
        cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
    }

    if (com_speeds->integer) {
        startTime = Sys_Milliseconds ();
    } else {
        startTime = 0;    // quite a compiler warning
    }

    // update ping based on the all received frames
    SV_CalcPings();

    if (com_dedicated->integer) {
        SV_BotFrame (sv.time);
    }

    // run the game simulation in chunks
    while (sv.timeResidual >= frameMsec) {
        sv.timeResidual -= frameMsec;
        svs.time += frameMsec;
        sv.time += frameMsec;
        // let everything in the world think and move
        VM_Call (gvm, GAME_RUN_FRAME, sv.time);
    }

    if (com_speeds->integer) {
        time_game = Sys_Milliseconds () - startTime;
    }

    // check timeouts
    SV_CheckTimeouts();
    
    // check user info buffer thingy
    SV_CheckClientUserinfoTimer();

    // send messages back to the clients
    SV_SendClientMessages();

    // send a heartbeat to the master if needed
    SV_MasterHeartbeat();
    
    // check that we are recording online players
    SV_CheckDemoRecording();
    
}
