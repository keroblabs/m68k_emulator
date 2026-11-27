# 00 — Introduction, goals and dreams

Status: architectural direction agreed 2026-11-27; detailed designs remain subject to review.

## Vision

Build our own Motorola 68K computers using vintage components, native firmware and modern development tools. The emulator is a hardware-development and software-debugging environment, not a single fixed virtual machine or an IBM PC/Macintosh compatibility product. Develop and test firmware before manufacturing hardware; use the same target firmware on the corresponding physical board wherever possible.

Two distinct machines share the emulator infrastructure:

- **MC68K/8 (Board 004 evolution):** compact MC68008 DIP48 computer with banked SRAM, V9958 video, YM2149 sound, a scanned keyboard, one MC68681 and SD storage. This is the first physical target.
- **MC68K/AT:** future MC68010 motherboard inspired by the IBM PC/AT expansion ecosystem, with 8/16-bit ISA slots, native 68K drivers, VGA, networking, sound and preferably SCSI storage on expansion cards.

Tima/Plus remains a distinct Macintosh Plus project using uMac and original Macintosh hardware interfaces. Share useful Tima framework and driver concepts with these machines where appropriate; do not merge unrelated hardware models. Retro-WM may later benefit from V9958 acceleration and ISA VGA backends; neither is a prerequisite for initial bring-up.

## Goals

1. Preserve a working Board 004 baseline and correct discrepancies between firmware, emulator and physical devices.
2. Make the emulator configurable at runtime so hardware topology belongs to a board profile, rather than being hard-coded in the application.
3. Complete MC68K/8, including a ROM monitor, FAT16/FAT32 SD access and loading native 68K applications.
4. Manufacture and bring up MC68K/8 with the same firmware developed in the emulator.
5. Port games and demos, build reusable graphics/audio/input libraries, and explore Tima on the hardware.
6. Develop MC68K/AT afterwards: a modular workstation using period ISA expansion cards, native 68K software and potentially a SCSI desktop storage system.

## Roadmap and gates

| Stage | Work | Exit evidence |
| --- | --- | --- |
| 1 | Correct Board 004 baseline | ROM boot, text output, keyboard and DUART regression checks |
| 2 | Runtime-configurable emulator | Existing baseline plus alternate UART/matrix profiles; invalid configuration rejected |
| 3 | MC68K/8 memory and device topology | Fixed/banked RAM, one DUART and agreed peripheral map verified |
| 4 | V9958 and YM2149 | Video modes, expansion VRAM, graphics commands, interrupts and audible PSG output tested |
| 5 | GPIO SPI SD interface and filesystem | Sector read/write, FAT16/FAT32, host-copyable image, application loading |
| 6 | CPLD and physical design | Schematic review, fitting, timing, voltage and pin-budget checks; can overlap stages 3–5 |
| 7 | Physical bring-up | Monitor and representative firmware/apps run on actual board |
| 8 | Games and applications | Native software demonstrating graphics, sound, input and storage |
| 9 | MC68K/AT | ISA bridge, native VGA, selected I/O and SCSI devices; separately staged |

Stages describe dependencies and milestones, not dates or completed work. The first board takes priority; ISA development is not on its critical path.

## Document series

- [01 — Configurable emulator](01-configurable-emulator.md)
- [02 — MC68K/8 (Board 004)](02-mc68k-8-board-004.md)
- [03 — MC68K/AT](03-mc68k-at.md)

No software implementation or hardware schematic is changed by this documentation series.
