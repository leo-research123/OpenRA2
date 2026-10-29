# Public host tests

The runner authors small temporary MIX, CSF, INI and map metadata inputs. It builds C#, imports an isolated missing-extension fixture and launches scenes with explicit paths. It requires the normal Debug native extension, Godot 4.7.2 .NET, and the configured .NET SDK. No installed game data is used.

```sh
python3 code/godot/tests/runners/run_tests.py --godot /path/to/godot --dotnet /path/to/dotnet
python3 code/godot/tests/runners/run_tests.py --windowed --capture-dir out/host-captures
```

| Scene | Authored inputs and independent project contract |
|---|---|
| `headless/TestApp` | Empty/small resource packages and malformed configuration values; load gating, navigation, settings persistence, overflow rejection, error handling and missing extension. |
| `headless/TestResourceWorker` | Authored packages; monotonic dynamic progress, one owner, cancellation/restart, worker join on disposal, and hotkey dictionary updates. Completion uses the reported total. |
| `headless/TestINI` | Literal INI/legacy codec bytes and Unicode strings; supplied values, output clearing, codec conversion and document lifetime after owner disposal. |
| `headless/TestMapSelection` | Sixteen authored metadata entries and generated CSF labels; project category/index policy, filtering, deduplication, loose-file precedence, selection and navigation. No scenario output or installed campaign count is an oracle. |
| `headless/TestSkirmishLobby` | One authored 8x12 metadata map, two waypoints, two declared multiplayer countries, and a literal-only LZO1X block holding four RGB pixels; preview availability, seat defaults and normalized starts. The test does not assert original game geometry. |
| `headless/TestAudioShutdown` | Five isolated processes using authored boot packs and a deliberately missing scenario; worker exit, direct tree exit with a real audio child, close/repeat/reopen, fresh playback identity, host resampler/device-rate preservation and deferred node cleanup. It does not certify playable map audio or original sound selection. |
| `windowed/TestResourceWindow` | Same authored boot packages; rendered loading/menu lifecycle and optional screenshots. |
| `windowed/TestDisplayWindow` | Same boot packages and project display settings; actual OS window modes, usable desktop bounds, render viewport sizes, persistence and Quit. Requires a real desktop. |

`support/TestRunner.cs` reports assertion/exception failures to process exit status and uses per-process temporary settings. `support/synthetic_fixtures.py` contains the independent input generators; MIX indexing uses standard CRC32 and a literal directory table, CSF strings use explicit UTF-16LE, and the 2x2 preview is a single literal LZO block. Code and authored inputs use the repository license; no external binary/assets or captured output are distributed by these tests.

The C# project excludes all test scripts outside Debug; export presets exclude test scenes and support files. Production menus, bundled maps and normal automation remain product features.
