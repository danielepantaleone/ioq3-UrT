ioquake3 engine for urban terror 4.x
====================================

## Description

This is a custom fork of the official ioquake3 engine supported by the Frozen Sand development team for the 
videogame Urban Terror 4.x serie (http://www.urbanterror.info). This specific engine version is compatible with 
both **4.1** version and **4.2** version, although it's highly optimized to work with Urban Terror 4.2.x.

## How to compile

### *Linux*

* open a terminal window and change to the directory containing this readme
* install necessary libraries: `sudo apt-get install libsdl1.2-dev libxxf86vm-dev libc6-dev-i386`
* run `make`

### *Windows (cross-compile)*

* open a terminal window and change to the directory containing this readme
* install necessary libraries:
    - `sudo apt-get install libsdl1.2-dev libxxf86vm-dev libc6-dev-i386`
    - `sudo apt-get install mingw-w64 gcc-mingw-w64-base gcc-mingw-w64 binutils-mingw-w64`
* run `exec make PLATFORM=mingw32 CC=i686-w64-mingw32-gcc LD=i686-w64-mingw32-ld WINDRES=i686-w64-mingw32-windres`

## Addons & Improvements

### *Server*
    
* added logging capability: writes in the same log file as the game module
* added position save/load: fixed permanent position save/load not working in game module
* added server-side ghosting feature: fix partial entity collision in jump mode
* added custom mapcycle parsing utility (works with plain mapcycle file)
* added flood protect fix: allow 2 reliable client commands every 1.5 seconds
* added `tell` command: send a private message to a specific client
* added RCON `teleport` command: teleport a specific client to another location
* added RCON `position` command: retrieve client world coordinates
* added RCON `sendclientcommand` command: send a reliable command as a specific client
* added RCON `spoof` command: send a game client command as a specific client
* added RCON `forcecvar` command: force a client USERINFO cvar to a specific value
* added RCON `follow` command: use QVM follow command but introduces pattern matching
* allow client position load while being in a jump run (will reset running timer if it was running)
* fixed map searching algorithm: no more unpredictability
* fixed `stopserverdemo` command being executed on non-dedicated servers
* replaced auth ban message with something understandable
* unlocked `sv_fps` cvar from game module constraint
* use of the new pure list system by default: removes compatibility with Q3A clients

### *Client*

* correctly draw new game module color codes in console
* correctly hide clock when cg_draw2d is set to zero
* fixed clipboard data paste crashing the client engine on unix systems
* fixed linux/mac copy&paste considering only the first six characters of a string
* fixed linux SDL gamma bug using XF86
* improved in-game console
* improved windows dedicated console readability
* unlock `snaps` cvar from game module constraint

## New CVARs

### *Server*

* `sv_callvoteWaitTime` - defines the amount of seconds between callvotes
* `sv_autoDemo` - auto record serverside demos of everyone when in matchmode
* `sv_disableRadio` - totally disable radio calls in jump mode
* `sv_ghostRadius` - specify the radius of the ghosting bounding box
* `sv_hideChatCmd` - hide big brother bot commands to everyone but the who issued them
* `sv_dropSuffix` -  a custom message in the disconnect box when a client gets disconnected
* `sv_dropSignature` - a signature to be attached to the drop suffix message
* `sv_checkClientGuid` - check guid validity upon client connection
* `sv_noKnife` - totally removes the knife from the server

### *Client*

* `cl_demoBlink` - make the demo recording string flashing when recording a demo
* `cl_chatArrow` - remove the **>** prefix from every chat message if set to zero
* `cl_drawSpree` - draw the current spree in the hud

## Credits

Even though most of the code has been written by me and / or revised by me, some ideas have been taken from 
other versions of ioquake3: because of that I would like to give the necessary credits to the people
I took ideas from:

* [Rambetter](https://github.com/Rambetter)
* [clearskies](https://github.com/anthonynguyen)
* [mickael9](https://bitbucket.org/mickael9)

[![Build Status](https://travis-ci.org/danielepantaleone/ioq3-for-UrbanTerror-4.svg?branch=1.1)](https://travis-ci.org/danielepantaleone/ioq3-for-UrbanTerror-4)
