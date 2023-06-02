## Welcome
If you would like to make a plugin for xNVSE, you may want to join us on Discord if you have any questions, in the #plugin-development channel (https://discord.gg/NXqSEcHjg2).

Likewise, if you'd like to contribute to xNVSE itself, we'd be happy to help you get set up on Discord. Pull requests are welcome.

A solid grasp of C++ fundamentals will be required to make sense of 90% of the code. A few things are written in x86 ASM (Assembly), due to the need to modify the game's machine code as it is running. Due to the complexity of the source code, the weird file #include structure, and the custom macros being used, trying to start learning C++ by making an NVSE plugin isn't very recommended. Don't be afraid to ask any C++ or ASM questions on our Discord!

The instructions below will outline the steps needed to get started.

## Setup
The project uses Visual Studio 2022 to view, modify and compile the C++ source code. Older versions of VS, such as 2020, may also work, but may require some additional help to get working. Plugins are also built in Visual Studio, though they may choose any version of Visual Studio, so long as it supports compiling the NVSE source files relevant to your project (for example, `GameForms.h` and `GameForms.cpp` are needed to define certain form classes the game uses).

When installing Visual Studio 2022, make sure you enable "Desktop development with C++" to be installed, to get the necessary build tools to build xNVSE and xNVSE plugins.

To open up a project, or a project's "solution", as Visual Studio calls it, find and open up the project's ".sln" (dot solution) file.
From there, you will have quick access to the preconfigured build setups.

Before compiling a project, you will want to set up an Environment Variable so that plugin solutions will know the path to your Fallout NV installation to send the built DLLs to.
Start by looking up "Edit the system environment variables" in the Windows Search Bar, click the first result. This should open up the Control Panel at a "System Properties" window. Click the `Environment Variables...` box option, then define a variable called `FalloutNVPath` with the path to your `Fallout New Vegas` folder (where the game is installed).

## Making an xNVSE Plugin
When to make an xNVSE plugin?
* If what you want can be scripted using the game's script language, Obscript, then chances are a plugin isn't needed. Plugins are more complicated to code, and can easily corrupt data and cause crashes if they aren't coded with care. Scripts are already sufficiently fast for most cases, and are much easier to maintain and for others to modify.
* Otherwise, if the mod can't be done in a script, such as if you want to add functions for scripts to use, or if you want to avoid relying on script jank and do it the hard way, plugins are the way to go.

To make an xNVSE plugin, go to https://github.com/xNVSE/NVSE/releases and download the source code from the latest release.
This way, you will avoid including any in-progress changes from the current source code.

You'll want to open and try compiling the Example Plugin included in the xNVSE source code first; afterwards, you can modify it and release it out into the wild.
To do so, open up the project by opening `nvse_plugin_example.sln` in the `nvse_plugin_example` folder, then hit compile.

To change the minimum (x)NVSE version required for the plugin to run, go to the `nvse_version.h` file and modify the `NVSE_VERSION_INTEGER`, `NVSE_VERSION_INTEGER_MINOR`, `NVSE_VERSION_INTEGER_BETA`, and `NVSE_VERSION_VERSTRING` macro definitions, as needed.

## Making edits to xNVSE itself
To make edits to xNVSE itself, clone the github repo, make your edits on the clone, and send a Pull Request.

To open up the project, find and open the `nvse.sln` file in the `nvse` folder.

## Building an xNVSE Project
xNVSE uses a couple of build solution configurations: "Debug", "Debug GECK", "Release", and "Release GECK".
* "Debug" means the code will be compiled with very little optimizations enabled, so that stepping through the code while in a debugger will more closely match the written source code, making for a better debugging environment.
* "Release" is the opposite of "Debug"; it will optimize the build as much as possible to benefit the users.
* "(...) GECK" build configurations are for compiling a separate plugin that will run exclusively when the GECK is open.
* Conversely, non-"GECK" build configs are for compiling a plugin that will run at runtime (when the game is running).

NOTE: Most xNVSE *plugins* only use two build configurations: Debug and Release. They are set up to compile code to run both when the GECK and the game is running.
To further explain, you may run across the `RUNTIME` and `EDITOR` macro definitions being used in `#if` checks or similar.
`#if RUNTIME` is a preprocessor operation stating that the code encapsulated in the "#if" directive should only be compiled when building a non-"GECK" build.
Likewise, `#if EDITOR` will only allow the encapsulated code from being included in a build when using a "GECK" build configuration.

For plugins that want to run both in GECK and at runtime with a single DLL plugin, simply never use `#if EDITOR` checks, and you may safely ignore/remove `#if RUNTIME` checks. You may use any build configuration, except for the "GECK" configurations.
