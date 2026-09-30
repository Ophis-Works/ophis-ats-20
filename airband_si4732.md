# Airband reception on the Si4732/Si4735 — how it works

How "airband support" (118–136.975 MHz, AM voice) is possible on Si4732/Si4735-based
radios such as the ATS Mini, ATS-20/25 and their firmware variants — even though it
lies far outside the chip's documented tuning ranges.

---

## TL;DR

The datasheet ranges (AM/SW/SSB: 153 kHz–30 MHz, FM: 64–108 MHz) are
**characterized-performance limits, not hard functional walls**. The on-chip firmware's
tuning validator accepts out-of-spec values in AM mode — far enough above 30 MHz to
cover the VHF airband. Host firmware that simply sends ordinary `AM_TUNE_FREQ`
commands for 118–137 MHz gets a working (if deaf) airband receiver. No chip
modification, no special commands, and the SSB patch is not involved.

---

## What the "airband mode" actually is

Firmware that supports airband does nothing exotic at the chip interface. It adds a
band-table entry and sends standard commands:

| Setting | Typical value |
|---|---|
| Band | "AIR", 118.000–136.975 MHz |
| Mode | **AM** (essential — FM mode cannot demodulate AM voice) |
| Step | 25 kHz (or 8.33 kHz where used) |
| Bandwidth | 6 kHz (the widest AM bandwidth the Si4732 offers) |
| Antenna | quarter-wave whip ≈ 55–60 cm (mid-band ~127 MHz) |

The host then sends regular `AM_TUNE_FREQ` (`0x40`) commands. The ROM's range
validation permits the values, the synthesizer locks in VHF, and the AM demodulator —
which is modulation-dependent, not strictly band-dependent — recovers the voice.

**Mode matters**: aviation voice is AM. If a firmware routes 118–137 MHz through the
FM broadcast path, you get silence or distortion. Working implementations select AM
explicitly for the airband range.

---

## Why it works despite the datasheet

- The documented ranges describe where Silicon Labs characterized **performance**
  (sensitivity, selectivity, images) — the tuning validator, however, does not hard-
  block every value outside them. On the Si4732/Si4735 it accepts AM tuning well above
  30 MHz, and the broadband synthesizer/IF chain cooperates.
- Reception is possible because aviation signals are strong: aircraft at altitude are
  powerful line-of-sight AM transmitters, so even a front end with no preselection
  above 108 MHz delivers usable audio.

---

## Why it remains out-of-spec (the honest limitations)

- **Front end is the real bottleneck.** Boards are filtered and matched for HF and
  64–108 MHz broadcast FM. Above 108 MHz there is no preselection: reduced
  sensitivity, image response, and overload from strong FM broadcasters and pagers.
- **Unit-to-unit variance.** Some chips/boards tune and receive well, others mute,
  tune without receiving, or drift. It is an out-of-spec fluke, not a supported mode.
- **Weak-signal performance** is far below a purpose-built airband scanner; ground
  stations are much harder to hear than aircraft.
- **Firmware landscape is split:**
  - Mainline ATS Mini firmware (G8PTN, `esp32-si4732/ats-mini`): airband **not**
    supported — its documentation states so explicitly.
  - Several vendor builds (e.g. ATS-25 Max firmwares) ship an "AIR" band button
    (118–137 MHz, AM) that works natively.
  - The ATS-20/25 mod scene (`goshante/ats20_ats_ex`) added airband experimentally.
  - The PU2CLR SI4735 library can send any frequency — band limits live in the host
    sketch, not the library.

---

## The reliable alternative

For consistent airband reception, the ATS Mini author's own recommendation
(`esp32-si4732/ats-mini` discussion #279) is the classic one: an **external
down-converter** ahead of the antenna input, mixing 118–137 MHz down below 30 MHz —
ideally with a band-pass filter to control images. The chip then operates entirely
inside its characterized range, with its full AGC/filter quality.

---

## Connection to the SSB patch (see `ssb_patch_how_it_works.md`)

- The frequency ranges are **enforced inside the chip's firmware** — the stock AM/FM
  ROM code, or the encrypted SSB patch for SSB mode. The patch download channel is
  encrypted, checksummed and SiLabs-only, so those checks cannot be rewritten.
- The airband hack is the one thing that *does* work purely from the host side: it
  exploits the validator's permissiveness rather than trying to change it.
- Neither the SSB nor the NBFM patch extends frequency coverage — they add
  demodulation modes (SSB, narrow-FM). Range comes from what the validators accept and
  what the analog front end survives.

---

## Legal note

Airband monitoring is receive-only and its legality varies by jurisdiction (some
countries restrict listening to aeronautical communications). Never connect anything
capable of transmitting; these chips are receivers only.

---

## Sources

- ATS Mini documentation (mainline firmware; airband explicitly unsupported):
  <https://esp32-si4732.github.io/ats-mini/manual.html>
- `esp32-si4732/ats-mini` discussion #279, "Air-Band — Is it possible?" (down-
  converter recommendation, 6 kHz AM bandwidth limit):
  <https://github.com/esp32-si4732/ats-mini/discussions/279>
- ATS-25 Max manuals Q&A (vendor firmware "AIR" band entry):
  <https://www.manuals.plus/qa/9000682713/>
- `goshante/ats20_ats_ex` — ATS-20 firmware with experimental airband:
  <https://github.com/goshante/ats20_ats_ex>
- Community guide compiling practical airband settings for Si4732/35:
  <https://freedom251.com/to-widen-the-si4732-35-radio-coverage-ver1-airband/>
