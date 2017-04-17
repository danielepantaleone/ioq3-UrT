ioQuake3 engine for urban terror 4.x
====================================

## Description

Custom ioQuake3 fork of the official engine supported by the Frozen Sand development team for the 
videogame Urban Terror 4.x serie (http://www.urbanterror.info).

## How to compile

### *Linux*

* Open a terminal window and change to the directory containing this readme
* Install necessary libraries: 
    - `sudo apt-get install build-essential libsdl1.2-dev libxxf86vm-dev`
    - `sudo apt-get install libc6-dev-i386 libxxf86vm-dev mesa-common-dev`
* Switch to the `scripts` directory
* Run either `make-x86.sh` or `make-x86_64.sh`

### *Windows (cross-compile)*

* Open a terminal window and change to the directory containing this readme
* Install necessary libraries:
    - `sudo apt-get install libsdl1.2-dev libxxf86vm-dev libc6-dev-i386`
    - `sudo apt-get install mingw-w64 gcc-mingw-w64-base gcc-mingw-w64 binutils-mingw-w64`
* Switch to the `scripts` directory
* Run `make-mingw32.sh`

## Addons & Improvements

### *Server*
    
* Added logging capability: writes in the same log file as the game module
* Added position save/load: fixed permanent position save/load not working in game module
* Added server-side ghosting feature: fix partial entity collision in jump mode
* Added flood protect fix: allow 2 reliable client commands every 1.5 seconds
* Added `tell` command: send a private message to a specific client
* Added RCON `teleport` command: teleport a specific client to another location
* Added RCON `position` command: retrieve client world coordinates
* Added RCON `sendclientcommand` command: send a reliable command as a specific client
* Added RCON `spoof` command: send a game client command as a specific client
* Added RCON `forcecvar` command: force a client USERINFO cvar to a specific value
* Added RCON `follow` command: use QVM follow command but introduces pattern matching
* Allow client position load while being in a jump run (reset running timer if necessary)
* Improved map searching algorithm
* Fixed `stopserverdemo` command being executed on non-dedicated servers
* Replaced auth ban message with something understandable
* Unlocked `sv_fps` cvar from game module constraint
* Use of the new pure list system by default: removes compatibility with Q3A clients

### *Client*

* Correctly draw new game module color codes in console
* Correctly hide clock when `cg_draw2d` is set to zero
* Fixed clipboard data paste crashing the client engine on unix systems
* Fixed linux/macOS copy&paste considering only the first six chars of a string
* Fixed linux SDL gamma bug using XF86
* Improved in-game console
* Improved windows dedicated console readability
* Unlock `snaps` cvar from game module constraint
* Added `cvarsearch` command to lookup cvars by partial names

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

* [FrozenSand](https://github.com/Barbatos/ioq3-for-UrbanTerror-4)
* [Rambetter](https://github.com/Rambetter)
* [clearskies](https://github.com/anthonynguyen)
* [mickael9](https://bitbucket.org/mickael9)

[![Build Status](https://travis-ci.org/danielepantaleone/ioq3-UrT.svg?branch=master)](https://travis-ci.org/danielepantaleone/ioq3-UrT)