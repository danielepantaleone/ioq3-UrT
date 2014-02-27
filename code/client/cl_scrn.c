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

// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

#include "client.h"

qboolean  scr_initialized;        // ready to draw

cvar_t    *cl_timegraph;
cvar_t    *cl_debuggraph;
cvar_t    *cl_graphheight;
cvar_t    *cl_graphscale;
cvar_t    *cl_graphshift;
cvar_t    *cl_drawclock;
cvar_t    *cl_keepvidaspect;

/////////////////////////////////////////////////////////////////////
// Name        : SCR_DrawNamedPic
// Description : Coordinates are 640*480 virtual values
/////////////////////////////////////////////////////////////////////
void SCR_DrawNamedPic(float x, float y, float width, float height, const char *picname) {
    
    qhandle_t    hShader;

    assert(width != 0);
    hShader = re.RegisterShader(picname);
    SCR_AdjustFrom640(&x, &y, &width, &height);
    re.DrawStretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}

/////////////////////////////////////////////////////////////////////
// Name        : SCR_AdjustFrom640
// Description : Adjusted for resolution and screen aspect ratio
/////////////////////////////////////////////////////////////////////
void SCR_AdjustFrom640(float *x, float *y, float *w, float *h) {
    
    float  xscale;
    float  yscale;
    float  xbias = 0.0;
    float  ybias = 0.0;

    xscale = cls.glconfig.vidWidth / 640.0;
    yscale = cls.glconfig.vidHeight / 480.0;
    
    if (cl_keepvidaspect->integer) {
 		if (cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640) {
			xbias = 0.5 * (cls.glconfig.vidWidth - (cls.glconfig.vidHeight * 640 / 480));
			xscale = yscale;
		} else if (cls.glconfig.vidWidth * 480 < cls.glconfig.vidHeight * 640) {
			ybias = 0.5 * (cls.glconfig.vidHeight - (cls.glconfig.vidWidth * 480.0 / 640.0));
			yscale = xscale;
 		}
	}

    if (x) {
        *x = xbias + *x * xscale;
    }
    if (y) {
        *y = ybias + *y * yscale;
    }
    if (w) {
        *w *= xscale;
    }
    if (h) {
        *h *= yscale;
    }
}

/////////////////////////////////////////////////////////////////////
// Name        : SCR_FillRect
// Description : Coordinates are 640*480 virtual values
/////////////////////////////////////////////////////////////////////
void SCR_FillRect(float x, float y, float width, float height, const float *color) {
    re.SetColor(color);
    SCR_AdjustFrom640(&x, &y, &width, &height);
    re.DrawStretchPic(x, y, width, height, 0, 0, 0, 0, cls.whiteShader);
    re.SetColor(NULL);
}

/////////////////////////////////////////////////////////////////////
// Name        : SCR_DrawPic
// Description : Coordinates are 640*480 virtual values
/////////////////////////////////////////////////////////////////////
void SCR_DrawPic(float x, float y, float width, float height, qhandle_t hShader) {
    SCR_AdjustFrom640(&x, &y, &width, &height);
    re.DrawStretchPic(x, y, width, height, 0, 0, 1, 1, hShader);
}

/////////////////////////////////////////////////////////////////////
// Name        : SCR_DrawChar
// Description : Chars are drawn at 640*480 virtual screen size
/////////////////////////////////////////////////////////////////////
static void SCR_DrawChar(int x, int y, float size, int ch) {
    
    int     row, col;
    float   frow, fcol;
    float   ax, ay, aw, ah;

    ch &= 255;

    if (ch == ' ') {
        return;
    }

    if (y < -size) {
        return;
    }

    ax = x;
    ay = y;
    aw = size;
    ah = size;
    SCR_AdjustFrom640(&ax, &ay, &aw, &ah);

    row = ch >> 4;
    col = ch & 15;

    frow = row * 0.0625;
    fcol = col * 0.0625;
    size = 0.0625;

    re.DrawStretchPic(ax, ay, aw, ah, fcol, frow, 
                      fcol + size, frow + size, cls.charSetShader);
}

/////////////////////////////////////////////////////////////////////
// Name        : SCR_DrawSmallChar
// Description : Small chars are drawn at native screen resolution
/////////////////////////////////////////////////////////////////////
void SCR_DrawSmallChar(int x, int y, int ch) {
    
    int row, col;
    float frow, fcol;
    float size;

    ch &= 255;

    if (ch == ' ') {
        return;
    }

    if (y < -SMALLCHAR_HEIGHT) {
        return;
    }

    row = ch >> 4;
    col = ch & 15;

    frow = row * 0.0625;
    fcol = col * 0.0625;
    size = 0.0625;

    re.DrawStretchPic(x, y, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, fcol, frow, 
                      fcol + size, frow + size, cls.charSetShader);
}

/////////////////////////////////////////////////////////////////////
// Name        : SCR_DrawStringExt
// Description : Coordinates are at 640 by 480 virtual resolution
/////////////////////////////////////////////////////////////////////
void SCR_DrawStringExt(int x, int y, float size, const char *string, float *setColor, qboolean forceColor) {
    
    vec4_t       color;
    const char   *s;
    int          xx;

    // draw the drop shadow
    color[0] = color[1] = color[2] = 0;
    color[3] = setColor[3];
    re.SetColor(color);
    s = string;
    xx = x;
    while (*s) {
        if (Q_IsColorString(s)) {
            s += 2;
            continue;
        }
        SCR_DrawChar(xx + 1, y + 1, size, *s);
        xx += size;
        s++;
    }

    // draw the colored text
    s = string;
    xx = x;
    re.SetColor(setColor);
    while (*s) {
        if (Q_IsColorString(s)) {
            if (!forceColor) {
                Com_Memcpy(color, g_color_table[ColorIndex(*(s+1))], sizeof(color));
                color[3] = setColor[3];
                re.SetColor(color);
            }
            s += 2;
            continue;
        }
        SCR_DrawChar(xx, y, size, *s);
        xx += size;
        s++;
    }
    re.SetColor(NULL);
}

/////////////////////////////////////////////////////////////////////
// Name        : SCR_DrawBigString
// Description : Coordinates are at 640 by 480 virtual resolution
/////////////////////////////////////////////////////////////////////
void SCR_DrawBigString(int x, int y, const char *s, float alpha) {
    float color[4];
    color[0] = color[1] = color[2] = 1.0;
    color[3] = alpha;
    SCR_DrawStringExt(x, y, BIGCHAR_WIDTH, s, color, qfalse);
}

/////////////////////////////////////////////////////////////////////
// Name        : SCR_DrawBigStringColor
// Description : Coordinates are at 640 by 480 virtual resolution
/////////////////////////////////////////////////////////////////////
void SCR_DrawBigStringColor(int x, int y, const char *s, vec4_t color) {
    SCR_DrawStringExt(x, y, BIGCHAR_WIDTH, s, color, qtrue);
}

/////////////////////////////////////////////////////////////////////
// Name        : SCR_DrawSmallStringExt
// Description : Coordinates are at 640 by 480 virtual resolution
/////////////////////////////////////////////////////////////////////
void SCR_DrawSmallStringExt(int x, int y, const char *string, float *setColor, qboolean forceColor) {
    
    vec4_t        color;
    const char    *s;
    int           xx;

    // draw the colored text
    s = string;
    xx = x;
    re.SetColor(setColor);
    while (*s) {
        if (Q_IsColorString(s)) {
            if (!forceColor) {
                Com_Memcpy(color, g_color_table[ColorIndex(*(s+1))], sizeof(color));
                color[3] = setColor[3];
                re.SetColor(color);
            }
            s += 2;
            continue;
        }
        SCR_DrawSmallChar(xx, y, *s);
        xx += SMALLCHAR_WIDTH;
        s++;
    }
    re.SetColor(NULL);
}

/////////////////////////////////////////////////////////////////////
// Name        : SCR_Strlen
// Description : Skips color escape codes
/////////////////////////////////////////////////////////////////////
static int SCR_Strlen(const char *str) {
    
    const char *s = str;
    int count = 0;

    while (*s) {
        if (Q_IsColorString(s)) {
            s += 2;
        } else {
            count++;
            s++;
        }
    }

    return count;
}

/////////////////////////////////////////////////////////////////////
// Name        : SCR_GetBigStringWidth
// Description : Skips color escape codes
/////////////////////////////////////////////////////////////////////
int SCR_GetSmallStringWidth(const char *str) {
    return SCR_Strlen(str) * SMALLCHAR_WIDTH;
}

/////////////////////////////////////////////////////////////////////
// Name        : SCR_GetBigStringWidth
// Description : Skips color escape codes
/////////////////////////////////////////////////////////////////////
int SCR_GetBigStringWidth(const char *str) {
    return SCR_Strlen(str) * BIGCHAR_WIDTH;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                          //
//  CUSTOM DRAWING                                                                                          //
//                                                                                                          //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// Name        : SCR_DrawDemoRecording
// Description : Draw the demo recording string
/////////////////////////////////////////////////////////////////////
void SCR_DrawDemoRecording(void) {
    
    int  pos;
    char string[1024];

    if (!clc.demorecording) {
        return;
    }
    
    if (clc.spDemoRecording) {
        return;
    }

    pos = FS_FTell(clc.demofile);
    Com_sprintf(string, sizeof(string), "%s[%sRECORDING%s][%s%iKB%s]", S_COLOR_WHITE, 
                (int)(cls.realtime >> 10) & 1 ? S_COLOR_RED : S_COLOR_WHITE, S_COLOR_WHITE, 
                S_COLOR_YELLOW, pos / 1024, S_COLOR_WHITE);
    
    // draw the demo notification on screen
    SCR_DrawStringExt(320 - (SCR_GetSmallStringWidth(string) / 2), 2, SMALLCHAR_WIDTH, 
                      string, g_color_table[7], qfalse);
}

/////////////////////////////////////////////////////////////////////
// Name        : SCR_DrawClock
// Description : Draw the clock
/////////////////////////////////////////////////////////////////////
void SCR_DrawClock(void) {
    
    qtime_t now;
    char    string[16];
    
    if (Cvar_VariableValue ("cl_drawclock")) {
        Com_RealTime(&now);
        Com_sprintf(string, sizeof(string), "%02i:%02i:%02i", now.tm_hour, now.tm_min, now.tm_sec);
        SCR_DrawStringExt(320 - (SCR_GetSmallStringWidth(string) / 2), 12, SMALLCHAR_WIDTH, 
                          string, g_color_table[7], qtrue);
    }
    
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                          //
//  DEBUG GRAPH                                                                                             //
//                                                                                                          //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    float    value;
    int      color;
} graphsamp_t;

static int          current;
static graphsamp_t  values[1024];

/////////////////////////////////////////////////////////////////////
// Name        : SCR_DebugGraph
/////////////////////////////////////////////////////////////////////
void SCR_DebugGraph(float value, int color) {
    values[current&1023].value = value;
    values[current&1023].color = color;
    current++;
}

/////////////////////////////////////////////////////////////////////
// Name        : SCR_DrawDebugGraph
/////////////////////////////////////////////////////////////////////
void SCR_DrawDebugGraph(void) {
    
    int      a, x, y, w, i, h;
    int      color;
    float    v;

    //
    // draw the graph
    //
    w = cls.glconfig.vidWidth;
    x = 0;
    y = cls.glconfig.vidHeight;
    re.SetColor(g_color_table[0]);
    re.DrawStretchPic(x, y - cl_graphheight->integer, w, cl_graphheight->integer, 0, 0, 0, 0, cls.whiteShader);
    re.SetColor(NULL);

    for (a=0 ; a<w ; a++) {
        i = (current - 1 - a + 1024) & 1023;
        v = values[i].value;
        color = values[i].color;
        v = v * cl_graphscale->integer + cl_graphshift->integer;
        
        if (v < 0) {
            v += cl_graphheight->integer * (1 + (int)(-v / cl_graphheight->integer));
        }
        
        h = (int)v % cl_graphheight->integer;
        re.DrawStretchPic(x+w-1-a, y - h, 1, h, 0, 0, 0, 0, cls.whiteShader);
    }
}

/////////////////////////////////////////////////////////////////////
// Name        : SCR_Init
// Description : Initialize screen settings
/////////////////////////////////////////////////////////////////////
void SCR_Init(void) {
    cl_timegraph = Cvar_Get("timegraph", "0", CVAR_CHEAT);
    cl_debuggraph = Cvar_Get("debuggraph", "0", CVAR_CHEAT);
    cl_graphheight = Cvar_Get("graphheight", "32", CVAR_CHEAT);
    cl_graphscale = Cvar_Get("graphscale", "1", CVAR_CHEAT);
    cl_graphshift = Cvar_Get("graphshift", "0", CVAR_CHEAT);
    cl_drawclock = Cvar_Get("cl_drawclock", "0", CVAR_ARCHIVE);
    cl_keepvidaspect = Cvar_Get("cl_keepvidaspect", "0", CVAR_ARCHIVE);
    scr_initialized = qtrue;
}

/////////////////////////////////////////////////////////////////////
// Name        : SCR_DrawScreenField
// Description : This will be called twice if rendering in stereo mode
/////////////////////////////////////////////////////////////////////
void SCR_DrawScreenField(stereoFrame_t stereoFrame) {
    
    re.BeginFrame(stereoFrame);

    // wide aspect ratio screens need to have the sides cleared
    // unless they are displaying game renderings
    if (cls.state != CA_ACTIVE && cls.state != CA_CINEMATIC) {
        if (cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640) {
            re.SetColor(g_color_table[0]);
            re.DrawStretchPic(0, 0, cls.glconfig.vidWidth, cls.glconfig.vidHeight, 0, 0, 0, 0, cls.whiteShader);
            re.SetColor(NULL);
        }
    }

    if (!uivm) {
        Com_DPrintf("draw screen without UI loaded\n");
        return;
    }

    // if the menu is going to cover the entire screen, we
    // don't need to render anything under it
    if (!VM_Call(uivm, UI_IS_FULLSCREEN)) {
        switch(cls.state) {
        default:
            Com_Error(ERR_FATAL, "SCR_DrawScreenField: bad cls.state");
            break;
        case CA_CINEMATIC:
            SCR_DrawCinematic();
            break;
        case CA_DISCONNECTED:
            // force menu up
            S_StopAllSounds();
            VM_Call(uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN);
            break;
        case CA_CONNECTING:
        case CA_CHALLENGING:
        case CA_CONNECTED:
            // connecting clients will only show the connection dialog
            // refresh to update the time
            VM_Call(uivm, UI_REFRESH, cls.realtime);
            VM_Call(uivm, UI_DRAW_CONNECT_SCREEN, qfalse);
            break;
        case CA_LOADING:
        case CA_PRIMED:
            // draw the game information screen and loading progress
            CL_CGameRendering(stereoFrame);

            // also draw the connection information, so it doesn't
            // flash away too briefly on local or lan games
            // refresh to update the time
            VM_Call(uivm, UI_REFRESH, cls.realtime);
            VM_Call(uivm, UI_DRAW_CONNECT_SCREEN, qtrue);
            break;
        case CA_ACTIVE:
            CL_CGameRendering(stereoFrame);
            SCR_DrawDemoRecording();
            SCR_DrawClock();
            break;
        }
    }

    // the menu draws next
    if (cls.keyCatchers & KEYCATCH_UI && uivm) {
        VM_Call(uivm, UI_REFRESH, cls.realtime);
    }

    // console draws next
    Con_DrawConsole ();

    // debug graph can be drawn on top of anything
    if (cl_debuggraph->integer || cl_timegraph->integer || cl_debugMove->integer) {
        SCR_DrawDebugGraph ();
    }
}

/////////////////////////////////////////////////////////////////////
// Name        : SCR_UpdateScreen
// Description : This is called every frame, and can also be called 
//               explicitly to flush text to the screen
/////////////////////////////////////////////////////////////////////
void SCR_UpdateScreen(void) {
    
    static int recursive;

    if (!scr_initialized) {
        return;
    }

    if (++recursive > 2) {
        Com_Error(ERR_FATAL, "SCR_UpdateScreen: recursively called");
    }
    
    recursive = 1;

    // if running in stereo, we need to draw the frame twice
    if (cls.glconfig.stereoEnabled) {
        SCR_DrawScreenField(STEREO_LEFT);
        SCR_DrawScreenField(STEREO_RIGHT);
    } else {
        SCR_DrawScreenField(STEREO_CENTER);
    }

    if (com_speeds->integer) {
        re.EndFrame(&time_frontend, &time_backend);
    } else {
        re.EndFrame(NULL, NULL);
    }

    recursive = 0;
}

