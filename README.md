**PS2 Cam Acolyte**

This is a lightweight Windows exe for using camera hacks and other cheats/debugging tools on PS2 games running on the PCSX2 emulator. Just run ps2_cam_acolyte.exe while playing a game in PS2, connect a second controller and use it to fly the camera around, enable/disable lighting, etc.

**Game Support**

Support is currently focused on PS2 horror games(in particular early prototypes), and will expand over time. Every game supports a free cam mode, but some games have other cheats or debugging tools such as disabling shadows or pausing gameplay.

| UUID     | Game                                | Notes                                                        |
| -------- | ----------------------------------- | ------------------------------------------------------------ |
| F7557FA5 | Kuon (Aug 2nd Prototype)            | https://hiddenpalace.org/Kuon_(Aug_2,_2004_prototype)        |
| F3FD313E | Rule of Rose NTSC UC                |                                                              |
| 6BBD4932 | Silent Hill 2 (Director's Cut) EU   |                                                              |
| D915592E | Silent Hill 2 (July 13th Prototype) | https://hiddenpalace.org/Silent_Hill_2_(Jul_13,_2001_prototype) |
| A8D83239 | Silent Hill Origins NTSC UC         |                                                              |

Additional games and functionality for existing games will be added over time.

**Installation**

Download the latest zip from the Releases, unzip into a directory of your choosing and double-click the .exe.

If you're upgrading from a previous version, copy over the preferences.ini from the last version.

**Instructions**

1. Ensure you are running PCSX2 v1.7.2864 or later (i.e. the latest nightly from https://pcsx2.net/downloads)
2. Make sure Settings -> Advanced -> PINE Settings -> Enable is on (slot should be the default 28011)
3. Run a supported game in PCSX2
4. Open ps2_cam_acolyte.exe
5. If successful, the game UUID will be recognized and displayed in the title bar
6. Connect a second controller - most common controllers should work(such as a DualShock or Xbox controller)
7. Click on Controller and select the controller you want to control cameras with
8. Click on Game - the controls are displayed(note that it currently always uses Xbox terminology, so button X will be displayed instead of Cross)
9. Start exploring the game world!
10. Your preferences, i.e. controller selection, are stored in preferences.ini next to the executable

**Troubleshooting**

*Q*: It just says 'Connecting to PCSX2...'

*A*: Ensure PCSX2 is running, it's a version newer than v1.7.2864, PINE is enabled in PCSX2 Settings -> Advanced -> PINE Settings and the slot is set to the default of 28011.

Q: It says [XXXXXXXX] Unsupported Game

A: PS2 Cam Acolyte only supports the games listed in the table above. Some games have multiple regions or versions - the UUID needs to match exactly. The [XXXXXXXX] is the UUID of the current unsupported game.

**About the Code**

PS2 Cam Acolyte uses:

* PINE (Protocol for Instrumentation of Emulators) to write and read memory/opcodes in the running PS2 game
* ImGui (running lazily on input) for the UI
* SDL2 to host DirectX 11(for older Windows compatibility) and for controller support
* glm for vector/matrix math

PCSX2 can be quite resource intensive, so some care has been taken to keep PS2 Cam Acolyte light:

* Runs single threaded, with a blocking socket connection to PCSX2
* Updates logic/commands at a maximum of 100hz and sleeps otherwise
* Draws only when input is received, and only at a maximum of 60hz

**Future Work**

* Support for more games: Haunting Ground, Silent Hill 3 & 4, Kuon (shipping version), etc.
* Reading PNACH cheat files and turning them into toggleable/bindable values using comments(i.e. // disable lighting(Y) would bind the Y button to all subsequent written values)
* Correctly displaying bound controller button labels instead of always using Xbox labels
* Record/playback smoothed camera inputs to help content creators create higher quality video essays, etc.
* A fully allocation free IPC command queue - currently it uses the stock PINE implementation, which has a few API wrinkles that make this impossible

**Contributors**

Pull requests are welcome so long as the following requirements are respected:

* No new cppcheck warnings or errors
* No memory leaks when starting, playing or stopping a game
* CPU/GPU usage remains 0% when not doing anything and under 0.5% when steering cameras on my system when I test it

**Support**

If you like this tool, you can support it by trying the demo for my game:

https://store.steampowered.com/app/2006140/Withering_Rooms

Or following me on Twitter:

https://x.com/WitheringRooms