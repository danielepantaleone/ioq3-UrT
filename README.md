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

## Changelog

### *Server*

* fixed map searching algorithm: no more unpredictability
* added engine-side logging capability: writes in the same log file as the game module
* added engine-side position save/load: fixed permanent position save/load not working in game module
* added flood protect fix patch: allow 2 reliable client commands every 1.5 seconds
* added RCON `teleport` command: teleport a specific client to another one or to the given coordinates
* added RCON `position` command: retrieve client world coordinates
* unlocked `sv_fps` cvar from game module constraint
* extended callvote spam protection: `sv_failedvotetime` defines the amount of secs between callvotes
* added `tell` command: send a private message to a specific client (works with partial client name)
* hidden big brother bot (or similar) commands: they are visible just to the client who issued them
* added RCON `sendclientcommand` command: send a reliable command as a specific client
* added RCON `spoof` command: send a game client command as a specific client
* fixed `stopserverdemo` command being executed on non-dedicated servers
* added custom mapcycle parsing utility
* added `sv_autodemo` cvar: auto record serverside demos of everyone when in matchmode
* added server-side ghosting feature: fix partial entity collision in jump mode
* added RCON `forcecvar` command: force a client USERINFO cvar to a specific value
* added `sv_disableradio` cvar: totally disable radio calls in Jump Mode
* added `sv_ghostradius` cvar: specify the radius of the ghosting bounding box
* allow client position load while being in a jump run (will reset running timer if it was running)

### *Client*

* fixed clipboard data paste crashing the client engine on unix systems
* added `cl_keepvidaspect` cvar: keeps the aspect ratio of cinematics when using non 4:3 video modes
* correctly draw new game module color codes in console
* improved in-game console readability
* fixed linux SDL gamma bug using XF86 (by [clearskies](https://github.com/clearskies))

## Credits

Even though most of the code has been written by me and / or revised by me, some ideas have been taken from 
other versions of ioquake3: because of that I would like to give the necessary credits to the people from which 
I took ideas from:

* [Rambetter](https://github.com/Rambetter)
* [mickael9](https://bitbucket.org/mickael9)
* [clearskies](https://github.com/clearskies)

For the original README please check: https://github.com/Barbatos/ioq3-for-UrbanTerror-4