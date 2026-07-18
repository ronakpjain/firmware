# PER Vehicle Firmware ⚡️

![Workflow Status](https://github.com/PurdueElectricRacing/firmware/actions/workflows/build.yml/badge.svg)
![GitHub commit activity](https://img.shields.io/github/commit-activity/m/PurdueElectricRacing/firmware?style=flat-square)

A monorepo of all firmware projects, build tools, and scripts driving the PER vehicle.


## Directory Structure
- `can_library/` - In-house distributed CAN library and generation infrastructure
- `common/` - Common libraries shared across the codebase
- `docs/` - Documentation files
- `external/` - External dependencies and third-party libraries
- `source/` - Source code for each vehicle PCB


## Doxygen
Most recent doxygen deployment (master branch): https://purdueelectricracing.github.io/firmware/


## Getting Started

To compile software for the PER vehicle, make sure your system is set up by following the steps in [setup.md](docs/setup.md) if you haven’t already.

> [!NOTE]
> [setup.md](docs/setup.md) is here!


## Building Firmware

Firmware is built using a python-based build system. The python script `per_build.py` handles CMake configuration and ninja build steps automatically.

To build the firmware, run:
```bash
python3 per_build.py
```

You can view available build targets and options with:
```bash
python3 per_build.py --help
```

## Unit Tests

Unit tests use GoogleTest and run on the host machine, independently of the
STM32 cross-compiled firmware build. GoogleTest is downloaded at its pinned
revision when the tests are configured for the first time.

On Windows, run the tests from the WSL environment described in the setup guide.

To configure, build, and run all tests through the project build script:

```bash
python3 per_build.py --test
```

The host suite builds isolated executables for shared code, G4-specific code,
A-box, dashboard, main-module, and driveline tests. Each executable receives only
the fakes and compile definitions it needs. Test sources live in the owning
module's `tests/` directory; shared fakes and runtime support live under
`tests/` at the repository root.

## Hardware Debugging 

In VS Code, go to **View → Run and Debug**, select the appropriate MCU target from the dropdown, then press the green ▶️ arrow to flash and live-debug the firmware.

Once everything is ![set up](setup.md), you can build the firmware by pressing:

```
Ctrl + Shift + B on Windows/Linux
Cmd + Shift + B on macOS
```

This triggers the default build task configured in .vscode/tasks.json, which runs the firmware build process automatically.

Make sure you're in the root of the `firmware` repo (code .) before triggering the build.
