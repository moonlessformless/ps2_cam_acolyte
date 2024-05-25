# Hacking Guide

PS2 Cam Acolyte works by establishing an IPC socket connection to PCSX2 and using the PINE protocol to send messages like "get 8 bytes at PS2 memory address X", "set 4 bytes at PS2 memory address Y" or "assemble opcode at address Z".

In order to implement a free cam, the goal is to disable the PS2 instructions that write the "final" camera values(before culling) and instead compute them ourselves based on controller input in the PS2 Cam Acolyte exe.

This guide assumes familiarity with basic 3D math and game programming concepts.

### Finding the Right Camera Values

In the PS2 era, games started to use C++ more and have fairly modern looking camera blending stacks that are dynamic in size and involve vtables or other indirection. However, they almost always end up writing the results of these camera blending functions to global variables before invoking the final rendering stages, since they will be consumed by many different systems(culling, screen space FX, lighting, etc.).

A typical PS2 game has a main loop that looks something like this(obviously simplified for demonstration purposes):

`while (true)
{
	secret_debug_things(); // you'd be surprised how many PS2 games have these
	read_controller_input();
	update_player_and_ai();
	final_camera_values = super_complicated_polymorphic_camera_stack_blending();
	// -> our goal is to write the final_camera_values ourselves here, before culling
	update_culling(final_camera_values);
	draw_scene(final_camera_values);
}`

When hunting through memory, you'll find many copies of camera values at various stages of completeness:

* Several at various stages of completeness from super_complicated_polymorphic_camera_stack_blending()
* Copies in update_culling or draw_scene

If you pick values at too early a stage, you'll find they get stomped later in the pipeline or are reinterpreted in an unexpected way. If you pick values at too late a stage, culling may be inaccurate or there may various rendering systems still using the old values.

# Tools

### PCSX2 Debugger

PCSX2 comes with a reasonably powerful debugger. Choose Debug -> Open Debugger.

You can set read/write/execute breakpoints, look at the disassembled mips code, assemble opcodes, view memory, etc.

Combined with F1 -> Save State and F3 -> Load State, you can rapidly test ideas and then just load the state if they don't work.

### Cheat Engine

The best tool for poking around and finding these camera values is Cheat Engine, obtained here:

https://www.patreon.com/cheatengine

Do the tutorial for learning how to search values: https://wiki.cheatengine.org/index.php?title=Tutorials:Cheat_Engine_Tutorial_Guide_x64

1. Run the game in PCSX2
2. Open the PCSX2 game process in Cheat Engine
3. Ensure Settings->Scan Settings->MEM_MAPPED is checked

Next, we want to determine the range where the PS2 memory lies:

Open Table->Show Cheat Table Lua Script

Enter the following and choose 'Execute script':

`local EEmemAddress = getAddress("EEmem")`
`local EEmem = readPointer(EEmemAddress)`
`local baseAddress = EEmem`
`print(string.format("%x", EEmem))`
`print(string.format("%x", (EEmem + 0x02000000 )))`

EEmem is the base address for the PS2 memory, which will be different from run to run of PCSX2. It will output the range of PS2 memory addresses like the following:

`7ff690000000` 
`7ff692000000`

You want to set your Cheat Engine scan range to be between these two numbers. In this case, if you find something interesting at 0x7ff69091B5F0, the actual PS2 memory address is 0x7ff69091B5F0 - 0x7ff690000000 = 0x0091B5F0.

If you find it useful, you can use Cheat Engine's lua interpreter as a scratchpad for hex and address math, i.e.

`print(string.format("%x", (0x7ff69091B5F0 - EEmem)))`

When searching for camera values, it usually makes sense to set all your value displays to "float".

**ps2dis**

This is an old disassembler that is useful - it lets you enumerate all string literals, quickly find references(i.e. all jal instructions that jump to a particular function), etc.

You can download it here: ps2dis099_23.zip
http://archiv.sega-dc.de/napalm-x.thegypsy.com/wire/html/downloads.htm

ps2dis doesn't work with archives or images, so you'll need to mount your .iso with a tool like Pismo File Mount and directly open the SLUS/SCES_* exe.

Use View->Jump to Labeled to find strings related to camera, "mark" it with spacebar, and then Analyzer -> Jump to Next/Previous Referer to find references. This is a great way to find debug functionality that has been left around.

# Procedure

It's more of an art than a science to find camera values and instructions, but here's a rough guide.

We want to find candidate variables, try disabling the PS2 code that writes to them, manipulate their values and see what happens. If we find the right variables and the right instructions, we can move the camera arbitrarily without any odd behavior.

1. Find candidate variable addresses with Cheat Engine:
   1. Make sure the camera is still
   2. Scan for unknown float
   3. Scan repeatedly for unchanged floats to narrow the set
   4. Move the camera, or (less ideally) move the player in such a way that the camera moves, and then let it settle to be still
   5. Scan for changed floats
   6. Go to 3 and repeat a few times
2. Look for values that appear to be vectors, euler angles(usually radians), or matrices - when you find a candidate, move the camera and watch how the values change and see if they make sense i.e.
   1. A yaw value smoothly going from -pi to pi or 0 to 2pi as you rotate the camera fully around
   2. A matrix, i.e. set of 16 values where the last 4 (position) changes as you move forward/backward
   3. A position vector where the middle value changes as you move the camera up/down
3. Once you find a candidate value, set a write memory breakpoint on the address in the PCSX2 debugger
4. Press Run a few times to see how many functions are writing to the value - the fewer the better. If a value is written to several times, it's likely a scratch buffer and is less useful.
5. Right click the instruction setting the value and choose 'NOP Instruction(s)'
   1. Does the camera stay still while the player continues to move? Good!
   2. Does it stop rendering or other strange things occur? Bad - it's probably an intermediate value.
6. After the NOP, and ensuring no PS2 code is writing to the value, you can try manipulating it in Cheat Engine(easier) or the PCSX2 memory view(harder)
   1. Does the camera change? Good!
   2. Does it look like the culling is left behind(i.e. only objects in the unchanged viewport are drawn)? Bad - it's too late in the pipeline.
7. Iterate on this process until you find the right camera values and the right instructions to disable.

# Writing PS2 Cam Acolyte Tool

1. Create a new tool in the games/folder by duplicating 00000000_nothing.cpp, name it GAMEUUID_gamename.cpp
2. Rename the class, and make sure the registration at the bottom of the file matches the UUID and title name
3. Try building and running while the game is playing in PCSX2 - ensure that your new tool is recognized
4. Implement the update() and draw_game_ui() functions, following the other games as an example

There are a handful of classes that make writing commands, such as setting data, more convenient.

While you have full ImGui access in draw_game_ui(), try to use the shared functions in games/shared_ui.h where possible.

**ps2_ipc_cmd**

Represents an arbitrary IPC command. Generally, you'll be using the other helpers but this is here if you need to do something more sophisticated.

**sentinel_counter**

When the user loads a save state, we don't immediately find out and could wind up in a strange state. Instead, each time the user toggles a flag, we increment a sentinel value written into unused memory. When this number goes down, we know the user has loaded a save state, and should then reset and restore all game state.

**toggle_state**

Has two states - on and off, each with a set of arbitrary commands to run. Use a builder style syntax to set it up in the constructor:

​		``brightness_flag.edit_off()`
​			`.write<int32_t>(0x00929684, 0x41B00000)`
​			`.write<int32_t>(0x00929688, 0x3F800000)`
​			`.write<int32_t>(0x00929690, 0xC414A420)`
​			`.finalize();`
​		`brightness_flag.edit_on()`
​			`.write<int32_t>(0x00929684, 0x43FA0000)`
​			`.write<int32_t>(0x00929688, 0x43F78000)`
​			`.write<int32_t>(0x00929690, 0x00000000)`
​			.finalize();`

**multi_toggle_state**

Similar to toggle_state but supports an arbitrary number of states.

**read_only_value_set<type, number of values>**

A set of addresses you want to read by calling update() and then get(index). Has two constructors - one takes an array of addresses, the other takes a base address.

**tweakable_value_set<type, number of values>**

A set of addresses that can be "tweaked". When you call start_tweaking(), it will save each current value, and when you call stop_tweaking(true), they'll be restored. Commonly used for camera values - when the freecam is turned on, the old values are saved and are restored when freecam is turned off.

**restoring_toggle_state<type, number of values>**

For each address, takes a "restore" value and a "toggled" value. When toggled on, sets to the "toggled" value, and toggled off sets to the "restore" value. A less verbose(but also less powerful) alternative to multi_toggle_state.