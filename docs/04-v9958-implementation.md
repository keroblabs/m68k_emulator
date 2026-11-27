# 04 — Yamaha V9958 bring-up (Board 004 + emulator)

Status: **implemented functional starting point, not hardware-verified or cycle-accurate**. Use branch `v9958-board004-emulator` until review and FPGA/CPLD wiring validation. The existing TMS9918 is retained.

## Build selection

| Component | Existing legacy default | Select V9958 |
| --- | --- | --- |
| Board 004 ROM | TMS9918 | `make -C board_004/make USE_V9958=1` (run from project's expected build working directory; may require local toolchain path overrides) |
| SDL emulator | TMS9918 | `cmake -S emulator -B build-v9958 -DM68K_VIDEO_V9958=ON` then `cmake --build build-v9958` |

**Both sides must use the same VDP selection.** The Board 004 VT100 monitor and original TMS9918 text-mode call sites remain unchanged. With V9958 enabled, the TMS-compatible register driver uses the new V9958 port/register driver, and the video device is V9958. The register-driver fixes (write-address bit `0x40`, preserving full register values) also apply in legacy mode.

## Current address/port contract

The present Board 004 firmware still uses legacy I/O base `0x70000`. The V9958 shares the historic video allocation at `0x70C00` rather than silently migrating to the proposed `0x48000` candidate window. Four **8-bit** CPU addresses are:

| CPU address | Read | Write | V9958 native port |
| --- | --- | --- | --- |
| `0x70C00` | Selected status register | VRAM address/register control latch | 1 |
| `0x70C01` | Buffered VRAM byte | VRAM byte | 0 |
| `0x70C02` | Not defined | Palette first/second byte | 2 |
| `0x70C03` | Not defined | Indirect register value | 3 |

The CPU address order deliberately retains the legacy firmware's **control at +0, data at +1** convention, which differs from the usual MSX I/O port numbering. Hardware decoder must drive the V9958 MODE0/MODE1 accordingly. `VDP9958_ADDR` is an alias of `VDP9918_ADDR`; **the two devices are mutually exclusive**. On MC68008 DIP48, each byte is a separate bus cycle. Before schematic freeze, verify `CSW`, `CSR`, MODE pins, byte-read and write timing, data bus, `RESET`, and how the VDP's `/WAIT` extends CPU `DTACK`. Never assume a direct 5 V `/WAIT` to CPU connection is sufficient.

Keep existing memory map unchanged in this PR. The `0x40000–0x7FFFF` I/O redesign and 448K fixed + 64K banked RAM are a separate project milestone.

## Driver services (`board_004/vdp9958`)

- V9958 direct register write (entire 8-bit value; registers 0–46, valid numbers only).
- Set 17-bit VRAM CPU address via R14 plus 2 control bytes; full 128 KB address space.
- Byte/block read and write; palette R16/port 2 RGB 3-bit writes.
- Status pointer R15; indirect register pointer R17 and automatic increment.
- R32–R46 15-byte command descriptor, command-ready via S2.CE, command opcodes.

**Concurrency:** A control-port write pair must not be split by an ISR that reads VDP status. Board code must protect these pairs if it enables VDP interrupts. The current ROM does not yet hook the V9958 interrupt pin. Firmware that uses the command engine must poll CE/TR with a *bounded* timeout before dependent operations; `vdp9958_command()` intentionally does not spin forever.

## Emulator coverage

Implemented: separate V9958 core; 128 KB VRAM and 64 KB expansion RAM; 4 CPU ports; registers 0–27 and 32–46 (excluding nonexistent 24, 28–31); 17-bit CPU VRAM address/read-ahead; legacy 16 KB wrap in old modes; R15 status selection; R16 palette writes and R17 indirect writes; S0 VBlank latch, S1 identification, S2 CE/TR for streaming, IRQ-pending signal API; display blanking, 192/212 line selection, Text 1/Text 2, Graphics 1/2/3/Multicolor and bitmap Graphics 4/5/6/7 render paths; programmable palette and GRAPHIC7 basic 8-bit RGB; immediate functional POINT/PSET, LMMV, LMMM, HMMV, HMMM, YMMM, and streaming LMMC/HMMC; basic logical raster operations; expansion command-memory selection (`R45.MXD/MXS`). SDL renders complete frames scaled and centred for 256- and 512-pixel video.

**Explicit limitations (not yet hardware-equivalent):** The renderer does not implement sprites, YJK/YAE, fine/vertical scrolling, interlace, line interrupt timing, or legacy detailed sprite/status collision logic. G6/G7 byte interleaving and screen-specific address/page conventions require verification on hardware; graphics rendering is a functional baseline, not certified pixel accuracy. LINE/SRCH/LMCM and more nuanced command rules/transfer handshakes are not yet implemented; command fills/copies execute instantly rather than taking VDP time. CPU accesses do not model V9958 `/WAIT` or DRAM contention. VDP IRQ is exposed as a query but not yet wired to the emulator's MC68681 interrupt arbitration or physical board schematics. Do not treat `vdp9958_irq_pending()` as a connected CPU IRQ. Reads from palette/indirect ports return open-bus `0xff` in the emulator. See hardware gates below.

## Host-side smoke test (no M68K cross compiler)

```sh
cc -std=c11 -Wall -Wextra -Werror -pedantic \
  -I emulator/peripherals \
  tests/vdp9958_test.c emulator/peripherals/vdp9958_emulator.c \
  -o /tmp/vdp9958_test
/tmp/vdp9958_test
```

This exercises high-byte register preservation, R17 auto-increment, R14 address extension, read-ahead, palette, S0/S1/S2, bitmap HMMV, expansion RAM writes and HMMC data handshaking. It uses only host C and can run on Ubuntu, before an M68K toolchain is installed. Test **both** legacy and V9958 SDL builds locally; confirm that the ROM uses the matching driver.

## Fedora Cups / hardware validation gates

1. Cross-compile Board 004 in both modes; check size and ROM boot in the matching emulator. Compare terminal output, cursor, wrap, clear and font rendering to legacy TMS9918.
2. Run the headless register tests on Ubuntu/Fedora; add disassembly/port-watch tests around the direct register writes and command programming.
3. Validate all four physical ports, especially mode/address swap and first/second control writes. Check DTACK stretch, VDP `/WAIT` and minimum VRAM access time on a logic analyser.
4. Verify 128 KiB VRAM bank boundary, 16 KiB wrap in TMS modes, 64 KiB expansion via VDP commands, and 192/212 line modes. Add graphics screen 5–8 and command reference patterns.
5. Complete sprite modes, YJK/YAE, command engine, scroll and status/line interrupt behaviour, then connect interrupt wiring and verify CPU acknowledgment.
6. Separately migrate the whole machine to the agreed MC68K/8 banked SRAM and 256 KiB I/O map; only then lock the physical decode.

Reference material: [V9958 MSX-VIDEO Technical Data Book](https://map.grauw.nl/resources/video/yamaha_v9958_ocr.pdf), [VDP programming tutorial](https://map.grauw.nl/articles/vdp_tut.php), and [MSX VDP screen mode tables](https://www.msx.org/wiki/Screen_Modes_description). The implementation is original to this project; these documents are interoperability references.