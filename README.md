# ClipboardSync
Synchronize clipboad contents between user accounts across LAN

## Build

Requires Qt 6 (Widgets + Network), CMake 3.23+ and Ninja. After a fresh clone:

```
git submodule update --init
cmake --preset default
cmake --build --preset release
```

Set the `CMAKE_PREFIX_PATH` environment variable to the Qt installation prefix (or put machine-specific settings in an untracked `CMakeUserPresets.json`). On Windows, run from a Visual Studio developer prompt so the compiler and the VS-bundled `cmake`/`ninja` are available.
