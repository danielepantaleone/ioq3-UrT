ioquake3 engine for urban terror 4.x
====================================

## Description

This is a custom fork of the official ioquake3 engine supported by the Frozen Sand development team for the 
videogame Urban Terror 4.x serie (http://www.urbanterror.info). This specific engine version is compatible with 
both **4.1** version and **4.2** version, although it's highly optimized to work with Urban Terror 4.2.x.

## How to compile

### *Linux*

* change to the directory containing this readme
* run `make`

### *Windows (cross-compile)*

* install `mingw32` libraries on your linux box
* change to the directory containing this readme
* run `cross-make-mingw.sh`

### *Mac OS*

*I still have to find a 'human comprehensible way' to compile this engine under OSX Mavericks: for older 
versions of OSX, please refer to the original repository README*

## Addons & Improvements

### *Server*
    
* added logging capability: writes in the same log file as the game module
* added position save/load: fixed permanent position save/load not working in game module
* added server-side ghosting feature: fix partial entity collision in jump mode
* added custom mapcycle parsing utility
* added `tell` command: send a private message to a specific client (works with partial client name)
* added flood protect fix patch: allow 2 reliable client commands every 1.5 seconds
* added RCON `teleport` command: teleport a specific client to another one or to the given coordinates
* added RCON `position` command: retrieve client world coordinates
* added RCON `sendclientcommand` command: send a reliable command as a specific client
* added RCON `spoof` command: send a game client command as a specific client
* added RCON `forcecvar` command: force a client USERINFO cvar to a specific value
* added RCON `forcecaptain` command: force a client to be the captain of his team
* added RCON `forcesub` command: force a client to be substitute for his team
* allow client position load while being in a jump run (will reset running timer if it was running)
* fixed map searching algorithm: no more unpredictability
* fixed `stopserverdemo` command being executed on non-dedicated servers
* replace **Ban Kick** message being displayed upon client auth ban with something understandable
* unlocked `sv_fps` cvar from game module constraint

### *Client*

* correctly draw new game module color codes in console
* fixed clipboard data paste crashing the client engine on unix systems
* fixed linux SDL gamma bug using XF86 (by [clearskies](https://github.com/clearskies))
* improved in-game console readability
* improved windows dedicated console readability
* unlock `snaps` cvar from game module constraint

## New CVARs

### *Server*

* `sv_failedvotetime` - defines the amount of seconds between callvotes
* `sv_autodemo` - auto record serverside demos of everyone when in matchmode
* `sv_disableradio` - totally disable radio calls in jump mode
* `sv_ghostradius` - specify the radius of the ghosting bounding box
* `sv_rconusers` - allow clients to use rcon commands without having to type the password
* `sv_rconuserfile` - specifies the name of the file from which to read auth logins
* `sv_hidechatcmds` - hide big brother bot commands to everyone but the who issued them

### *Client*

* `cl_drawHealth` - draw player health percentace in the HUD
* `cl_demoblink` - make the demo recording string flashing when recording a demo

## Credits

Even though most of the code has been written by me and / or revised by me, some ideas have been taken from 
other versions of ioquake3: because of that I would like to give the necessary credits to the people from which 
I took ideas from:

* [Rambetter](https://github.com/Rambetter)
* [clearskies](https://github.com/clearskies)
* [mickael9](https://bitbucket.org/mickael9)

For the original README please check: https://github.com/Barbatos/ioq3-for-UrbanTerror-4