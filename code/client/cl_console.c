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
// console.c

#include "client.h"


int g_console_field_width = 78;


#define NUM_CON_TIMES  4
#define CON_TEXTSIZE   32768

#define BOX_MARGIN      30
#define CON_MARGIN      8
#define CON_PADDING     4

int con_width    = SCREEN_WIDTH;
int con_marginX  = 0;
int con_marginY  = 0;
int con_paddingX = 0;
int con_paddingY = 0;

float opacityMult = 1;
float targetOpacityMult = 1;

qboolean ignoreNewline = qtrue;

typedef struct {
    qboolean    initialized;

    short       text[CON_TEXTSIZE];
    int         current;                    // line where next message will be printed
    int         x;                          // offset in current line for next print
    int         display;                    // bottom of console displays this line

    int         linewidth;                  // characters across screen
    int         totallines;                 // total lines in console scrollback

    float       xadjust;                    // for wide aspect screens
    float       displayFrac;                // aproaches finalFrac at scr_conspeed
    float       finalFrac;                  // 0.0 to 1.0 lines of console to display

    int         vislines;                   // in scanlines
    int         times[NUM_CON_TIMES];       // cls.realtime time the line was generated
                                            // for transparent notify lines
    vec4_t      color;
} console_t;

extern console_t con;
console_t        con;

cvar_t   *con_bgAlpha;
cvar_t   *con_notifytime;

#define  DEFAULT_CONSOLE_WIDTH  78

vec4_t   console_color = {1.0, 1.0, 1.0, 1.0};

void Con_RE_SetColor(vec4_t colour) {
    vec4_t c;
    if (colour) {
        c[0] = colour[0];
        c[1] = colour[1];
        c[2] = colour[2];
        c[3] = colour[3] * opacityMult;
        re.SetColor(c);
    } else {
        re.SetColor(NULL);
    }
}

void SCR_AdjustedFillRect(float x, float y, float width, float height, const float *color) {
    vec4_t c;
    if (color) {
        c[0] = color[0];
        c[1] = color[1];
        c[2] = color[2];
        c[3] = color[3] * opacityMult;
    } else {
        c[0] = 1;
        c[1] = 1;
        c[2] = 1;
        c[3] = opacityMult;
    }

    SCR_FillRect(x, y, width, height, c);
}

void SCR_AdjustedDrawString(int x, int y, float size, const char *string, float *setColor, qboolean forceColor) {
    vec4_t c;
    if (setColor) {
        c[0] = setColor[0];
        c[1] = setColor[1];
        c[2] = setColor[2];
        c[3] = setColor[3] * opacityMult;
    } else {
        c[0] = 1;
        c[1] = 1;
        c[2] = 1;
        c[3] = opacityMult;
    }
    SCR_DrawStringExt(x, y, size, string, c, forceColor);
}

/////////////////////////////////////////////////////////////////////
// Name        : Con_ToggleConsole_f
// Description : Toggle the console
/////////////////////////////////////////////////////////////////////
void Con_ToggleConsole_f (void) {
    
    // closing a full screen console restarts the demo loop
    if (cls.state == CA_DISCONNECTED && cls.keyCatchers == KEYCATCH_CONSOLE) {
        CL_StartDemoLoop();
        return;
    }

    Field_Clear(&g_consoleField);
    g_consoleField.widthInChars = g_console_field_width;

    Con_ClearNotify ();
    cls.keyCatchers ^= KEYCATCH_CONSOLE;
}

/////////////////////////////////////////////////////////////////////
// Name : Con_MessageMode_f
/////////////////////////////////////////////////////////////////////
void Con_MessageMode_f (void) {
    chat_playerNum = -1;
    chat_team = qfalse;
    Field_Clear(&chatField);
    chatField.widthInChars = 30;
    cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/////////////////////////////////////////////////////////////////////
// Name : Con_MessageMode2_f
/////////////////////////////////////////////////////////////////////
void Con_MessageMode2_f (void) {
    chat_playerNum = -1;
    chat_team = qtrue;
    Field_Clear(&chatField);
    chatField.widthInChars = 25;
    cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/////////////////////////////////////////////////////////////////////
// Name : Con_MessageMode3_f
/////////////////////////////////////////////////////////////////////
void Con_MessageMode3_f (void) {
    chat_playerNum = VM_Call(cgvm, CG_CROSSHAIR_PLAYER);
    if (chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS) {
        chat_playerNum = -1;
        return;
    }
    chat_team = qfalse;
    Field_Clear(&chatField);
    chatField.widthInChars = 30;
    cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/////////////////////////////////////////////////////////////////////
// Name : Con_MessageMode4_f
/////////////////////////////////////////////////////////////////////
void Con_MessageMode4_f (void) {
    chat_playerNum = VM_Call(cgvm, CG_LAST_ATTACKER);
    if (chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS) {
        chat_playerNum = -1;
        return;
    }
    chat_team = qfalse;
    Field_Clear(&chatField);
    chatField.widthInChars = 30;
    cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/////////////////////////////////////////////////////////////////////
// Name : Con_Clear_f
/////////////////////////////////////////////////////////////////////
void Con_Clear_f(void) {
    int i;

    for (i = 0 ; i < CON_TEXTSIZE ; i++) {
        con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
    }

    Con_Bottom();
}

/////////////////////////////////////////////////////////////////////
// Name        : Con_Dump_f
// Description : Save the console contents out to a file
/////////////////////////////////////////////////////////////////////
void Con_Dump_f(void) {
    
    int             l, x, i;
    short           *line;
    fileHandle_t    f;
    char            buffer[1024];

    if (Cmd_Argc() != 2) {
        Com_Printf("Usage: condump <filename>\n");
        return;
    }
    
    f = FS_FOpenFileWrite(Cmd_Argv(1));
    if (!f) {
        Com_Printf("ERROR: couldn't open %s.\n", Cmd_Argv(1));
        return;
    }

    // skip empty lines
    for (l = con.current - con.totallines + 1 ; l <= con.current ; l++) {
        line = con.text + (l%con.totallines)*con.linewidth;
        for (x = 0 ; x < con.linewidth ; x++)
        if ((line[x] & 0xff) != ' ') {
            break;
        }
        if (x != con.linewidth) {
            break;
        }
    }

    // write the remaining lines
    buffer[con.linewidth] = 0;
    for (; l <= con.current ; l++) {
        
        line = con.text + (l%con.totallines)*con.linewidth;
        for (i=0; i<con.linewidth; i++) {
            buffer[i] = line[i] & 0xff;
        }
        
        for (x=con.linewidth-1 ; x>=0 ; x--) {
            if (buffer[x] == ' ')
                buffer[x] = 0;
            else
                break;
        }
        
        strcat(buffer, "\n");
        FS_Write(buffer, strlen(buffer), f);
    }

    FS_FCloseFile(f);
    Com_Printf("Dumped console text to %s.\n", Cmd_Argv(1));
    
}

/////////////////////////////////////////////////////////////////////
// Name : Con_ClearNotify
/////////////////////////////////////////////////////////////////////
void Con_ClearNotify(void) {
    int i;
    for (i = 0 ; i < NUM_CON_TIMES ; i++) {
        con.times[i] = 0;
    }
}

/////////////////////////////////////////////////////////////////////
// Name        : Con_CheckResize
// Description : If the line width has changed, reformat the buffer
/////////////////////////////////////////////////////////////////////
void Con_CheckResize (void) {
    
    int      i, j, width, oldwidth, oldtotallines, numlines, numchars;
    short    tbuf[CON_TEXTSIZE];

    width = (SCREEN_WIDTH / SMALLCHAR_WIDTH) * 2;

    if (width == con.linewidth) {
        return;
    }

    if (width < 1) { // video hasn't been initialized yet
        
        width = DEFAULT_CONSOLE_WIDTH;
        con.linewidth = width;
        con.totallines = CON_TEXTSIZE / con.linewidth;
        for(i = 0; i < CON_TEXTSIZE; i++) {
            con.text[i] = (ColorIndex(COLOR_WHITE) << 8) | ' ';
        }
        
    } else {
        
        oldwidth = con.linewidth;
        con.linewidth = width;
        oldtotallines = con.totallines;
        con.totallines = CON_TEXTSIZE / con.linewidth;
        numlines = oldtotallines;

        if (con.totallines < numlines) {
            numlines = con.totallines;
        }
        
        numchars = oldwidth;
    
        if (con.linewidth < numchars) {
            numchars = con.linewidth;
        }
        
        Com_Memcpy (tbuf, con.text, CON_TEXTSIZE * sizeof(short));
        for (i = 0; i < CON_TEXTSIZE; i++) {
            con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
        }

        for (i = 0 ; i < numlines ; i++) {
            for (j = 0 ; j < numchars ; j++) {
                con.text[(con.totallines - 1 - i) * con.linewidth + j] =
                        tbuf[((con.current - i + oldtotallines) % oldtotallines) * oldwidth + j];
            }
        }

        Con_ClearNotify ();
    }

    con.current = con.totallines - 1;
    con.display = con.current;
}

/////////////////////////////////////////////////////////////////////
// Name        : Con_Init
// Description : Initialize the console
/////////////////////////////////////////////////////////////////////
void Con_Init (void) {
    
    int i;

    con_bgAlpha = Cvar_Get("con_bgAlpha", "96", CVAR_ARCHIVE);
    con_notifytime = Cvar_Get ("con_notifytime", "3", 0);

    Field_Clear(&g_consoleField);
    g_consoleField.widthInChars = g_console_field_width;
    for (i = 0 ; i < COMMAND_HISTORY ; i++) {
        Field_Clear(&historyEditLines[i]);
        historyEditLines[i].widthInChars = g_console_field_width;
    }
    
    CL_LoadConsoleHistory();

    Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f);
    Cmd_AddCommand ("messagemode", Con_MessageMode_f);
    Cmd_AddCommand ("messagemode2", Con_MessageMode2_f);
    Cmd_AddCommand ("messagemode3", Con_MessageMode3_f);
    Cmd_AddCommand ("messagemode4", Con_MessageMode4_f);
    Cmd_AddCommand ("clear", Con_Clear_f);
    Cmd_AddCommand ("condump", Con_Dump_f);
}

/////////////////////////////////////////////////////////////////////
// Name : Con_Linefeed
/////////////////////////////////////////////////////////////////////
void Con_Linefeed (qboolean skipnotify) {
    
    int i;

    // mark time for transparent overlay
    if (con.current >= 0) {
        if (skipnotify) {
            con.times[con.current % NUM_CON_TIMES] = 0;
        } else {
            con.times[con.current % NUM_CON_TIMES] = cls.realtime;
        }
    }

    con.x = 0;
    if (con.display == con.current) {
        con.display++;
    }
    
    con.current++;
    for (i = 0; i < con.linewidth; i++) {
        con.text[(con.current % con.totallines) * con.linewidth + i] = (ColorIndex(COLOR_WHITE) << 8) | ' ';
    }
    
}

/////////////////////////////////////////////////////////////////////
// Name        : CL_ConsolePrint
// Description : Handles cursor positioning, line wrapping, etc
//               All console printing must go through this in order 
//               to be logged to disk. If no console is visible, the 
//               text will appear at the top of the game window
/////////////////////////////////////////////////////////////////////
void CL_ConsolePrint(char *txt) {
    
    int        y;
    int        c, l;
    int        color;
    qboolean   skipnotify = qfalse;        // NERVE - SMF
    int        prev;                       // NERVE - SMF

    // TTimo - prefix for text that shows up in console but not in notify
    // backported from RTCW
    if (!Q_strncmp(txt, "[skipnotify]", 12)) {
        skipnotify = qtrue;
        txt += 12;
    }
    
    // for some demos we don't want to ever show anything on the console
    if (cl_noprint && cl_noprint->integer) {
        return;
    }
    
    if (!con.initialized) {
        con.color[0] = 
        con.color[1] = 
        con.color[2] =
        con.color[3] = 1.0f;
        con.linewidth = -1;
        Con_CheckResize();
        con.initialized = qtrue;
    }

    color = ColorIndex(COLOR_WHITE);

    while ((c = *txt) != 0) {
        
        if (Q_IsColorString(txt)) {
            color = ColorIndex(*(txt+1));
            txt += 2;
            continue;
        }

        // count word length
        for (l = 0 ; l < con.linewidth ; l++) {
            if (txt[l] <= ' ') {
                break;
            }

        }

        // word wrap
        if (l != con.linewidth && (con.x + l >= con.linewidth)) {
            Con_Linefeed(skipnotify);

        }

        txt++;

        switch (c) {
            
            case '\n':
                Con_Linefeed(skipnotify);
                break;
            case '\r':
                con.x = 0;
                break;
            default:    // display character and advance
                y = con.current % con.totallines;
                con.text[y * con.linewidth + con.x] = (short) ((color << 8) | c);
                con.x++;
                if (con.x >= con.linewidth) {
                    Con_Linefeed(skipnotify);
                    con.x = 0;
                }
                break;
        }
    }

    // mark time for transparent overlay
    if (con.current >= 0) {
        // NERVE - SMF
        if (skipnotify) {
            prev = con.current % NUM_CON_TIMES - 1;
            if (prev < 0) {
                prev = NUM_CON_TIMES - 1;
            }
            con.times[prev] = 0;
        }
        else {
            // -NERVE - SMF
            con.times[con.current % NUM_CON_TIMES] = cls.realtime;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                          //
//  DRAWING                                                                                                 //
//                                                                                                          //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// Name        : Con_DrawInput
// Description : Draw the editline after a ] prompt
/////////////////////////////////////////////////////////////////////
void Con_DrawInput (void) {
    
    int      i;
    int      y;
    int      length;
    char     *prompt;
    qtime_t  now;
    vec4_t   col;

    if (cls.state != CA_DISCONNECTED && !(cls.keyCatchers & KEYCATCH_CONSOLE)) {
        return;
    }

    Com_RealTime(&now);
    
    y = con.vislines - (SMALLCHAR_HEIGHT * 2) + 6;
    prompt = va("[%02i:%02i:%02i] ", now.tm_hour, now.tm_min, now.tm_sec);
    length = (int) strlen(prompt);
    
    col[0] = con.color[0];
    col[1] = con.color[1];
    col[2] = con.color[2];
    col[3] = 0.20f;
    
    Con_RE_SetColor(col);
    
    for (i = 0; i < length; i++) {
        SCR_DrawSmallChar(i * SMALLCHAR_WIDTH + con_marginX + con_paddingX, y + CON_MARGIN * 2, prompt[i]);
    }
    
    Con_RE_SetColor(con.color);

    if (opacityMult) {
        Field_Draw(&g_consoleField, (length * SMALLCHAR_WIDTH) + con_marginX + con_paddingX, y + CON_MARGIN * 2, 
         (cls.glconfig.vidWidth - (con_marginX * 2) - (con_paddingX * 2) - (3 * SMALLCHAR_WIDTH) - 300), qtrue);
    }
    
}

/////////////////////////////////////////////////////////////////////
// Name        : Con_DrawNotify
// Description : Draws the last few lines of output transparently 
//               over the game top
/////////////////////////////////////////////////////////////////////
void Con_DrawNotify (void) {
    
    int       x, v;
    short     *text;
    int       i;
    int       time;
    int       skip;
    int       currentColor;

    currentColor = 7;
    re.SetColor(g_color_table[currentColor]);

    v = 0;
    for (i = con.current - NUM_CON_TIMES + 1 ; i <= con.current; i++) {
        
        if (i < 0) {
            continue;
        }
        
        time = con.times[i % NUM_CON_TIMES];
        if (time == 0) {
            continue;
        }
        
        time = cls.realtime - time;
        if (time > con_notifytime->value * 1000) {
            continue;
        }
        
        text = con.text + (i % con.totallines) * con.linewidth;
        if (cl.snap.ps.pm_type != PM_INTERMISSION && (cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME))) {
            continue;
        }

        for (x = 0 ; x < con.linewidth ; x++) {
            
            if ((text[x] & 0xff) == ' ') {
                continue;
            }
            
            if (((text[x]>>8)&7) != currentColor) {
                currentColor = (text[x]>>8)&7;
                re.SetColor(g_color_table[currentColor]);
            }
            
            SCR_DrawSmallChar((int) (cl_conXOffset->integer + con.xadjust + (x + 1) * SMALLCHAR_WIDTH), v, text[x] & 0xff);
        }

        v += SMALLCHAR_HEIGHT;
    }

    re.SetColor(NULL);
    if (cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME)) {
        return;
    }

    // draw the chat line
    if (cls.keyCatchers & KEYCATCH_MESSAGE) {
        if (chat_team) {
            SCR_DrawBigString (7, v, "say_team:", 1.0f);
            skip = 10;
        } else {
            SCR_DrawBigString (7, v, "say:", 1.0f);
            skip = 5;
        }

        Field_BigDraw(&chatField, skip * BIGCHAR_WIDTH, v, SCREEN_WIDTH - (skip + 1) * BIGCHAR_WIDTH, qtrue);
        v += BIGCHAR_HEIGHT;
    }

}

/////////////////////////////////////////////////////////////////////
// Name        : Con_DrawSolidConsole
// Description : Draws the console with the solid background
/////////////////////////////////////////////////////////////////////
void Con_DrawSolidConsole(float frac) {
    
    int             i, x, y;
    int             rows;
    int             row;
    int             lines;
    int             conHeight;
    int             curColor;
    vec4_t          borderColor;
    vec4_t          bgColor;
    short           *text;
    char            *version;

    lines = (int) (cls.glconfig.vidHeight * frac);
    if (lines <= 0) {
        return;
    }

    if (lines > cls.glconfig.vidHeight) {
        lines = cls.glconfig.vidHeight;
    }

    // on wide screens, we will center the text
    con.xadjust = 0;
    SCR_AdjustFrom640(&con.xadjust, NULL, NULL, NULL);
    
    bgColor[0] = 0.00f;
    bgColor[1] = 0.00f;
    bgColor[2] = 0.00f;

    if (con_bgAlpha->integer >= 0 && con_bgAlpha->integer <= 100) {
        bgColor[3] = (vec_t) (con_bgAlpha->integer / 100.0);
    } else {
        bgColor[3] = 0.96f;
    }

    // draw the background
    y = (int) (frac * SCREEN_HEIGHT - 2);
    if (y >= 1) {
        SCR_AdjustedFillRect(CON_MARGIN, CON_MARGIN, con_width, y, bgColor);
    }

    borderColor[0] = 0.90f;
    borderColor[1] = 0.42f;
    borderColor[2] = 0.13f;
    borderColor[3] = 1.00f;

    conHeight = 240;
    
    //--- DRAW THE CONSOLE BORDER ----------------------------------------------------
    
    SCR_AdjustedFillRect(CON_MARGIN, CON_MARGIN, con_width, 1, borderColor);
    SCR_AdjustedFillRect(CON_MARGIN, CON_MARGIN, 1, conHeight - 1, borderColor);
    SCR_AdjustedFillRect(CON_MARGIN + con_width - 1, CON_MARGIN, 1, conHeight - 1, borderColor);
    SCR_AdjustedFillRect(CON_MARGIN, CON_MARGIN + conHeight - 2, con_width, 1, borderColor);
    
    //--- DRAW THE VERSION NUMBER ----------------------------------------------------
    
    borderColor[3] = opacityMult;
    Con_RE_SetColor(borderColor);
    version = va("%s %s", SVN_VERSION, Cvar_VariableString("ui_modversion"));    
    SCR_DrawSmallStringExt(
            (int) (cls.glconfig.vidWidth - con_marginX - con_paddingX - (SMALLCHAR_WIDTH * strlen(version))),
            lines - (SMALLCHAR_HEIGHT + SMALLCHAR_HEIGHT / 2) + CON_MARGIN, version, borderColor, qtrue);
    
    //--- COMPUTE SOME VALUES FOR TEXT AND SCROLLBAR ----------------------------------
    
    con.vislines = lines;
    rows = (lines - SMALLCHAR_WIDTH) / SMALLCHAR_WIDTH;
    y = lines - (SMALLCHAR_HEIGHT * 3);
    row = con.display;

    if (con.x == 0) {
        row--;
    }
    
    //--- DRAW THE SCROLLBAR ----------------------------------------------------------
    
    if (con.displayFrac == con.finalFrac) {
        
        int     cursor_height;
        int     cursor_pos;
        int     total_lines = con.current;
        int     visible = rows;
        vec4_t  bg_col = { 1.00f, 1.00f, 1.00f, 0.20f };
        
        cursor_height = (int) (visible / (float)total_lines * 180);
        cursor_pos = (int) (con.display / (float)total_lines * (180 - cursor_height));

        SCR_AdjustedFillRect(628 - CON_MARGIN, CON_MARGIN + 30, 2, 180, bg_col);
        bg_col[3] = 0.8;
        SCR_AdjustedFillRect(628 - CON_MARGIN, CON_MARGIN + 30 + cursor_pos, 2, cursor_height, bg_col);
        
    }
    
    //--- DRAW THE CONSOLE TEXT -------------------------------------------------------

    curColor = 7;
    Con_RE_SetColor(g_color_table[curColor]);
    
    for (i = 0 ; i < rows ; i++, y -= SMALLCHAR_HEIGHT, row--) {
        
        if ( y < (CON_MARGIN / 8)) {
            break;
        }

        if (row < 0) {
            break;
        }
        
        if (con.current - row >= con.totallines) {
            // past scrollback wrap point
            continue;    
        }
        
        text = con.text + (row % con.totallines) * con.linewidth;

        for (x = 0 ; x < con.linewidth ; x++) {
            
            if ((text[x] & 0xff) == ' ') {
                continue;
            }

            if (((text[x] >> 8) & 7) != curColor) {
                curColor = (text[x] >> 8) % 10;
                Con_RE_SetColor(g_color_table[curColor]);
            }
        
            SCR_DrawSmallChar(x * SMALLCHAR_WIDTH + con_marginX + con_paddingX, y + CON_MARGIN * 2, text[x] & 0xff);
        
        }
    }

    // draw the input prompt, user text, and cursor if desired
    Con_DrawInput();
    Con_RE_SetColor(NULL);
}

/////////////////////////////////////////////////////////////////////
// Name : Con_DrawConsole
/////////////////////////////////////////////////////////////////////
void Con_DrawConsole(void) {
    
    // check for console width changes from a vid mode change
    Con_CheckResize ();

    // if disconnected, render console full screen
    if (cls.state == CA_DISCONNECTED) {
        if (!(cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME))) {
            Con_DrawSolidConsole(1.0);
            return;
        }
    }

    if (con.displayFrac) {
        Con_DrawSolidConsole(con.displayFrac);
    } else {
        // draw notify lines
        if (cls.state == CA_ACTIVE) {
            Con_DrawNotify();
        }
    }
}

/////////////////////////////////////////////////////////////////////
// Name        : Con_RunConsole
// Description : Scroll it up or down
/////////////////////////////////////////////////////////////////////
void Con_RunConsole(void) {
    
    con_width    = SCREEN_WIDTH - CON_MARGIN * 2;
    con_marginX  = (int) (CON_MARGIN * (cls.glconfig.vidWidth / 640.0));
    con_marginY  = (int) (CON_MARGIN * (cls.glconfig.vidHeight / 480.0));
    con_paddingX = (int) (CON_PADDING * (cls.glconfig.vidWidth / 640.0));
    con_paddingY = (int) (CON_PADDING * (cls.glconfig.vidHeight / 480.0));
    
    // decide on the destination height of the console
    if (cls.keyCatchers & KEYCATCH_CONSOLE) {
        con.finalFrac = 0.5;        // half screen
        targetOpacityMult = 1;
    } else {
        con.finalFrac = 0;          // none visible
        targetOpacityMult = 0;
    }
    
    float moveDist = (float) (cls.realFrametime * 0.005);
    if (targetOpacityMult < opacityMult) {
        opacityMult -= moveDist;
        if (opacityMult < targetOpacityMult) {
            opacityMult = targetOpacityMult;
        }
    } else if (targetOpacityMult > opacityMult) {
        opacityMult += moveDist;
        if (opacityMult > targetOpacityMult) {
            opacityMult = targetOpacityMult;
        }
    }
    
    con.finalFrac = 0.5;        // half screen
    
    if (!targetOpacityMult && !opacityMult) {
        con.displayFrac = 0;
    } else {
        con.displayFrac = con.finalFrac;
    }

}

/////////////////////////////////////////////////////////////////////
// Name : Con_PageUp
/////////////////////////////////////////////////////////////////////
void Con_PageUp(void) {
    con.display -= 2;
    if (con.current - con.display >= con.totallines) {
        con.display = con.current - con.totallines + 1;
    }
}

/////////////////////////////////////////////////////////////////////
// Name : Con_PageDown
/////////////////////////////////////////////////////////////////////
void Con_PageDown(void) {
    con.display += 2;
    if (con.display > con.current) {
        con.display = con.current;
    }
}

/////////////////////////////////////////////////////////////////////
// Name : Con_Top
/////////////////////////////////////////////////////////////////////
void Con_Top(void) {
    con.display = con.totallines;
    if (con.current - con.display >= con.totallines) {
        con.display = con.current - con.totallines + 1;
    }
}

/////////////////////////////////////////////////////////////////////
// Name : Con_Bottom
/////////////////////////////////////////////////////////////////////
void Con_Bottom(void) {
    con.display = con.current;
}

/////////////////////////////////////////////////////////////////////
// Name : Con_Close
/////////////////////////////////////////////////////////////////////
void Con_Close(void) {
    
    if (!com_cl_running->integer) {
        return;
    }
    
    Field_Clear(&g_consoleField);
    Con_ClearNotify ();
    cls.keyCatchers &= ~KEYCATCH_CONSOLE;
    con.finalFrac = 0;
    con.displayFrac = 0;

}
