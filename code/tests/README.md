# Public product tests

These tests use authored inputs and project API, ownership, error, build and lifecycle contracts. No game installation or external test repository is required. Repository licensing and retained upstream notices apply to the code; all temporary input files are generated from literals by these tests.

Testing defaults to OFF. From the repository root:

```sh
cmake -S code -B out/tests-core -DBUILD_TESTING=ON -DRA2_BUILD_GODOT=OFF -DRA2_BUILD_MOVIES=OFF -DRA2_BUILD_RENDER_DEVICE=OFF
cmake --build out/tests-core --parallel
ctest --test-dir out/tests-core --output-on-failure
ctest --test-dir out/tests-core -L render-cpu --output-on-failure
```

For the optional software component add `-DRA2_BUILD_SOFTWARE_RENDER=ON` in a separate build directory. `software_render_link` forces every object in that component to link. `map_headers_compile` is a compile-only public inheritance check. A host-enabled build also creates `text_encoding`, backed by the pinned ICU dependency. Pure core builds do not create or depend on that text component.

`contracts/`, `unit/`, and `build/` own their CMake registrations. All native cases use upstream `GTest::gtest_main`; `support/scoped_state.hpp` only restores ordinary values and runs cleanup. `RA2_GTEST_DISCOVER_TESTS=ON` additionally exposes individual instances; aggregate entries remain available.

The previously reviewed per-case basis is recorded in [contracts/README.md](contracts/README.md) and [unit/README.md](unit/README.md). Additional independent groups are listed below; each row names a declaration, including the four Boolean instances of the resource-shutdown parameterized declaration.

| Declaration | Input and expectation basis |
|---|---|
| `FileChain.Contracts` | Project filesystem ownership, case-insensitive mounted names, error/EOF status; bytes authored by the test and independently read through std::ifstream. |
| `ResourceShutdown.OwnershipAndReload` | Caller-owned SHP header remains caller-owned across resource destruction/recreation; four explicit Boolean lifecycle combinations and a 33-byte authored 1x1 image. |
| `CoreFullLink.WholeArchiveAndPublicLifecycle` | Every real core translation unit links; public resource handles can be created and destroyed using an empty authored directory. |
| `PointGrid.RandomMovementDeletionAndReusedIdsMatchBruteForce` | Deterministic authored points, 30000 mutations and integer boundaries compared with exhaustive membership, independent of index buckets. |
| `RawFileStorage.OwnedNameAndHandleErrors` | Published RawFile class ownership contract and filesystem handle/error operations, with authored temporary files; constructor borrowing and SetFileName copying are checked separately. |
| `RawFileStorage.RejectsOversizedFileSizes` | An independently created sparse file larger than UINT32_MAX cannot narrow to a valid signed 32-bit file size. |
| `ProjectileLog.CopiedMetadataAndRotation` | Project diagnostic metadata lifetime, explicit 8 MiB rotation policy, disabling logs and invalid path handling; synthetic objects and names, with original VA assertion excluded. |
| `SoftwareRenderLink.WholeArchiveLifecycle` | Whole-archive optional component link and owned converter allocation/destruction; original shade-count assertion remains research-owned. |
| `ResourceScope.OutputOwnershipAndNestedExceptions` | Documented public handle ownership, output preservation and nested callback exception translation; authored INI Count=42 and empty language archives. |
| `ResourceScope.CancelAndRestartKeepsMonotonicProgress` | Public progress callback ordering and cancellation/recreation; completed equals dynamically reported total, without an original package count. |
| `ResourceScope.RawImageDecodeCopiesTheSuppliedPixels` | Uncompressed 2x2 image copies the four supplied bytes in row order. |
| `AudioSessionContract.UniqueOwnerAndFiniteLevels` | Published unique session ownership, null competing output, NaN rejection and exact supplied gain values; no original audio selectors. |
| `TypeDrawing.LightingResourceAndRasterFallback` | Project backend negotiation and literal request forwarding, including supplied frame offsets and fallback availability. |
| `TypeDrawing.LightTestRejectsOldBackendAndNonPixelRequests` | Explicit project backend interface version and supported request kinds; stale/unsupported calls fail without mutation. |
| `TypeDrawing.UiBatchFallbackAndFailureContracts` | Project batch backend ownership/error and supported-operation negotiation; authored one-pixel resources and literal positions. |
| `TypeDrawingDiagnostics.CopiesRejectedParametersAndKeepsFirstFailureUntilExplicitBegin` | Project diagnostic snapshots copy supplied values and preserve the first failure until an explicit reset. |
| `TypeDrawingDiagnostics.BackendFailureAndFallbackNegotiationHaveDifferentContracts` | Project backend exception containment and unavailable fallback produce distinct documented statuses. |
| `FrameDiagnostics.SeparatesCoreFailureFromFinalizationAndKeepsFirstCause` | Exhaustive Boolean core/end-frame success matrix; first failure metadata and host stage context. |
| `FrameDiagnostics.ShapeFrameRejectionRecordsActualFrameAndResourceLimit` | Authored two-frame image rejects frame seven and records those supplied bounds, without raster output. |

The two build scripts in `build/` compile their own temporary projects. Debug-command discovery verifies adding/removing source files, normal static-library registration and linker dead-code elimination. The Godot profile check verifies atomic API publication, unchanged reloads, dependency-local overrides, missing output regeneration and preservation after an intentional failed generator. Neither script consumes game outputs.

For host tests and their per-scene input basis see [../godot/tests/README.md](../godot/tests/README.md). Window tests are opt-in and require a desktop session. `BUILD_TESTING=OFF` configures and builds the product without fetching GoogleTest or registering tests.
