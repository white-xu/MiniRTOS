# Repository Guidelines

## Project Structure & Module Organization
`MiniRTOS/` contains the kernel core: `task.c`, `queue.c`, `list.c`, public headers in `MiniRTOS/include/`, the Cortex-M4 port in `MiniRTOS/protable/RVDS/`, and heap management in `MiniRTOS/protable/MemMang/`. `User/` holds the application entry point, interrupt handlers, and board-specific configuration. `SYSTEM/` contains shared debug and delay utilities. `Libraries/` vendors CMSIS and STM32 HAL sources; treat these as third-party code and keep edits minimal. `cm_backtrace/` provides fault analysis support. `Project/MiniRTOS.uvprojx` is the Keil MDK project, and build artifacts are emitted to `Output/`.

## Build, Test, and Development Commands
Use Keil MDK/uVision for normal development.

```powershell
UV4.exe -b Project\MiniRTOS.uvprojx -t MiniRTOS
```

Builds the `MiniRTOS` target from the command line when Keil is installed and `UV4.exe` is on `PATH`.

```powershell
UV4.exe Project\MiniRTOS.uvprojx
```

Opens the project in the IDE for editing, debugging, and flashing STM32F407 hardware. Successful builds generate `Output\MiniRTOS.hex`.

## Coding Style & Naming Conventions
Write C99-compatible code to match the Keil project settings. Follow the existing FreeRTOS-style naming: `vTask...` for `void` functions, `xQueue...` for status-returning functions, `px...` for pointers, and `...Handle_t` for opaque handles. Prefer 4-space indentation in new code; keep surrounding style unchanged when modifying legacy files. Use header guards in uppercase and keep task, queue, and list APIs declared under `MiniRTOS/include/`. Comments should be brief and explain scheduler, queue, or interrupt behavior when it is not obvious.

## Testing Guidelines
There is no automated unit-test framework in this repository today. Verify changes by rebuilding the Keil target and running on STM32F407 hardware. For kernel changes, test task creation, delay/blocking paths, queue send/receive behavior, and fault output from `cm_backtrace`. If you add tests, place them in a dedicated top-level `tests/` directory and document how to execute them.

## Commit & Pull Request Guidelines
Recent history uses short, focused commit subjects, mostly in Chinese, sometimes with a prefix such as `docs:`. Keep commits scoped to one change, use an imperative summary, and mention the subsystem when useful, for example `task: fix delayed task switch`. Pull requests should describe the problem, summarize the fix, note the target board/toolchain used for validation, and attach logs or screenshots when the change affects debug output, flash artifacts, or IDE configuration.
