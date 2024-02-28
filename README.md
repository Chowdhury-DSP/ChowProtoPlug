# ChowProtoPlug

ChowProtoPlug is a simple plugin for prototyping DSP code in C++.

The plugin loads a "module" (a DLL with a simple pre-defined interface),
and will continuously re-compile and re-load the module as you make changes
to the module source code.

At the moment, this plugin is primarily set up to work on my computer.
I don't know how much effort it would take to get this system to work
for any user, but this repository is open to pull requests if folks would
like to contribute.

## Building

To build from scratch, you must have CMake installed.

```bash
# Clone the repository
$ git clone https://github.com/Chowdhury-DSP/ChowProtoPlug.git
$ cd ChowProtoPlug

# build with CMake
$ cmake -Bbuild
$ cmake --build build --config Release
```

## Usage

The first time you run the plugin, the plugin will create a config file,
which can be accessed from the "Settings" menu. From here you can set the
path for your CMake executable, and the module that you want to have
continuously reloaded.

## License

ChowProtoPlug is open source, and is licensed under the BSD 3-clause license.
Enjoy!
