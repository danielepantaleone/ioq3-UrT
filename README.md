ioquake3 engine for Urban Terror 4.x
====================================

## Description

This is a custom fork of the official ioquake3 engine supported by the Frozen Sand development team for the 
videogame Urban Terror 4.x serie (http://www.urbanterror.info). This specific engine version is compatible with 
both **4.1** version and **4.2*** version, although it's highly optimized to work with Urban Terror 4.2.x.
* **NOTE #1:** this engine is meant to work with Urban Terror 4.x serie only: it may be used with other Quake 3 Arena modifications, although its functionality it's not ensured.
* **NOTE #2:** this engine natively supports the Urban Terror 4.2.x auth system.<br />
* **NOTE #3:** this engine includes @p5yc0runn3r dma HD sound engine and RAW mouse implementation.

## How to compile

### Linux

* Change to the directory containing this readme
* Run 'make'

### Windows (cross-compile)

* Install mingw32 libraries on your linux box
* Change to the directory containing this readme
* Run 'cross-make-mingw.sh'

### Mac OS

*I still have to find a 'human comprehensible way' to compile this engine under OSX Mavericks: for older versions of OSX, please refer to the original repository README*

## Changelog

### Server

* Fixed map searching algorithm: no more unpredictability
* Added engine-side logging capability: writes in the same log file as the game module
* Added engine-side position save/load: fixed permanent position save/load not working in game module
* Added flood protect fix patch: allow 2 reliable client commands every 1.5 seconds
* Added RCON **teleport** command: teleport a specific client to another one or to the given world coordinates
* Added RCON **position** command: retrieve client world coordinates
* Unlocked **sv_fps** CVAR from game module constraint
* Extended callvote spam protection: **sv_failedvotetime** CVAR defines the amount of secs between callvotes
* Added **/tell** command: send a private message to a specific client (works with partial client name)
* Hidden big brother bot (or similar) commands: they are visible just to the client who issued them
* Added RCON **sendclientcommand** command: send a reliable command as a specific client
* Added RCON **spoof** command: send a game client command as a specific client
* Fixed **stopserverdemo** command being executed on non-dedicated servers
* Added custom mapcycle parsing utility
* Added **sv_autodemo** CVAR: auto record serverside demos of everyone when in matchmode
* Added server-side ghosting feature (**EXPERIMENTAL**): should fix partial blocking in Jump Mode
* Added RCON **forcecvar** command: force a client USERINFO CVAR to a specific value
* Added **sv_disableradio** CVAR: totally disable radio calls in Jump Mode
* Added **sv_ghostradius** CVAR: specify the radius of the ghosting bounding box
* Allow client position load while being in a jump run (will reset running timer if it was running)

### Client

* Fixed clipboard data paste crashing the client engine on unix systems
* Added **cl_keepvidaspect** cvar: keeps the aspect ratio of cinematics when using non 4:3 video modes
* Correctly draw new game module color codes in console
* Improved in-game console readability

## Credits

Although most of the code was written by me and / or revised by me, some ideas have been taken from other 
versions  of ioquake3: because of that I would like to give the necessary credits to the people from which 
I took ideas from:

* Rambetter (https://github.com/Rambetter)
* mickael9 (https://bitbucket.org/mickael9)

For the original README please check: https://github.com/Barbatos/ioq3-for-UrbanTerror-4