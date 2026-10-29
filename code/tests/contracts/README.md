# Product contract provenance

`presentation/residency_tests.cpp` specifies project resource ownership and
eviction behavior. It builds its small image buffers in memory and uses no
external corpus, installation or executable.

- `SequenceTablePressureRetainsQueuedFramesAndReclaimsIdleSequences`: a held
  frame retains its resources; releasing ownership makes capacity available.
- `PressureEvictsOnlyTheOldestIdleSequence`: the compiler's idle eviction policy
  preserves newer entries, observed through independent weak ownership.
- `WorldAndFrameCompilerStateHaveIndependentLifetimes`: destroying the compiler
  handle and releasing a queued frame leave independently owned world state intact.

The target explicitly lists its internal header access in the test configuration.
It uses the standard GoogleTest main. These assertions express project contracts;
they do not assert compatibility with any external implementation.

## Surface storage

`resources/surface_storage_tests.cpp` specifies storage contracts through the
existing Surface API. Inputs are dimensions, local byte arrays and oversized
integers; no image or external output is loaded.

| Case | Independent expectation |
|---|---|
| `SurfaceStorage.OwnedAndBorrowedBuffers` | Seven by three pixels at one or two bytes per pixel require `21 * depth` bytes. Row-major addressing gives the last offset `20 * depth`. A borrowed array remains alive and retains writes after polymorphic destruction; nested locks balance. |
| `SurfaceStorage.RejectsInvalidAndOverflowingDimensions` | Empty storage has no pixels; negative dimensions and unsupported pixel depth fail explicitly. A pitch or allocation larger than the signed integer range must raise a length error. |

Default resolution, unmatched lock counters, virtual query side effects,
fixed layouts and virtual dispatch slots are outside these storage contracts.

## Mixer storage and output clock

`../unit/audio/mixer_tests.cpp` constructs constant or short PCM arrays directly.
It does not construct a sound definition, use a playlist event scheduler or
load media. These tests use standard GoogleTest fixtures with no global sound
registry setup. Internal mixer/session access is declared beside the target.

| Case | Independent expectation |
|---|---|
| `AudioMixerTest.StreamRingWrapRefillAndFinalNeighbor` | A half-rate constant stream crosses the last ring slot. The unpublished interpolation neighbor produces silence; publishing values 0.4 and 0.6 produces midpoint 0.5 and the declared final endpoint. Read and completion fences follow the consumed source frames. |
| `AudioMixerTest.UnitStepLoopAndGainRamp` | Looping constant 0.5 at unit rate stays constant. A four-frame parameter update to zero yields the linear sequence 0.375, 0.25, 0.125, 0 and retains the looping voice. This is project interpolation policy, not an attenuation-table test. |
| `AudioMixerTest.NativeRateCompletesOneSecondOfAudioAfter22050Frames` | At a 22050 Hz source and output rate, exactly 22050 frames consume one second. The completion fence is absent after 22049 frames and present after the final frame. |
| `AudioSessionTest.HandlesRejectNullForeignMisalignedAndDeadSlots` | A handle must identify a live, exactly aligned element of its own session's array. Null, foreign, one-past, byte-shifted and dead entries fail; every live array element succeeds. No original object layout is asserted. |

## Host scheduling and floating-point isolation

`host/game_speed_tests.cpp` uses deterministic injectable wall time and CPU-work
budgets. Its small harness calls the real scheduler and MainLoop; it does not
implement gameplay phases. Ordinary C++ scope guards restore temporary state.
The project host extensions define 120/240 Hz levels, a 32-step batch cap and a
4 ms work budget. Original speed delays and scenario pause rules are outside
the cases below.

| Case | Independent expectation |
|---|---|
| `GameSpeed.SingleEntryStillCapsHighSpeedAtOneIteration` | A single-step entry permits at most one iteration per call; a 240 Hz batch can perform four steps per 60 Hz host interval. Device polling and presentation occur once per host call. |
| `GameSpeed.BothEntriesContainHookFailuresAndRestoreTheActiveContext` | Both false-returning and throwing host hooks must produce failure, leave the frame unchanged, clear active-call state and restore presentation/input flags. |
| `GameSpeed.HostBindingWrapsOnlyIterationsThatRun` | A due iteration is bound once; a zero-time call with no due work is not bound. A rejected binding cannot advance a frame. Callback order follows the project hook declarations. |
| `GameSpeed.HostBoundaryRestoresFloatingPointEnvironmentOnSuccessAndFailure` | Three caller-selected C floating-point rounding modes survive a scope that changes the mode, both on normal exit and exception unwinding. |
| `GameSpeed.TargetRatesDoNotDependOnHostRefreshOrPresentation` | Two seconds at 120 or 240 Hz give 240 or 480 iterations after initialization, independently of 30/60/144 Hz calls or presentation. Phase callbacks preserve project hook order. |
| `GameSpeed.BatchesKeepFrameAndWallClocksSeparateAndPollDeviceOnce` | Four logic frames can share one host clock advance and one input/presentation pair. A four-frame timer expires after those four frame-authority increments; no original delay constant is checked. |
| `GameSpeed.UnlimitedAndSlowWorkAreBoundedWithoutDebt` | Unlimited mode respects the 32-step cap; injected work exceeding 4 ms limits a batch. Zero-time calls accrue no work debt and long stalls restart with one step. |
| `GameSpeed.PauseSpeedChangesAndFailuresCannotCreateCatchUpBursts` | Explicit pause, speed changes, invalid elapsed times and hook failures preserve bounded resume behavior and restore the active context. |
| `GameSpeed.BackgroundUsesTheSameScheduleAndStillHonorsHostPause` | Identical call sequences produce identical foreground/background iteration histories. Explicit host pause stops both and resume adds one bounded iteration. This is a relational property, not a reference speed table. |

## Debug command registry and parser

`host/debug_command_tests.cpp` uses a test-owned command, literal strings and
public command descriptors. It does not create a map, unit or building, load
assets or execute a gameplay mutation. The command's parser describes integer,
text and boolean arguments; the built-in catalog supplies its public command
grammar and bounds.

| Case | Independent expectation |
|---|---|
| `DebugCommands.RuntimeRegistrationAndConflictIsolation` | Scoped registration increments the observed count and permits case-insensitive lookup. Duplicate identifiers/names, reserved names and invalid metadata reject registration without corrupting the existing entry; scope exit restores the count. |
| `DebugCommands.MetadataAndStrictParser` | Catalog descriptors contain required fields; an invalid index preserves output. Literal whitespace, signed integers, overflow, trailing tokens, embedded nulls and input length exercise the declared parser contract. Compile-time checks cover this project's own standard-layout API, whose descriptor fields total 344 bytes (`16 + 24 + 48 + 128 + 128`). |
| `DebugCommands.ExtendedArgumentsRejectMalformedAndNonFiniteValues` | Literal examples cover the catalog's integer, boolean, finite real and reset grammar. Malformed, out-of-domain, non-finite and extra arguments fail without executing a command. No gameplay rank, damage or movement threshold is asserted. |
