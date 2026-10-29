# Product unit contract provenance

These tests specify project storage, scheduling, caching and logging contracts.
Their inputs are constructed in memory or in a temporary directory. They do not
load a game installation, captured output, external images or an executable.
The source files retain their project history and repository licensing.

Each target uses `GTest::gtest_main`. Private header dependencies are listed next
to the target in `cmake/Tests.cmake`; these tests add no public API.

## Render storage and scheduling

| Case | Input and independent expectation |
|---|---|
| `DrawResourcePool.QueuedVersionKeepsItsAddressAndBudget` | A 16-word, four-descriptor pool holds two eight-word allocations. Shared ownership pins the first interval; only its last release permits reuse, with a new version. |
| `DrawResourcePool.CoalescingReclaimsBothNeighbours` | Three contiguous eight-word intervals partition 24 words. Releasing all three in outer-then-middle order must permit one 24-word allocation. |
| `DrawResourcePool.DescriptorLimitAndRendererThreadRelease` | Two descriptors limit a 1024-word pool independently of byte capacity. A joined second thread releases one owner, permitting the remaining 1022 words to be allocated. |
| `DrawResourcePool.StatisticsDistinguishFragmentationAndPinnedRetirement` | Owner counts and interval arithmetic independently give zero early releases, then two releases, 16 free words in two eight-word holes, and finally one 24-word hole. |
| `SceneRowTable.QueuedSnapshotsAndCoalescedPublications` | A fixed local integer generator constructs 100 transactions. Separate `std::map` values provide the current and previous snapshot oracle; applying reported changes must reconstruct the current map. |
| `SceneRowTable.ChangeVisitsOnlyReplacedRows` | A 2000-row table changes one shared row. Exactly that key must be reported; the previous snapshot must retain the previous value. |
| `ScenePageCache.RejectsViewDependentDepthAndDestinationBlends` | Hand-built project packet words distinguish plain cached records from view-dependent depth and destination-reading blends. The cache must reject records whose output depends on the view or destination. This tests cache admission, not pixel fidelity. |
| `ScenePageCache.WrappedDepthProofIncludesViewportAndSourceEdges` | For the project's 16-bit depth field, explicit endpoint substitution checks `uint16(base + camera + texel) - camera == base + texel`. Source-row, viewport-row and byte-depth extremes provide an independent mathematical cache-admission oracle. It does not validate the origin of depth values or a game's drawing behavior. |
| `PixelRun.PreservesOrderedClippedBinMembershipAndResourcePrefix` | Four constructed rectangles cover clipping, offscreen and corner cases. Exhaustive pixel intersection determines each 32-pixel bin's ordered members. The project schedule encoding uses kind 13 and retains its pre-existing resource prefix. |
| `PixelRun.EmptyRunDoesNotDispatchOrOverwriteResourceOffsets` | Two wholly offscreen rectangles have no visible bin. Dispatch dimensions must be zero and the supplied resource prefix must remain intact. |
| `VoxelResource.CompositeHitsUseImmutableChildrenAndAllPlacementFields` | A generated two-by-two image uses three distinct nonzero labels and transparency. Row-major placement supplies exact destination indices; changing child identity, position, clip or crop must invalidate the cache while published owners retain the old image. |
| `VoxelResource.CompositeOrderAndCroppedIndexesArePreserved` | Two generated one-pixel layers overlap. The last submitted opaque label wins; reversing order changes the result. Clearing the cache must not invalidate the retained published image. |

Pixel labels such as `0x1000001` use the project's composite pixel encoding.
No expected image is imported. Packet fields and cache keys are internal project
contracts; these tests make no compatibility claim about another implementation.

## Runtime diagnostics

| Case | Input and independent expectation |
|---|---|
| `RuntimeLogFixture.FlushesContextAndCopiedTagMetadataBeforeShutdown` | A generated tag and supplied context strings exercise escaping, flushing, disabled logging and copied metadata after deletion. The expected identifier is read from the live object, not fixed by an object layout or counter value. |
| `RuntimeLogFixture.BoundsRotationAndPreservesContextAndPreviousSession` | Repeated generated text fills the documented project rotation policy: each segment is at most 2 MiB, three backups remain, and each segment retains context. Reconfiguration records the previous session and resets context. |
| `RuntimeLogFixture.RejectsInvalidPathsAndRotationDirectoriesWithoutMovingLogs` | Temporary files and directories create invalid destinations and a rotation collision. Failure must disable logging, preserve existing bytes and avoid moving logs. |
| `RuntimeLogFixture.PersistsCopiedRenderFailureAndStagesBeforeTeardown` | All snapshot numbers, filenames, map labels and messages are supplied literals. The logger must serialize those inputs and retain the copied snapshot after shutdown; no named map or external source file is opened. |
| `RuntimeLogFixture.SnapshotWriteFailureDoesNotErasePrimaryAndStillWritesEvents` | A temporary directory blocks the intended snapshot filename. Failure must clear the output path, preserve the primary diagnostic and still record the failed write event. |

The project logging schema and rotation policy are defined by
`core/include/api/runtime_diagnostics.hpp`, `core/src/runtime_diagnostics.hpp`
and `core/src/runtime_diagnostics.cpp`. The tests verify emitted data with an
independent filesystem reader. They do not assert original object layout,
dispatch slots or exact gameplay state transitions.


## Text encoding adapter

`text/text_encoding_tests.cpp` contains literal strings and byte arrays already
maintained in this repository. It loads no document, game data or reference
capture. The component is the internal host text adapter declared in
`bridge/src/text_encoding.hpp`, backed by the pinned upstream ICU implementation
and conversion data (upstream license retained by the dependency adapter).
The target declares this private-header access explicitly and uses standard
GoogleTest main. Enabling the host text component, or building it independently,
is required; the pure core configuration does not create this component.

| Case | Input and independently explained expectation |
|---|---|
| `TextEncoding.Utf8AndBoundaries` | UTF-8 literals have 12 bytes. The output includes a terminating null, so a three-byte buffer cannot hold six bytes. Empty input succeeds and invalid pointer/length pairs fail with cleared output. These are adapter API contracts. |
| `TextEncoding.SimplifiedDocument` | Literal GB18030 bytes decode to the supplied simplified Chinese literal. Sixteen copies provide a document-sized detector sample; a subsequent UTF-8 field remains readable. Independent standard-library GB18030 decoding agrees. |
| `TextEncoding.TraditionalDocument` | Literal Big5 bytes decode to the supplied traditional Chinese literal, independently checked with a separate Big5 codec. The same document/UTF-8-field policy applies. |
| `TextEncoding.JapaneseDocument` | Literal Shift-JIS bytes decode to the supplied Japanese literal, independently checked with a separate Shift-JIS codec. No original game text capture is consumed. |
| `TextEncoding.RussianDocument` | Literal Windows-1251 bytes decode to the supplied Russian literal, independently checked with a separate Windows-1251 codec. |
| `TextEncoding.Iso2022JapaneseDocument` | ISO-2022-JP escape/state sequences decode to the supplied literal under an independent codec. The escape-prefixed sample must pass through the detector instead of being accepted as plain ASCII. |
| `TextEncoding.InvalidTextAndAmbiguousSample` | A two-byte sample is too short for the adapter's confidence policy and leaves no inferred encoding. Explicit GB18030 decodes D6 D0 as U+4E2D; FF is invalid, must fail and clear output. This does not claim universal detector accuracy. |
| `TextEncoding.UnicodeSignatures` | FF FE selects UTF-16 little endian; 2D 4E 87 65 decodes to U+4E2D/U+6587 and a later field inherits that endian mode. EF BB BF is a UTF-8 signature removed from the output. Independent Unicode codecs agree. |

The expected Unicode strings, buffer sizes and error statuses are public format
or project adapter contracts. No fixed original structure, speed, object state,
address or output golden supplies these expectations. All eight existing test
bodies remain unchanged. These cases cover this sample corpus and adapter
policy; they do not certify every charset detector result or locale.
