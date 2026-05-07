# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

The build system entry point is `build.py`. All platforms use the same script with a `--framework` flag.

```shell
# Android — debug APK, run on connected device
python3 build.py --framework android --run

# Android — release APK only
python3 build.py --framework android --config Release

# Android — generate Android Studio project (no build)
python3 build.py --framework android --generate_only

# Android — generate compile_commands.json for clangd
python3 build.py --framework android --clangd

# Clean before configure (resolves some build errors)
python3 build.py --framework android --clean
```

Other supported frameworks: `glfw` (Windows/macOS), `macos`, `ios`.

**Required environment variables** (build.py auto-detects where possible):

| Variable | Purpose |
|---|---|
| `VULKAN_SDK` | Vulkan SDK path (auto-installable) |
| `ANDROID_HOME` | Android SDK — must be set manually (Android Studio) |
| `JAVA_HOME` | JDK path — auto-detected from Android Studio |
| `VCPKG_PATH` | Windows only, vcpkg root (auto-installable) |

**Build output:** `build_system/<platform>/output/` for artifacts, `product/` for archives.

See [docs/Build.md](docs/Build.md) for all build options and IDE setup.

## Running Tests

```shell
# Smoke test — verifies full init pipeline (headless)
python3 build.py --framework android --run --test_case smoke --headless true

# Screenshot test — captures one frame after scene load
python3 build.py --framework android --run --test_case screenshot --headless true

# Run with frame budget (fail if not done within N frames)
python3 build.py --framework android --run --test_case <name> --test_timeout 60
```

Screenshots are saved to `[external-storage]/screenshots/`. See [docs/Run.md](docs/Run.md) for the platform-specific path.

See [docs/Test.md](docs/Test.md) for writing test cases and visual testing.

## Code Style

- **C++20**: Use modern features (concepts, ranges, `std::format`).
- **No dead code**: Remove unused includes, variables, and arguments immediately.
- **Formatters are mandatory**: Always run formatters after editing; never format manually.
  - C++/ObjC/Slang → `clang-format` (`.clang-format`)
  - Markdown → `markdownlint` (`.markdownlint.json`)
  - Python → autopep8 (PEP8)
- **clang-tidy**: All warnings are errors (`WarningsAsErrors: "*"`). Code must pass before commit.

**Naming conventions:**

| Construct | Style | Example |
|---|---|---|
| Namespace | `lower_case` | `sparkle` |
| Class / Struct / Enum | `CamelCase` | `RenderFramework` |
| Function | `CamelCase` | `BeginFrame()` |
| Local variable / public member | `lower_case` | `frame_count` |
| Private / protected member | `lower_case_` (trailing `_`) | `render_thread_` |
| `constexpr` / static constant | `CamelCase` | `MaxBufferedTaskFrames` |
| Enum constant | `Camel_Case` | `Primary_Left` |

## Architecture

### Key Components

| Component | Location | Description |
|---|---|---|
| `AppFramework` | `libraries/source/application/` | Main application base class |
| `RenderFramework` | `libraries/source/application/` | Rendering pipeline lifecycle |
| `NativeView` | `frameworks/source/` | Platform windowing/input interface |
| `RHI` / `RHIContext` | `libraries/include/rhi/` | Graphics API abstraction (singleton, Vulkan or Metal) |
| `TaskManager` | `libraries/include/core/task/` | Async task scheduling |
| `Scene` | `libraries/source/scene/` | Scene graph root; owns all components |
| `Renderer` | `libraries/include/renderer/` | Abstract base for all pipeline renderers |
| `ConfigManager` | `libraries/include/core/ConfigManager.h` | Runtime config/cvar registry |
| `MaterialManager` | `libraries/include/scene/material/` | Material lifecycle and lookup |

### Repository Layout

```text
sparkle/
├── build.py              # Main build entry point
├── build_system/         # Per-platform build scripts (glfw, macos, ios, android)
├── libraries/
│   ├── include/          # Public headers: application/, core/, rhi/, renderer/, scene/, io/
│   └── source/           # Implementations
├── frameworks/source/    # Platform wrappers (NativeView, FileManager, etc.)
├── shaders/              # Slang shader source → SPIRV → Metal
│   ├── ray_trace/        # Path tracer compute shaders
│   ├── screen/           # Post-processing
│   ├── standard/         # Vertex/pixel shaders
│   └── utilities/        # Utility compute
├── tests/                # TestCase implementations (C++)
├── dev/                  # Developer Python scripts (functional_test.py, etc.)
├── resources/            # Assets: models, textures, default config
└── thirdparty/           # Git submodule dependencies
```

### Shader Pipeline

Slang (`.slang`) → SPIRV → Metal (via spirv-cross). The build script handles compilation automatically.

### Build System Architecture

`build.py` uses a factory pattern: `build_system/builder_factory.py` creates a platform-specific `FrameworkBuilder` subclass. Prerequisites (CMake, Vulkan SDK, etc.) are auto-detected and installed where possible by `build_system/prerequisites.py`.

### Graphics Pipelines

Supports multiple rendering modes: forward, deferred, GPU path tracing, CPU path tracing. Select via `--pipeline` at runtime.

## Writing TestCases

Create a `.cpp` under `tests/`, subclass `sparkle::TestCase`, implement `Tick()`, register with `TestCaseRegistrar`:

```cpp
#include "application/AppFramework.h"
#include "application/TestCase.h"

namespace sparkle
{
class MyTest : public TestCase
{
public:
    Result Tick(AppFramework &app) override
    {
        if (frame_++ < 10)
            return Result::Pending;
        return Result::Pass;  // or Result::Fail
    }
private:
    uint32_t frame_ = 0;
};
static TestCaseRegistrar<MyTest> registrar("my_test");
}
```

Test case names must be unique across all files under `tests/`.

## Visual Debugging

- Use `--test_case screenshot --headless true` to capture a frame without a window.
- Use `--debug_mode` to route intermediate textures to the render target.
- To inspect a non-texture shader variable: write it to full screen in the shader and check the screenshot (disable tonemapping for raw values).
- For temporal issues, use `--test_case multi_frame_screenshot` (captures 5 frames).

## Common Pitfalls

- **Android builds**: `ANDROID_HOME` / `JAVA_HOME` must point to Android Studio's SDK/JBR — not standalone JDK.
- **iOS builds**: Require `APPLE_DEVELOPER_TEAM_ID` and `--apple_auto_sign` for device deployment.
- **Shader errors**: Check both SPIRV compilation and Metal conversion logs.
- **RHI resources**: Use the deferred deletion pattern for GPU resource cleanup.
- **Cross-platform paths**: Use `FileManager` abstraction — never raw path separators.
- **Crashes and errors**: Do not ignore unrelated crashes; record them in [docs/TODO.md](docs/TODO.md).
