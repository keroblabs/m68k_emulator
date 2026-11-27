# 01 — Configurable emulator

Status: architecture direction agreed; config syntax and APIs proposed, not implemented.

## Existing baseline

At main commit 7d2df9d40ce0a4d7382aea0882611b7caab4f7f4, firmware exists in board_001, board_003 and board_004. The emulator defaults to Board 004's ROM, 68000 Musashi core (68008 ISA), 8-bit peripheral address mapping and matrix keyboard. It also retains optional 68010 core, 16-bit byte-lane mapping, UART keyboard and an early generic framebuffer display. YM2149 GPIO and registers are implemented but audio synthesis is not. The TMS9918 model provides VRAM and rendering. Board 004 currently has two emulated DUARTs; the new MC68K/8 target requires one.

## Design principle

One emulator executable supports multiple machine profiles. The board-specific configuration describes the actual hardware, not an arbitrary collection of firmware compile flags. Preserve the ability to reproduce old profiles while moving duplicated firmware trees into archival history after verification. Board firmware may remain separately built for each target; runtime emulator configuration does not imply automatic firmware portability.

Profiles define CPU model, address width, clock, external bus width, ROM path and image size, RAM and banking, memory/I/O decode, device type and instances, GPIO wiring, interrupt routes, keyboard adapter, display model, storage image and optional expansion buses/cards. Keep the CPU ISA/core model independent of external bus width; use Musashi's 68000 core for 68008, with explicit 20-bit physical address semantics for DIP48. Do not claim cycle accuracy from a core selection alone.

## Proposed structure

- Machine configuration loader and validator, with explicit defaults and meaningful errors.
- Machine builder: instantiate devices, connect callbacks/interrupts, register address ranges, reset in defined order.
- Bus layer: CPU memory, native peripherals, optional ISA I/O and ISA memory, access sizes, byte lanes and endianness.
- Independent peripheral models: DUART, PSG, TMS9918, V9958, SD card, keyboard matrix, generic display and future ISA VGA/SCSI/network/audio.
- Host adapters: SDL graphics/audio/input, serial backend and disk-image storage. Host UI must not bypass guest hardware interfaces.
- Timing and interrupt scheduler: distinguish deterministic emulated time from host presentation; support clock-rate configuration and debugging.

Keep the generic display as a bring-up/test option, not the target VGA implementation. Device rendering feeds a common host display interface. Audio output is similarly separate from device register semantics.

## Profiles and configuration

INI is a candidate format; schema and exact addresses are not yet frozen. Example of intent:

```ini
[system]
cpu = 68008
address_bits = 20
data_bus_width = 8
rom = ../firmware/m68k_rom.bin
[memory]
rom_kib = 256
io_kib = 256
sram_kib = 1024
fixed_ram_kib = 448
bank_window_kib = 64
[video]
type = v9958
vram_kib = 128
expansion_vram_kib = 64
[sound]
type = ym2149
[duart0]
type = mc68681
[keyboard]
type = matrix
select = ym2149.port_a
sense = duart0.ip5
[storage]
type = sd_spi
controller = duart0.gpio
image = sdcard.img
```

This is schematic configuration, not a runnable file; SD pin mapping, addresses and clock are deliberately omitted pending hardware decisions. Resolve relative paths consistently against a documented base. UART keyboard profiles instead select a DUART channel; matrix profiles require a PSG and appropriate DUART input. Validate GPIO conflicts, duplicate address ranges (unless intentional aliases), incompatible bus/CPU options, missing ROM/images, and unsupported devices. Fail explicitly rather than silently substituting hardware.

Candidate profiles: board_004_legacy (reproduces existing defaults), mc68k8 (new target), mc68k10 (optional future native 16-bit configuration), mc68k_at (ISA). CLI proposal: cpu_emulator --config <profile>. Keep debug options independent of hardware topology.

## Peripheral and bus accuracy

CPU byte/word/long operations must obey actual bus width and byte lanes, including split cycles where applicable. Peripheral register access should not depend on host endianness. Preserve TMS9918 read-ahead and register semantics; add V9958 as a separate selectable model with its own four ports, command engine and 128+64 KiB memory. Model SD SPI at the signal/protocol boundary, driven by DUART GPIO, not by direct guest file access. Back the emulated card with a persistent sector image usable on Linux. Model ISA cards behind an ISA bridge independently of the Motorola CPU.

## Debugging and verification

Provide ROM-load diagnostics, CPU register inspection, instruction breakpoints, memory watchpoints, bank mapping inspection, interrupt tracing, VDP registers/VRAM inspection, SPI transaction logging and configurable serial output. These are roadmap items.

Regression tests should exercise native memory and byte-lane mapping, ROM boot, timer and IRQ behaviour, keyboard UART/matrix modes, VDP transactions, YM2149 GPIO and audio, bank isolation and SD initialization/read/write. A headless test mode is desirable. Avoid absolute developer-specific build/ROM paths and establish reproducible builds.

The current code review identified VDP write-address setup and register-bit truncation, slow matrix scan timing, unassigned key events emitting NUL, stale GPIO aliases, partial 16-bit VDP mapping, and incomplete interrupt arbitration. Address these explicitly before treating Board 004 as a verified hardware baseline. Test real-device behaviour rather than relying solely on agreement between emulator and driver.
