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
// sv_game.c -- interface to the game dll

#include "server.h"

#include "../botlib/botlib.h"

botlib_export_t    *botlib_export;

// these functions must be used instead of pointer arithmetic, because
// the game allocates gentities with private information after the server shared part
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
int SV_NumForGentity(sharedEntity_t *ent) {
    return (int) (((byte *)ent - (byte *)sv.gentities) / sv.gentitySize);
}
#pragma clang diagnostic pop

sharedEntity_t *SV_GentityNum(int num) {
    sharedEntity_t *ent;

    ent = (sharedEntity_t *)((byte *)sv.gentities + sv.gentitySize*(num));

    return ent;
}

playerState_t *SV_GameClientNum(int num) {
    playerState_t    *ps;

    ps = (playerState_t *)((byte *)sv.gameClients + sv.gameClientSize*(num));

    return ps;
}

svEntity_t    *SV_SvEntityForGentity(sharedEntity_t *gEnt) {
    if (!gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES) {
        Com_Error(ERR_DROP, "SV_SvEntityForGentity: bad gEnt");
        return NULL;
    }
    return &sv.svEntities[ gEnt->s.number ];
}

sharedEntity_t *SV_GEntityForSvEntity(svEntity_t *svEnt) {
    return SV_GentityNum((int) (svEnt - sv.svEntities));
}

/**
 * SV_GameSendServerCommand
 * 
 * @description Sends a command string to a client
 * @param clientNum The number of the client to send the command
 * @param text The command string to be sent
 */
void SV_GameSendServerCommand(int clientNum, const char *text) {
    
    int  i = 0;
    int  val[32];
    char cmd[MAX_NAME_LENGTH];
    char name[MAX_NAME_LENGTH];
    char auth[MAX_NAME_LENGTH];
    client_t *cl1, *cl2;
    
    if (clientNum == -1) {
                
        // scan the command looking for the ccprint one
        if(sscanf(text, "%s %i %s %i", cmd, &val[0], name, &val[1]) != EOF) {
            
            // captain status being set/unset
            if (!Q_stricmp("ccprint", cmd) && ((val[0] == MATCH_CAP) || (val[0] == MATCH_UNCAP))) {
                
                // search the player by his name
                cl1 = SV_GetPlayerByParam(name);
                if (cl1 != NULL) {
                    
                    // if someone just becausme the captain or left the captain spot, reset the flags 
                    // for everyone in his team including him: if the guy because the captain then set
                    // again the captain flag to qtrue just for him: keep consistency with qvm module
                    for (i = 0, cl2 = svs.clients; i < sv_maxclients->integer; i++, cl2++) {

                        // if the client is not active
                        if (cl2->state != CS_ACTIVE) {
                            continue;
                        }
                        
                        // if they are on different teams
                        if (SV_GetClientTeam((int) (cl1 - svs.clients)) != SV_GetClientTeam((int) (cl2 - svs.clients))) {
                            continue;
                        }

                        // set captain false
                        cl2->captain = qfalse;
                    }
                    
                    // if the guy became the captain
                    if (val[0] == MATCH_CAP) {
                        cl1->captain = qtrue;
                    }
                }                
            }
        }
        
        SV_SendServerCommand(NULL, "%s", text);
    
    } else {
        
        if (clientNum < 0 || clientNum >= sv_maxclients->integer) {
            return;
        }
        
        // scan the game command looking for the score single one
        if (sscanf(text, "%s %i %i %i %i %i %i %i %i %i %i %s", 
                   cmd, &val[0], &val[1], &val[2], &val[3], &val[4],  
                        &val[5], &val[6], &val[7], &val[8], &val[9], auth) != EOF) {

            if (!Q_stricmp("scoress", cmd)) {
                
                // set the readi flag: will cast int to qboolean
                svs.clients[val[0]].ready = (qboolean) val[6];

                #ifdef USE_AUTH
                // if the guys is authed and we didn't parse his auth already
                if ((Q_stricmp("---", auth) != 0) && (Q_stricmp(svs.clients[val[0]].auth, auth) != 0)) {
                    Q_strncpyz(svs.clients[val[0]].auth, auth, MAX_NAME_LENGTH);
                }
                #endif
            }    
        }
                                                             
        SV_SendServerCommand(svs.clients + clientNum, "%s", text);    
        
    }
    
}


/*
===============
SV_GameDropClient

Disconnects the client with a message
===============
*/
void SV_GameDropClient(int clientNum, const char *reason) {
    if (clientNum < 0 || clientNum >= sv_maxclients->integer) {
        return;
    }
    SV_DropClient(svs.clients + clientNum, reason);    
}

#ifdef USE_AUTH
/*
===============
SV_Auth_GameDropClient

Disconnects the client with a public reason and private message
===============
*/
void SV_Auth_GameDropClient(int clientNum, const char *reason, const char *message) {
    if (clientNum < 0 || clientNum >= sv_maxclients->integer) {
        return;
    }
    SV_Auth_DropClient(svs.clients + clientNum, reason, message);    
}
#endif

/*
=================
SV_SetBrushModel

sets mins and maxs for inline bmodels
=================
*/
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuninitialized"
void SV_SetBrushModel(sharedEntity_t *ent, const char *name) {

    clipHandle_t h;
    vec3_t mins, maxs;

    if (!name) {
        Com_Error(ERR_DROP, "SV_SetBrushModel: NULL");
        return;
    }

    if (name[0] != '*') {
        Com_Error(ERR_DROP, "SV_SetBrushModel: %s isn't a brush model", name);
    }


    ent->s.modelindex = atoi(name + 1);

    h = CM_InlineModel(ent->s.modelindex);
    CM_ModelBounds(h, mins, maxs);
    VectorCopy (mins, ent->r.mins);
    VectorCopy (maxs, ent->r.maxs);
    ent->r.bmodel = qtrue;

    ent->r.contents = -1;        // we don't know exactly what is in the brushes

    SV_LinkEntity(ent);        // FIXME: remove
}
#pragma clang diagnostic pop



/*
=================
SV_inPVS

Also checks portalareas so that doors block sight
=================
*/
qboolean SV_inPVS (const vec3_t p1, const vec3_t p2)
{
    int        leafnum;
    int        cluster;
    int        area1, area2;
    byte    *mask;

    leafnum = CM_PointLeafnum (p1);
    cluster = CM_LeafCluster (leafnum);
    area1 = CM_LeafArea (leafnum);
    mask = CM_ClusterPVS (cluster);

    leafnum = CM_PointLeafnum (p2);
    cluster = CM_LeafCluster (leafnum);
    area2 = CM_LeafArea (leafnum);
    if (mask && (!(mask[cluster>>3] & (1<<(cluster&7)))))
        return qfalse;
    if (!CM_AreasConnected (area1, area2))
        return qfalse;        // a door blocks sight
    return qtrue;
}


/*
=================
SV_inPVSIgnorePortals

Does NOT check portalareas
=================
*/
qboolean SV_inPVSIgnorePortals(const vec3_t p1, const vec3_t p2)
{
    int        leafnum;
    int        cluster;
    byte    *mask;

    leafnum = CM_PointLeafnum (p1);
    cluster = CM_LeafCluster (leafnum);
    mask = CM_ClusterPVS (cluster);

    leafnum = CM_PointLeafnum (p2);
    cluster = CM_LeafCluster (leafnum);

    if (mask && (!(mask[cluster>>3] & (1<<(cluster&7)))))
        return qfalse;

    return qtrue;
}


/*
========================
SV_AdjustAreaPortalState
========================
*/
void SV_AdjustAreaPortalState(sharedEntity_t *ent, qboolean open) {
    svEntity_t    *svEnt;

    svEnt = SV_SvEntityForGentity(ent);
    if (svEnt->areanum2 == -1) {
        return;
    }
    CM_AdjustAreaPortalState(svEnt->areanum, svEnt->areanum2, open);
}


/*
==================
SV_GameAreaEntities
==================
*/
qboolean    SV_EntityContact(vec3_t mins, vec3_t maxs, const sharedEntity_t *gEnt, int capsule) {
    const float    *origin, *angles;
    clipHandle_t    ch;
    trace_t            trace;

    // check for exact collision
    origin = gEnt->r.currentOrigin;
    angles = gEnt->r.currentAngles;

    ch = SV_ClipHandleForEntity(gEnt);
    CM_TransformedBoxTrace (&trace, vec3_origin, vec3_origin, mins, maxs,
        ch, -1, origin, angles, capsule);

    return trace.startsolid;
}


/*
===============
SV_GetServerinfo

===============
*/
void SV_GetServerinfo(char *buffer, int bufferSize) {
    if (bufferSize < 1) {
        Com_Error(ERR_DROP, "SV_GetServerinfo: bufferSize == %i", bufferSize);
    }
    Q_strncpyz(buffer, Cvar_InfoString(CVAR_SERVERINFO), bufferSize);
}

/*
===============
SV_LocateGameData

===============
*/
void SV_LocateGameData(sharedEntity_t *gEnts, int numGEntities, int sizeofGEntity_t,
                       playerState_t *clients, int sizeofGameClient) {
    sv.gentities = gEnts;
    sv.gentitySize = sizeofGEntity_t;
    sv.num_entities = numGEntities;

    sv.gameClients = clients;
    sv.gameClientSize = sizeofGameClient;
}


/*
===============
SV_GetUsercmd

===============
*/
void SV_GetUsercmd(int clientNum, usercmd_t *cmd) {
    if (clientNum < 0 || clientNum >= sv_maxclients->integer) {
        Com_Error(ERR_DROP, "SV_GetUsercmd: bad clientNum:%i", clientNum);
    }
    *cmd = svs.clients[clientNum].lastUsercmd;
}

//==============================================

static int    FloatAsInt(float f) {
    union
    {
        int i;
        float f;
    } temp;
    
    temp.f = f;
    return temp.i;
}

/*
====================
SV_GameSystemCalls

The module is making a system call
====================
*/
intptr_t SV_GameSystemCalls(intptr_t *args) {

    switch(args[0]) {
    case G_PRINT:
        Com_Printf("%s", (const char*)VMA(1));
        return 0;
    case G_ERROR:
        Com_Error(ERR_DROP, "%s", (const char*)VMA(1));
        return 0;
    case G_MILLISECONDS:
        return Sys_Milliseconds();
    case G_CVAR_REGISTER:
        Cvar_Register(VMA(1), VMA(2), VMA(3), (int) args[4]);
        return 0;
    case G_CVAR_UPDATE:
        Cvar_Update(VMA(1));
        return 0;
    case G_CVAR_SET:
        // exclude some game module cvars
        if (!Q_stricmp((char *)VMA(1), "sv_fps") ||
            !Q_stricmp((char *)VMA(1), "g_failedvotetime")) {
            return 0;
        }
        Cvar_Set((const char *)VMA(1), (const char *)VMA(2));
        return 0;
    case G_CVAR_VARIABLE_INTEGER_VALUE:
        return Cvar_VariableIntegerValue((const char *)VMA(1));
    case G_CVAR_VARIABLE_STRING_BUFFER:
        Cvar_VariableStringBuffer(VMA(1), VMA(2), (int) args[3]);
        return 0;
    case G_ARGC:
        return Cmd_Argc();
    case G_ARGV:
        Cmd_ArgvBuffer((int) args[1], VMA(2), (int) args[3]);
        return 0;
    case G_SEND_CONSOLE_COMMAND:
        Cbuf_ExecuteText((int) args[1], VMA(2));
        return 0;

    case G_FS_FOPEN_FILE:
        return FS_FOpenFileByMode(VMA(1), VMA(2), (fsMode_t) args[3]);
    case G_FS_READ:
        FS_Read2(VMA(1), (int) args[2], (fileHandle_t) args[3]);
        return 0;
    case G_FS_WRITE:
        FS_Write(VMA(1), (int) args[2], (fileHandle_t) args[3]);
        return 0;
    case G_FS_FCLOSE_FILE:
        FS_FCloseFile((fileHandle_t) args[1]);
        return 0;
    case G_FS_GETFILELIST:
        return FS_GetFileList(VMA(1), VMA(2), VMA(3), (int) args[4]);
    case G_FS_SEEK:
        return FS_Seek((fileHandle_t) args[1], args[2], (int) args[3]);

    case G_LOCATE_GAME_DATA:
        SV_LocateGameData(VMA(1), (int) args[2], (int) args[3], VMA(4), (int) args[5]);
        return 0;
    case G_DROP_CLIENT:
        SV_GameDropClient((int) args[1], VMA(2));
        return 0;

#ifdef USE_AUTH
    case G_AUTH_DROP_CLIENT:
        SV_Auth_GameDropClient(args[1], VMA(2), VMA(3));
        return 0;
#endif
        
    case G_SEND_SERVER_COMMAND:
        SV_GameSendServerCommand((int) args[1], VMA(2));
        return 0;
    case G_LINKENTITY:
        SV_LinkEntity(VMA(1));
        return 0;
    case G_UNLINKENTITY:
        SV_UnlinkEntity(VMA(1));
        return 0;
    case G_ENTITIES_IN_BOX:
        return SV_AreaEntities(VMA(1), VMA(2), VMA(3), (int) args[4]);
    case G_ENTITY_CONTACT:
        return SV_EntityContact(VMA(1), VMA(2), VMA(3), /*int capsule*/ qfalse);
    case G_ENTITY_CONTACTCAPSULE:
        return SV_EntityContact(VMA(1), VMA(2), VMA(3), /*int capsule*/ qtrue);
    case G_TRACE:
        SV_Trace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), (int) args[6], (int) args[7], /*int capsule*/ qfalse);
        return 0;
    case G_TRACECAPSULE:
        SV_Trace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), (int) args[6], (int) args[7], /*int capsule*/ qtrue);
        return 0;
    case G_POINT_CONTENTS:
        return SV_PointContents(VMA(1), (int) args[2]);
    case G_SET_BRUSH_MODEL:
        SV_SetBrushModel(VMA(1), VMA(2));
        return 0;
    case G_IN_PVS:
        return SV_inPVS(VMA(1), VMA(2));
    case G_IN_PVS_IGNORE_PORTALS:
        return SV_inPVSIgnorePortals(VMA(1), VMA(2));

    case G_SET_CONFIGSTRING:
        SV_SetConfigstring((int) args[1], VMA(2));
        return 0;
    case G_GET_CONFIGSTRING:
        SV_GetConfigstring((int) args[1], VMA(2), (int) args[3]);
        return 0;
    case G_SET_USERINFO:
        SV_SetUserinfo((int) args[1], VMA(2));
        return 0;
    case G_GET_USERINFO:
        SV_GetUserinfo((int) args[1], VMA(2), (int) args[3]);
        return 0;
    case G_GET_SERVERINFO:
        SV_GetServerinfo(VMA(1), (int) args[2]);
        return 0;
    case G_ADJUST_AREA_PORTAL_STATE:
        SV_AdjustAreaPortalState(VMA(1), (qboolean) args[2]);
        return 0;
    case G_AREAS_CONNECTED:
        return CM_AreasConnected((int) args[1], (int) args[2]);

    case G_BOT_ALLOCATE_CLIENT:
        return SV_BotAllocateClient();
    case G_BOT_FREE_CLIENT:
        SV_BotFreeClient((int) args[1]);
        return 0;

    case G_GET_USERCMD:
        SV_GetUsercmd((int) args[1], VMA(2));
        return 0;
    case G_GET_ENTITY_TOKEN:
        {
            const char    *s;

            s = COM_Parse(&sv.entityParsePoint);
            Q_strncpyz(VMA(1), s, (int) args[2]);
            if (!sv.entityParsePoint && !s[0]) {
                return qfalse;
            } else {
                return qtrue;
            }
        }

    case G_DEBUG_POLYGON_CREATE:
        return BotImport_DebugPolygonCreate((int) args[1], (int) args[2], VMA(3));
    case G_DEBUG_POLYGON_DELETE:
        BotImport_DebugPolygonDelete((int) args[1]);
        return 0;
    case G_REAL_TIME:
        return Com_RealTime(VMA(1));
    case G_SNAPVECTOR:
        Sys_SnapVector(VMA(1));
        return 0;

        //====================================

    case BOTLIB_SETUP:
        return SV_BotLibSetup();
    case BOTLIB_SHUTDOWN:
        return SV_BotLibShutdown();
    case BOTLIB_LIBVAR_SET:
        return botlib_export->BotLibVarSet(VMA(1), VMA(2));
    case BOTLIB_LIBVAR_GET:
        return botlib_export->BotLibVarGet(VMA(1), VMA(2), (int) args[3]);

    case BOTLIB_PC_ADD_GLOBAL_DEFINE:
        return botlib_export->PC_AddGlobalDefine(VMA(1));
    case BOTLIB_PC_LOAD_SOURCE:
        return botlib_export->PC_LoadSourceHandle(VMA(1));
    case BOTLIB_PC_FREE_SOURCE:
        return botlib_export->PC_FreeSourceHandle((int) args[1]);
    case BOTLIB_PC_READ_TOKEN:
        return botlib_export->PC_ReadTokenHandle((int) args[1], VMA(2));
    case BOTLIB_PC_SOURCE_FILE_AND_LINE:
        return botlib_export->PC_SourceFileAndLine((int) args[1], VMA(2), VMA(3));

    case BOTLIB_START_FRAME:
        return botlib_export->BotLibStartFrame(VMF(1));
    case BOTLIB_LOAD_MAP:
        return botlib_export->BotLibLoadMap(VMA(1));
    case BOTLIB_UPDATENTITY:
        return botlib_export->BotLibUpdateEntity((int) args[1], VMA(2));
    case BOTLIB_TEST:
        return botlib_export->Test((int) args[1], VMA(2), VMA(3), VMA(4));

    case BOTLIB_GET_SNAPSHOT_ENTITY:
        return SV_BotGetSnapshotEntity((int) args[1], (int) args[2]);
    case BOTLIB_GET_CONSOLE_MESSAGE:
        return SV_BotGetConsoleMessage((int) args[1], VMA(2), (int) args[3]);
    case BOTLIB_USER_COMMAND:
        SV_ClientThink(&svs.clients[args[1]], VMA(2));
        return 0;

    case BOTLIB_AAS_BBOX_AREAS:
        return botlib_export->aas.AAS_BBoxAreas(VMA(1), VMA(2), VMA(3), (int) args[4]);
    case BOTLIB_AAS_AREA_INFO:
        return botlib_export->aas.AAS_AreaInfo((int) args[1], VMA(2));
    case BOTLIB_AAS_ALTERNATIVE_ROUTE_GOAL:
        return botlib_export->aas.AAS_AlternativeRouteGoals(VMA(1), (int) args[2], VMA(3), (int) args[4], (int) args[5], VMA(6), (int) args[7], (int) args[8]);
    case BOTLIB_AAS_ENTITY_INFO:
        botlib_export->aas.AAS_EntityInfo((int) args[1], VMA(2));
        return 0;

    case BOTLIB_AAS_INITIALIZED:
        return botlib_export->aas.AAS_Initialized();
    case BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX:
        botlib_export->aas.AAS_PresenceTypeBoundingBox((int) args[1], VMA(2), VMA(3));
        return 0;
    case BOTLIB_AAS_TIME:
        return FloatAsInt(botlib_export->aas.AAS_Time());

    case BOTLIB_AAS_POINT_AREA_NUM:
        return botlib_export->aas.AAS_PointAreaNum(VMA(1));
    case BOTLIB_AAS_POINT_REACHABILITY_AREA_INDEX:
        return botlib_export->aas.AAS_PointReachabilityAreaIndex(VMA(1));
    case BOTLIB_AAS_TRACE_AREAS:
        return botlib_export->aas.AAS_TraceAreas(VMA(1), VMA(2), VMA(3), VMA(4), (int) args[5]);

    case BOTLIB_AAS_POINT_CONTENTS:
        return botlib_export->aas.AAS_PointContents(VMA(1));
    case BOTLIB_AAS_NEXT_BSP_ENTITY:
        return botlib_export->aas.AAS_NextBSPEntity((int) args[1]);
    case BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY:
        return botlib_export->aas.AAS_ValueForBSPEpairKey((int) args[1], VMA(2), VMA(3), (int) args[4]);
    case BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY:
        return botlib_export->aas.AAS_VectorForBSPEpairKey((int) args[1], VMA(2), VMA(3));
    case BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY:
        return botlib_export->aas.AAS_FloatForBSPEpairKey((int) args[1], VMA(2), VMA(3));
    case BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY:
        return botlib_export->aas.AAS_IntForBSPEpairKey((int) args[1], VMA(2), VMA(3));

    case BOTLIB_AAS_AREA_REACHABILITY:
        return botlib_export->aas.AAS_AreaReachability((int) args[1]);

    case BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA:
        return botlib_export->aas.AAS_AreaTravelTimeToGoalArea((int) args[1], VMA(2), (int) args[3], (int) args[4]);
    case BOTLIB_AAS_ENABLE_ROUTING_AREA:
        return botlib_export->aas.AAS_EnableRoutingArea((int) args[1], (int) args[2]);
    case BOTLIB_AAS_PREDICT_ROUTE:
        return botlib_export->aas.AAS_PredictRoute(VMA(1), (int) args[2], VMA(3), (int) args[4], (int) args[5], (int) args[6],
                                                   (int) args[7], (int) args[8], (int) args[9], (int) args[10], (int) args[11]);

    case BOTLIB_AAS_SWIMMING:
        return botlib_export->aas.AAS_Swimming(VMA(1));
    case BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT:
        return botlib_export->aas.AAS_PredictClientMovement(VMA(1), (int) args[2], VMA(3), (int) args[4], (int) args[5],
                                                            VMA(6), VMA(7), (int) args[8], (int) args[9], VMF(10),
                                                            (int) args[11], (int) args[12], (int) args[13]);

    case BOTLIB_EA_SAY:
        botlib_export->ea.EA_Say((int) args[1], VMA(2));
        return 0;
    case BOTLIB_EA_SAY_TEAM:
        botlib_export->ea.EA_SayTeam((int) args[1], VMA(2));
        return 0;
    case BOTLIB_EA_COMMAND:
        botlib_export->ea.EA_Command((int) args[1], VMA(2));
        return 0;

    case BOTLIB_EA_ACTION:
        botlib_export->ea.EA_Action((int) args[1], (int) args[2]);
        break;
    case BOTLIB_EA_GESTURE:
        botlib_export->ea.EA_Gesture((int) args[1]);
        return 0;
    case BOTLIB_EA_TALK:
        botlib_export->ea.EA_Talk((int) args[1]);
        return 0;
    case BOTLIB_EA_ATTACK:
        botlib_export->ea.EA_Attack((int) args[1]);
        return 0;
    case BOTLIB_EA_USE:
        botlib_export->ea.EA_Use((int) args[1]);
        return 0;
    case BOTLIB_EA_RESPAWN:
        botlib_export->ea.EA_Respawn((int) args[1]);
        return 0;
    case BOTLIB_EA_CROUCH:
        botlib_export->ea.EA_Crouch((int) args[1]);
        return 0;
    case BOTLIB_EA_MOVE_UP:
        botlib_export->ea.EA_MoveUp((int) args[1]);
        return 0;
    case BOTLIB_EA_MOVE_DOWN:
        botlib_export->ea.EA_MoveDown((int) args[1]);
        return 0;
    case BOTLIB_EA_MOVE_FORWARD:
        botlib_export->ea.EA_MoveForward((int) args[1]);
        return 0;
    case BOTLIB_EA_MOVE_BACK:
        botlib_export->ea.EA_MoveBack((int) args[1]);
        return 0;
    case BOTLIB_EA_MOVE_LEFT:
        botlib_export->ea.EA_MoveLeft((int) args[1]);
        return 0;
    case BOTLIB_EA_MOVE_RIGHT:
        botlib_export->ea.EA_MoveRight((int) args[1]);
        return 0;

    case BOTLIB_EA_SELECT_WEAPON:
        botlib_export->ea.EA_SelectWeapon((int) args[1], (int) args[2]);
        return 0;
    case BOTLIB_EA_JUMP:
        botlib_export->ea.EA_Jump((int) args[1]);
        return 0;
    case BOTLIB_EA_DELAYED_JUMP:
        botlib_export->ea.EA_DelayedJump((int) args[1]);
        return 0;
    case BOTLIB_EA_MOVE:
        botlib_export->ea.EA_Move((int) args[1], VMA(2), VMF(3));
        return 0;
    case BOTLIB_EA_VIEW:
        botlib_export->ea.EA_View((int) args[1], VMA(2));
        return 0;

    case BOTLIB_EA_END_REGULAR:
        botlib_export->ea.EA_EndRegular((int) args[1], VMF(2));
        return 0;
    case BOTLIB_EA_GET_INPUT:
        botlib_export->ea.EA_GetInput((int) args[1], VMF(2), VMA(3));
        return 0;
    case BOTLIB_EA_RESET_INPUT:
        botlib_export->ea.EA_ResetInput((int) args[1]);
        return 0;

    case BOTLIB_AI_LOAD_CHARACTER:
        return botlib_export->ai.BotLoadCharacter(VMA(1), VMF(2));
    case BOTLIB_AI_FREE_CHARACTER:
        botlib_export->ai.BotFreeCharacter((int) args[1]);
        return 0;
    case BOTLIB_AI_CHARACTERISTIC_FLOAT:
        return FloatAsInt(botlib_export->ai.Characteristic_Float((int) args[1], (int) args[2]));
    case BOTLIB_AI_CHARACTERISTIC_BFLOAT:
        return FloatAsInt(botlib_export->ai.Characteristic_BFloat((int) args[1], (int) args[2], VMF(3), VMF(4)));
    case BOTLIB_AI_CHARACTERISTIC_INTEGER:
        return botlib_export->ai.Characteristic_Integer((int) args[1], (int) args[2]);
    case BOTLIB_AI_CHARACTERISTIC_BINTEGER:
        return botlib_export->ai.Characteristic_BInteger((int) args[1], (int) args[2], (int) args[3], (int) args[4]);
    case BOTLIB_AI_CHARACTERISTIC_STRING:
        botlib_export->ai.Characteristic_String((int) args[1], (int) args[2], VMA(3), (int) args[4]);
        return 0;

    case BOTLIB_AI_ALLOC_CHAT_STATE:
        return botlib_export->ai.BotAllocChatState();
    case BOTLIB_AI_FREE_CHAT_STATE:
        botlib_export->ai.BotFreeChatState((int) args[1]);
        return 0;
    case BOTLIB_AI_QUEUE_CONSOLE_MESSAGE:
        botlib_export->ai.BotQueueConsoleMessage((int) args[1], (int) args[2], VMA(3));
        return 0;
    case BOTLIB_AI_REMOVE_CONSOLE_MESSAGE:
        botlib_export->ai.BotRemoveConsoleMessage((int) args[1], (int) args[2]);
        return 0;
    case BOTLIB_AI_NEXT_CONSOLE_MESSAGE:
        return botlib_export->ai.BotNextConsoleMessage((int) args[1], VMA(2));
    case BOTLIB_AI_NUM_CONSOLE_MESSAGE:
        return botlib_export->ai.BotNumConsoleMessages((int) args[1]);
    case BOTLIB_AI_INITIAL_CHAT:
        botlib_export->ai.BotInitialChat((int) args[1], VMA(2), (int) args[3], VMA(4), VMA(5), VMA(6), VMA(7), VMA(8), VMA(9), VMA(10), VMA(11));
        return 0;
    case BOTLIB_AI_NUM_INITIAL_CHATS:
        return botlib_export->ai.BotNumInitialChats((int) args[1], VMA(2));
    case BOTLIB_AI_REPLY_CHAT:
        return botlib_export->ai.BotReplyChat((int) args[1], VMA(2), (int) args[3], (int) args[4], VMA(5), VMA(6), VMA(7), VMA(8), VMA(9), VMA(10), VMA(11), VMA(12));
    case BOTLIB_AI_CHAT_LENGTH:
        return botlib_export->ai.BotChatLength((int) args[1]);
    case BOTLIB_AI_ENTER_CHAT:
        botlib_export->ai.BotEnterChat((int) args[1], (int) args[2], (int) args[3]);
        return 0;
    case BOTLIB_AI_GET_CHAT_MESSAGE:
        botlib_export->ai.BotGetChatMessage((int) args[1], VMA(2), (int) args[3]);
        return 0;
    case BOTLIB_AI_STRING_CONTAINS:
        return botlib_export->ai.StringContains(VMA(1), VMA(2), (int) args[3]);
    case BOTLIB_AI_FIND_MATCH:
        return botlib_export->ai.BotFindMatch(VMA(1), VMA(2), (unsigned long) args[3]);
    case BOTLIB_AI_MATCH_VARIABLE:
        botlib_export->ai.BotMatchVariable(VMA(1), (int) args[2], VMA(3), (int) args[4]);
        return 0;
    case BOTLIB_AI_UNIFY_WHITE_SPACES:
        botlib_export->ai.UnifyWhiteSpaces(VMA(1));
        return 0;
    case BOTLIB_AI_REPLACE_SYNONYMS:
        botlib_export->ai.BotReplaceSynonyms(VMA(1), (unsigned long) args[2]);
        return 0;
    case BOTLIB_AI_LOAD_CHAT_FILE:
        return botlib_export->ai.BotLoadChatFile((int) args[1], VMA(2), VMA(3));
    case BOTLIB_AI_SET_CHAT_GENDER:
        botlib_export->ai.BotSetChatGender((int) args[1], (int) args[2]);
        return 0;
    case BOTLIB_AI_SET_CHAT_NAME:
        botlib_export->ai.BotSetChatName((int) args[1], VMA(2), (int) args[3]);
        return 0;

    case BOTLIB_AI_RESET_GOAL_STATE:
        botlib_export->ai.BotResetGoalState((int) args[1]);
        return 0;
    case BOTLIB_AI_RESET_AVOID_GOALS:
        botlib_export->ai.BotResetAvoidGoals((int) args[1]);
        return 0;
    case BOTLIB_AI_REMOVE_FROM_AVOID_GOALS:
        botlib_export->ai.BotRemoveFromAvoidGoals((int) args[1], (int) args[2]);
        return 0;
    case BOTLIB_AI_PUSH_GOAL:
        botlib_export->ai.BotPushGoal((int) args[1], VMA(2));
        return 0;
    case BOTLIB_AI_POP_GOAL:
        botlib_export->ai.BotPopGoal((int) args[1]);
        return 0;
    case BOTLIB_AI_EMPTY_GOAL_STACK:
        botlib_export->ai.BotEmptyGoalStack((int) args[1]);
        return 0;
    case BOTLIB_AI_DUMP_AVOID_GOALS:
        botlib_export->ai.BotDumpAvoidGoals((int) args[1]);
        return 0;
    case BOTLIB_AI_DUMP_GOAL_STACK:
        botlib_export->ai.BotDumpGoalStack((int) args[1]);
        return 0;
    case BOTLIB_AI_GOAL_NAME:
        botlib_export->ai.BotGoalName((int) args[1], VMA(2), (int) args[3]);
        return 0;
    case BOTLIB_AI_GET_TOP_GOAL:
        return botlib_export->ai.BotGetTopGoal((int) args[1], VMA(2));
    case BOTLIB_AI_GET_SECOND_GOAL:
        return botlib_export->ai.BotGetSecondGoal((int) args[1], VMA(2));
    case BOTLIB_AI_CHOOSE_LTG_ITEM:
        return botlib_export->ai.BotChooseLTGItem((int) args[1], VMA(2), VMA(3), (int) args[4]);
    case BOTLIB_AI_CHOOSE_NBG_ITEM:
        return botlib_export->ai.BotChooseNBGItem((int) args[1], VMA(2), VMA(3), (int) args[4], VMA(5), VMF(6));
    case BOTLIB_AI_TOUCHING_GOAL:
        return botlib_export->ai.BotTouchingGoal(VMA(1), VMA(2));
    case BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE:
        return botlib_export->ai.BotItemGoalInVisButNotVisible((int) args[1], VMA(2), VMA(3), VMA(4));
    case BOTLIB_AI_GET_LEVEL_ITEM_GOAL:
        return botlib_export->ai.BotGetLevelItemGoal((int) args[1], VMA(2), VMA(3));
    case BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL:
        return botlib_export->ai.BotGetNextCampSpotGoal((int) args[1], VMA(2));
    case BOTLIB_AI_GET_MAP_LOCATION_GOAL:
        return botlib_export->ai.BotGetMapLocationGoal(VMA(1), VMA(2));
    case BOTLIB_AI_AVOID_GOAL_TIME:
        return FloatAsInt(botlib_export->ai.BotAvoidGoalTime((int) args[1], (int) args[2]));
    case BOTLIB_AI_SET_AVOID_GOAL_TIME:
        botlib_export->ai.BotSetAvoidGoalTime((int) args[1], (int) args[2], VMF(3));
        return 0;
    case BOTLIB_AI_INIT_LEVEL_ITEMS:
        botlib_export->ai.BotInitLevelItems();
        return 0;
    case BOTLIB_AI_UPDATE_ENTITY_ITEMS:
        botlib_export->ai.BotUpdateEntityItems();
        return 0;
    case BOTLIB_AI_LOAD_ITEM_WEIGHTS:
        return botlib_export->ai.BotLoadItemWeights((int) args[1], VMA(2));
    case BOTLIB_AI_FREE_ITEM_WEIGHTS:
        botlib_export->ai.BotFreeItemWeights((int) args[1]);
        return 0;
    case BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC:
        botlib_export->ai.BotInterbreedGoalFuzzyLogic((int) args[1], (int) args[2], (int) args[3]);
        return 0;
    case BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC:
        botlib_export->ai.BotSaveGoalFuzzyLogic((int) args[1], VMA(2));
        return 0;
    case BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC:
        botlib_export->ai.BotMutateGoalFuzzyLogic((int) args[1], VMF(2));
        return 0;
    case BOTLIB_AI_ALLOC_GOAL_STATE:
        return botlib_export->ai.BotAllocGoalState((int) args[1]);
    case BOTLIB_AI_FREE_GOAL_STATE:
        botlib_export->ai.BotFreeGoalState((int) args[1]);
        return 0;

    case BOTLIB_AI_RESET_MOVE_STATE:
        botlib_export->ai.BotResetMoveState((int) args[1]);
        return 0;
    case BOTLIB_AI_ADD_AVOID_SPOT:
        botlib_export->ai.BotAddAvoidSpot((int) args[1], VMA(2), VMF(3), (int) args[4]);
        return 0;
    case BOTLIB_AI_MOVE_TO_GOAL:
        botlib_export->ai.BotMoveToGoal(VMA(1), (int) args[2], VMA(3), (int) args[4]);
        return 0;
    case BOTLIB_AI_MOVE_IN_DIRECTION:
        return botlib_export->ai.BotMoveInDirection((int) args[1], VMA(2), VMF(3), (int) args[4]);
    case BOTLIB_AI_RESET_AVOID_REACH:
        botlib_export->ai.BotResetAvoidReach((int) args[1]);
        return 0;
    case BOTLIB_AI_RESET_LAST_AVOID_REACH:
        botlib_export->ai.BotResetLastAvoidReach((int) args[1]);
        return 0;
    case BOTLIB_AI_REACHABILITY_AREA:
        return botlib_export->ai.BotReachabilityArea(VMA(1), (int) args[2]);
    case BOTLIB_AI_MOVEMENT_VIEW_TARGET:
        return botlib_export->ai.BotMovementViewTarget((int) args[1], VMA(2), (int) args[3], VMF(4), VMA(5));
    case BOTLIB_AI_PREDICT_VISIBLE_POSITION:
        return botlib_export->ai.BotPredictVisiblePosition(VMA(1), (int) args[2], VMA(3), (int) args[4], VMA(5));
    case BOTLIB_AI_ALLOC_MOVE_STATE:
        return botlib_export->ai.BotAllocMoveState();
    case BOTLIB_AI_FREE_MOVE_STATE:
        botlib_export->ai.BotFreeMoveState((int) args[1]);
        return 0;
    case BOTLIB_AI_INIT_MOVE_STATE:
        botlib_export->ai.BotInitMoveState((int) args[1], VMA(2));
        return 0;

    case BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON:
        return botlib_export->ai.BotChooseBestFightWeapon((int) args[1], VMA(2));
    case BOTLIB_AI_GET_WEAPON_INFO:
        botlib_export->ai.BotGetWeaponInfo((int) args[1], (int) args[2], VMA(3));
        return 0;
    case BOTLIB_AI_LOAD_WEAPON_WEIGHTS:
        return botlib_export->ai.BotLoadWeaponWeights((int) args[1], VMA(2));
    case BOTLIB_AI_ALLOC_WEAPON_STATE:
        return botlib_export->ai.BotAllocWeaponState();
    case BOTLIB_AI_FREE_WEAPON_STATE:
        botlib_export->ai.BotFreeWeaponState((int) args[1]);
        return 0;
    case BOTLIB_AI_RESET_WEAPON_STATE:
        botlib_export->ai.BotResetWeaponState((int) args[1]);
        return 0;

    case BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION:
        return botlib_export->ai.GeneticParentsAndChildSelection((int) args[1], VMA(2), VMA(3), VMA(4), VMA(5));

    //@Barbatos
    #ifdef USE_AUTH
    case G_NET_STRINGTOADR:
        return NET_StringToAdr(VMA(1), VMA(2));
        
    case G_NET_SENDPACKET:
        {
            netadr_t addr;
            const char * destination = VMA(4);     
            
            NET_StringToAdr(destination, &addr);                                                                                                                                                                                                                                   
            NET_SendPacket(args[1], args[2], VMA(3), addr); 
        }
        return 0;
    
    //case G_SYS_STARTPROCESS:
    //    Sys_StartProcess(VMA(1), VMA(2));
    //    return 0;
        
    #endif
    
    case TRAP_MEMSET:
        Com_Memset(VMA(1), args[2], args[3]);
        return 0;

    case TRAP_MEMCPY:
        Com_Memcpy(VMA(1), VMA(2), args[3]);
        return 0;

    case TRAP_STRNCPY:
        strncpy(VMA(1), VMA(2), args[3]);
        return args[1];

    case TRAP_SIN:
        return FloatAsInt((float) sin(VMF(1)));

    case TRAP_COS:
        return FloatAsInt((float) cos(VMF(1)));

    case TRAP_ATAN2:
        return FloatAsInt((float) atan2(VMF(1), VMF(2)));

    case TRAP_SQRT:
        return FloatAsInt((float) sqrt(VMF(1)));

    case TRAP_MATRIXMULTIPLY:
        MatrixMultiply(VMA(1), VMA(2), VMA(3));
        return 0;

    case TRAP_ANGLEVECTORS:
        AngleVectors(VMA(1), VMA(2), VMA(3), VMA(4));
        return 0;

    case TRAP_PERPENDICULARVECTOR:
        PerpendicularVector(VMA(1), VMA(2));
        return 0;

    case TRAP_FLOOR:
        return FloatAsInt((float) floor(VMF(1)));

    case TRAP_CEIL:
        return FloatAsInt((float) ceil(VMF(1)));

    default:
        Com_Error(ERR_DROP, "Bad game system trap: %ld", (long int) args[0]);
        break;
    }

    return -1;
}

/*
===============
SV_ShutdownGameProgs

Called every time a map changes
===============
*/
void SV_ShutdownGameProgs(void) {
    if (!gvm) {
        return;
    }
    VM_Call(gvm, GAME_SHUTDOWN, qfalse);
    VM_Free(gvm);
    gvm = NULL;
}

/*
==================
SV_InitGameVM

Called for both a full init and a restart
==================
*/
static void SV_InitGameVM(qboolean restart) {
    int        i;

    // start the entity parsing at the beginning
    sv.entityParsePoint = CM_EntityString();

    // clear all gentity pointers that might still be set from
    // a previous level
    // https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=522
    //   now done before GAME_INIT call
    for (i = 0 ; i < sv_maxclients->integer ; i++) {
        svs.clients[i].gentity = NULL;
    }
    
    // use the current msec count for a random seed
    // init for this gamestate
    VM_Call (gvm, GAME_INIT, sv.time, Com_Milliseconds(), restart);
}



/*
===================
SV_RestartGameProgs

Called on a map_restart, but not on a normal map change
===================
*/
void SV_RestartGameProgs(void) {
    if (!gvm) {
        return;
    }
    VM_Call(gvm, GAME_SHUTDOWN, qtrue);

    // do a restart instead of a free
    gvm = VM_Restart(gvm);
    if (!gvm) {
        Com_Error(ERR_FATAL, "VM_Restart on game failed");
    }

    SV_InitGameVM(qtrue);
}


/*
===============
SV_InitGameProgs

Called on a normal map change, not on a map_restart
===============
*/
void SV_InitGameProgs(void) {
    int       i;
    cvar_t    *var;
    //FIXME these are temp while I make bots run in vm
    extern int    bot_enable;

    var = Cvar_Get("bot_enable", "1", CVAR_LATCH);
    if (var) {
        bot_enable = var->integer;
    }
    else {
        bot_enable = 0;
    }
    
    for ( i = 0 ; i < MAX_MASTER_SERVERS ; i++ ) {
		if ( !sv_master[i]->string[0] ) {
			continue;
		}
		sv_master[i]->modified = qtrue;
	}
    
    // load the dll or bytecode
    gvm = VM_Create("qagame", SV_GameSystemCalls, Cvar_VariableValue("vm_game"));
    if (!gvm) {
        Com_Error(ERR_FATAL, "VM_Create on game failed");
    }

    SV_InitGameVM(qfalse);
}


/*
====================
SV_GameCommand

See if the current console command is claimed by the game
====================
*/
qboolean SV_GameCommand(void) {
    if (sv.state != SS_GAME)
        return qfalse;
    return (qboolean) VM_Call(gvm, GAME_CONSOLE_COMMAND);
}

