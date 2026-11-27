# 03 — MC68K/AT

Status: future architecture; not a dependency of MC68K/8.

## Vision

An IBM PC/AT-inspired modular desktop with a Motorola MC68010 instead of an Intel CPU. Use period ISA expansion cards and native 68K firmware/drivers. The objective is an enterprise-style retro workstation with VGA, SCSI disk storage, networking and sound, not IBM PC software compatibility.

## Motherboard

- MC68010, 16-bit external data bus, 24-bit address space; clock and RAM/ROM capacity to be decided.
- Main RAM and boot ROM, reset and interrupt logic, ISA bridge, suitable power and mechanical layout.
- One onboard MC68681 for system timer, debugging and UART keyboard.
- Onboard SD card via MC68681 GPIO (same design principle as MC68K/8, exact assignments TBD), useful for rescue, loading drivers and development.
- Keyboard returns to UART; no onboard matrix or PSG requirement.
- ISA 8/16-bit compatible expansion slots. Video, audio, network, mass storage and additional I/O reside on expansion cards.

Four to six slots and conventional PC case geometry are candidates, not fixed requirements. SCSI is the preferred hard-drive interface; IDE/CF is optional rather than a replacement for the SCSI goal.

## ISA bridge

Map ISA I/O and memory into separate CPU address windows. Candidate map: ROM at 0x000000–0x07FFFF, main RAM within 0x080000–0x7FFFFF, legacy ISA memory aperture at 0x800000–0x8FFFFF, I/O at 0x900000–0x90FFFF, onboard devices at 0xA00000–0xA0FFFF. The 1 MiB ISA memory aperture covers legacy VGA/ROM addresses but not full 24-bit ISA memory; additional mapping/banking may be needed. No addresses or RAM size are locked.

Translate Motorola AS, UDS/LDS, R/W, DTACK and interrupt acknowledge to ISA address/data, SA0/SBHE, IOR/IOW, MEMR/MEMW, IOCHRDY, IOCS16/MEMCS16 and IRQ signals. Define byte/word semantics explicitly: Motorola is big-endian, ISA little-endian; byte routing, numerical register conversion and splitting word accesses to 8-bit cards require design and tests. Consider ISA clock, reset, wait states, bus turnaround, address latching, electrical loading and voltage rails. CPLD implementation is a candidate; verify fitting and external bus transceiver needs before schematic commitment.

Initial compatibility: programmed I/O and interrupts for selected cards. Full ISA compatibility is a later goal requiring DMA/DRQ/DACK, arbitration, bus mastering, refresh where needed, IRQ routing and legacy card configuration. A mechanically correct slot does not guarantee universal ISA compatibility. Explicitly record supported card models and features.

## VGA and native driver selection

The first graphics target is Trident TVGA9000/TVGA9000i, with proper ISA VGA register and memory behaviour rather than the early generic framebuffer. Implement standard VGA text, planar and packed graphics, sequencer, CRTC, graphics controller, attribute controller, DAC and chipset-specific banked/SVGA extensions as needed. Use native 68K initialization and drivers.

The x86 expansion BIOS is **not executed**. Use the ROM as identification data: read candidate expansion-ROM headers/signatures, validate bounds/checksum where applicable, inspect recognized vendor/chipset strings or identifiers and match to a driver catalog. A ROM signature alone is not a unique chipset ID; use safe device-specific probing or explicit configuration if needed. Load the appropriate native driver into 68010 RAM (from SD or other available boot storage). Define driver ABI, discovery order, fallback and diagnostics; do not require the SCSI driver to load from an inaccessible SCSI disk. A minimal ROM monitor and SD bootstrap avoid circular dependencies.

Potential references for future review: Sergey Kiselev ISA Super VGA (https://github.com/skiselev/isa-super-vga), EmuTOS Nova driver (https://github.com/emutos/emutos/blob/master/bios/nova.c). Verify chipset/board differences and licensing before reusing code.

## Expansion devices

| Class | Initial candidate / intent |
| --- | --- |
| Graphics | Trident TVGA9000 ISA VGA |
| Storage | ISA SCSI host adapter; specific controller TBD |
| Networking | NE2000-compatible ISA Ethernet candidate |
| Audio | OPL2/AdLib or other ISA sound card |
| Multi-I/O | 16550 serial, parallel and optional IDE |
| Later | DMA-dependent floppy/sound and more demanding cards |

SCSI is strategically interesting because the Macintosh Plus/uMac project already has experience with NCR 5380 and SCSI block-device emulation. Reuse protocol knowledge, test images or target abstractions where suitable, but do not assume an ISA adapter uses the Macintosh controller register map. Select a documented adapter supporting programmed I/O for first bring-up, then add interrupt/DMA features if needed. Emulate the actual adapter and a disk-image-backed SCSI target; write native 68K initiator/driver and test commands, status, errors and boot/loading.

## Development path

1. Preserve first-target MC68K/8 progress; specify ISA bridge and native driver architecture separately.
2. Extend configurable emulator with 68010 profile, native DUART/SD/UART keyboard and ISA bridge.
3. Verify 8/16-bit bus transfers, endian behaviour, IOCHRDY, interrupt routing and unsupported-cycle handling.
4. Implement standard VGA then TVGA9000 extensions; native ROM identification/driver loader and VGA monitor.
5. Select and emulate an ISA SCSI adapter; build native driver and disk support.
6. Add selected network/audio/multi-I/O cards; document compatibility matrix.
7. Develop bridge RTL, validate with simulation/fitting and physical timing, then design/manufacture motherboard.
8. Bring up a workstation with native Tima software and eventually Retro-WM graphics.

MC68K/AT is a future dream with a concrete direction, not an approved PCB or assertion that arbitrary ISA cards will work.
