# VERA20k test maps

These maps are copied unchanged from the `VERA20k-20000Units-30Houses`
test-map package. The package describes its standby validation against VERA
`b38479aad`; this is provenance, not an OpenRA2 performance guarantee.
Only the authored maps are included here, without the Windows launcher,
Python runtime, game executable or retail assets.

| File | SHA-256 | Initial unit mission |
| --- | --- | --- |
| `vera-20000-30p-standby.map` | `e70054ab58b79798dd32b51f2dce73dc96f8546175e9741473fbd695f09eecbb` | Guard |
| `vera-20000-30p-combat.map` | `3d6502652039e0a1eab56454a8d44bfd23540e86b25d805ab6ccb5ce091cb3c7` | Hunt |

Both maps have a 250 × 250 TEMPERATE map header, 30 houses and 20,000 initially
placed `HTNK` units. P01 is the local player with 667 tanks. P01 through P20
have 667 tanks each; P21 through P30 have 666 each. The other 29 houses are
computer-controlled with IQ 0. There are no bases, factories or economy.
This is a local authored campaign sandbox, not a 30-client network lobby.
Combat deaths reduce the live unit count; sustained combat performance is
not guaranteed.

OpenRA2 lists both maps under the test-map category and uses the existing
bundled-resource extraction and native scenario loader. Preserve the original
map bytes when updating these copies, and update the hashes above if the
source package changes.
