# Getting Started

This page explains how to integrate TinyTRL into your project.

## Requirements

* A C++20-compatible compiler: **GCC**, **Clang**, or **MSVC**.
* No additional runtime dependencies beyond a small subset of the C standard library (e.g. `memcpy`, `memmove`).
* Exceptions are **not required** — TinyTRL does not use or catch them. Disabling exceptions is recommended so your build matches the library's exception-free design (see Step 4).

## Integration

TinyTRL is a header-and-source library that is added directly to your project. There is no precompiled binary to link against. A `CMakeLists.txt` is provided at the project root, so you can also integrate it via CMake (see **Using TinyTRL with CMake** below).

### Step 1 — Add the include path

Make sure the `include/` directory of TinyTRL is visible to your compiler.

| Tool | How |
|---|---|
| Command line (GCC/Clang) | `-I/path/to/TinyTRL/include` |
| MSVC (`.vcxproj`) | Append `..\..\..\include` (or your own path) to `<IncludePath>` in a `PropertyGroup`, as the examples under `examples\msvc\` do: `<IncludePath>$(VC_IncludePath);$(WindowsSDK_IncludePath);..\..\..\include</IncludePath>`. |
| CodeLite (`.project`) | Add `<IncludePath Value="../../../include"/>` inside the `<Compiler>` element of the relevant `<Settings>` block. |

### Step 2 — Add the source files

Add all four `.cpp` files from `src/` to your project as compile units. These contain the non-template function definitions.

| File | Contents |
|---|---|
| `TinyTRL_Math.cpp` | Non-template math functions (`fmod`, `fma`, `log2`, …) and floating-point bit utilities |
| `TinyTRL_Strings.cpp` | String utilities, comparisons, search, and replacement |
| `TinyTRL_Timing.cpp` | High-resolution timing functions |
| `TinyTRL_Streams.cpp` | `FileStream` and `MemoryStream` implementations |

Template and inline code lives in headers under `include/` and does not need to be compiled separately.

### Step 3 — Include the header in your code

```cpp
#include "TinyTRL.h"
using namespace trl;
```

That is the only include you need; `TinyTRL.h` pulls in all feature headers.

### Step 4 — Disable exceptions

TinyTRL is designed to work in freestanding and exception-free environments. When compiling on a platform whose default toolchain enables exceptions, you should disable them to match:

* **GCC / Clang**: `-fno-exceptions`
* **MSVC**: Set `<ExceptionHandling>false</ExceptionHandling>` in the `.vcxproj` `ItemDefinitionGroup` (equivalently, pass `/EHs-c-` or `/EHsc-` to the compiler). The example projects under `examples/msvc/` already have `<ExceptionHandling>false</ExceptionHandling>` set.

## Example projects

Ready-to-open sample projects are provided in `examples/` for two common IDEs. Each demonstrates a different set of features and can be built without modification.

| Directory | IDE | Examples included |
|---|---|---|
| `examples/msvc/<Name>/` | Visual Studio (2022) | `Arrays`, `FlatMapsAndSets`, `Streams` |
| `examples/codelite/<Name>/` | CodeLite | `Arrays`, `FlatMapsAndSets`, `Streams` |

The shared source file for each example lives in `examples/src/<Name>.cpp`.

### Visual Studio

1. Open `examples\msvc\<Name>\<Name>.sln` in Visual Studio 2022 (toolset `v143`).
2. Build with the **Debug** or **Release** configuration for **Win32** or **x64**.
3. Run — the program is a console application; no extra steps are needed.

Key settings you can inspect in the `.vcxproj` for reference:

```xml
<!-- Source files from TinyTRL are added directly to the project -->
<ClCompile Include="..\..\..\src\TinyTRL_Math.cpp" />
<ClCompile Include="..\..\..\src\TinyTRL_Streams.cpp" />
<ClCompile Include="..\..\..\src\TinyTRL_Strings.cpp" />
<ClCompile Include="..\..\..\src\TinyTRL_Timing.cpp" />

<!-- Include path pointing to TinyTRL headers -->
<IncludePath>$(VC_IncludePath);$(WindowsSDK_IncludePath);..\..\..\include</IncludePath>

<!-- Exceptions disabled (recommended) -->
<ExceptionHandling>false</ExceptionHandling>

<!-- C++20 language standard -->
<LanguageStandard>stdcpp20</LanguageStandard>
```

### CodeLite

1. Open `examples\codelite\<Name>\<Name>.workspace` in CodeLite (GCC toolchain).
2. Build the **Debug** or **Release** configuration.
3. Run — the program is a console application.

Key settings you can inspect in the `.project` file for reference:

```xml
<!-- Source files from TinyTRL are listed directly in the project -->
<VirtualDirectory Name="TinyTRL">
  <VirtualDirectory Name="src">
    <File Name="../../../src/TinyTRL_Math.cpp"/>
    <File Name="../../../src/TinyTRL_Strings.cpp"/>
    <File Name="../../../src/TinyTRL_Timing.cpp"/>
    <File Name="../../../src/TinyTRL_Streams.cpp"/>
  </VirtualDirectory>
</VirtualDirectory>

<!-- Include path and exception-free mode are both set -->
<Compiler Options="-std=gnu++20;-fno-exceptions;-funsigned-char;-Wno-parentheses" C_Options="" Assembler="">
  <IncludePath Value="../../../include"/>
</Compiler>
```

## Using TinyTRL in your own project

Once the include path and source files are in place, use the `trl` namespace just as shown in the examples:

```cpp
#include "TinyTRL.h"

using namespace trl;

int main()
{
  Array<int32_t> values = {10, 20, 30};
  values.addp(5);

  if (!values) {
    printf("Operation failed (likely out of memory).\n");
    return 1;
  }

  for (int32_t v : values)
    printf("%d\n", v);

  return 0;
}
```

For a full description of how to detect and handle failures, see [Error Handling](Error_Handling.md).

## Using TinyTRL with CMake

The root `CMakeLists.txt` defines a library target, `TinyTRL` (aliased as `TinyTRL::TinyTRL`, and exported as `TinyTRL::TinyTRL` when installed). It requires C++20 and disables exceptions automatically (matching the flags and settings used by the IDE example projects).

When TinyTRL is the top-level project, the example applications in `examples/` are also built by default alongside the library. Pass `-DTINYTRL_BUILD_EXAMPLES=OFF` to build the library only. (When TinyTRL is added to someone else's project via `add_subdirectory()`, examples are off by default.)

### Choosing Debug or Release

Which command selects the build type depends on the CMake **generator**, not the compiler:

* **Single-config generators** (Ninja, Makefiles) — normally used with **GCC** and **Clang** — read the build type from `CMAKE_BUILD_TYPE` at configure time.
* **Multi-config generators** (Visual Studio, Xcode) — the default when CMake runs on Windows — *ignore* `CMAKE_BUILD_TYPE` and instead select the configuration at build time with `--config`.

To avoid this being a surprise, the project defaults `CMAKE_BUILD_TYPE` to **Release** for single-config generators and exposes the usual choices (`Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`). So a build with no explicit type is already optimized.

Run CMake from the source directory into a build folder **inside the tree**, e.g. `build-release/` and `build-debug/` (these are just normal CMake build directories, so either location works).

### GCC / Clang (Ninja — single-config)

```sh
# Release
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release

# Debug
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
```

The library is written to `build-release/libTinyTRL.a` (a static archive; the name/format varies by platform).

### MSVC (Visual Studio generator — multi-config)

`--config` selects Debug or Release at build time. One build directory can hold both, or you can keep them in separate directories as shown here:

```sh
# Release
cmake -S . -B build-release
cmake --build build-release --config Release     # -> build-release\Release\TinyTRL.lib

# Debug
cmake -S . -B build-debug
cmake --build build-debug --config Debug         # -> build-debug\Debug\TinyTRL.lib
```

`CMAKE_BUILD_TYPE` has no effect with multi-config generators (the configuration is selected at build time with `--config`), so there is no need to pass it on the command line. If MSVC prints a `D9025` warning about overriding the default exception-handling flag, that is the no-exceptions setting taking effect and can be safely ignored.

### Using TinyTRL in your own CMake project

Either add it as a subdirectory or find the installed package:

```cmake
# Option A: build TinyTRL in-tree
add_subdirectory(path/to/TinyTRL)
add_executable(YourApp main.cpp)
target_link_libraries(YourApp PRIVATE TinyTRL::TinyTRL)
```

```cmake
# Option B: use an installed TinyTRL
find_package(TinyTRL REQUIRED CONFIG)
add_executable(YourApp main.cpp)
target_link_libraries(YourApp PRIVATE TinyTRL::TinyTRL)
```

To produce an installed TinyTRL, run `cmake --install build-release` (add `--config Release` when using a multi-config generator such as Visual Studio). It is installed under the standard `--prefix` layout (`lib/`, `include/`, and `lib/cmake/TinyTRL/` for the package config files).
