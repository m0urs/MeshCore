# mups18 – MU PowerSaving firmware (based on PowerSaving-v18)

Custom MeshCore firmware maintained by Michael Urspringer.
Branch `mups18` is **generated** by `build-mups18.sh` and force-pushed on every
rebuild – never commit to it directly. Releases are git tags `mups18-<n>`;
the firmware reports them as `v1.18.0.mups<n>` (test builds: `v1.18.0.dev-<hash>`).

> **Status:** first release is `mups18-1` (see [Release history](#release-history)).
> When updating from mups17, note that WiFi credentials have to be set again
> (`set wifi.ssid` / `set wifi.pwd`, see below).

## Contents

1. [What is included](#what-is-included)
   - [Core features and optional features](#core-features-and-optional-features)
   - [Changes compared to mups17](#changes-compared-to-mups17)
2. [Additional and changed CLI commands](#additional-and-changed-cli-commands-compared-to-meshcore-main)
3. [Changed default settings](#changed-default-settings)
4. [Release history](#release-history)

---

## What is included

| Layer | Source | How it gets in | Notes |
|---|---|---|---|
| Base | [IoTThinks/MeshCore `PowerSaving-v18`](https://github.com/IoTThinks/MeshCore/tree/PowerSaving-v18) | branch base | RX duty-cycle power saving for repeater, room server, sensor and (new in PowerSaving-v18) companion, based on MeshCore `main` v1.18.0 |
| Release fixes | [meshcore-dev/MeshCore `main`](https://github.com/meshcore-dev/MeshCore/tree/main), pinned to v1.18.0 (`81dbae59`) | merge | pinned so later `main` commits only come in deliberately |
| PR #1349 | [Allow setting RTC clock backwards, fix elapsed-time underflow](https://github.com/meshcore-dev/MeshCore/pull/1349) | squashed cherry-pick | conflict in `ArduinoHelpers.h` resolved automatically (rerere) |
| PR #2834 | [Separate replay counters for login and messages](https://github.com/meshcore-dev/MeshCore/pull/2834) | squashed cherry-pick | repeater / room server / sensor |
| PR #3260 | [Send flood ACK on DM retries](https://github.com/meshcore-dev/MeshCore/pull/3260) | squashed cherry-pick | – |
| PR #3536 | [Beeps for silence and unsilence](https://github.com/meshcore-dev/MeshCore/pull/3536) | squashed cherry-pick | companion UI only; T1000-E button debounce 20 ms |
| PR #2586 | [Repeater: set reply path (`outpath`)](https://github.com/meshcore-dev/MeshCore/pull/2586) | squashed cherry-pick | replaces the `outpath` command that PowerSaving-v17 had (removed in PowerSaving-v18); conflict in `simple_repeater/MyMesh.cpp` (ACK path with PR #3260) resolved once, then automatic (rerere) |
| PR #2670 | [Repeated sending of direct packets](https://github.com/meshcore-dev/MeshCore/pull/2670) | port branch `mu/pr-2670-ps18` | **partial port**, see below |
| DMC packet filter | [Dutch-MeshCore `dmc-dev`](https://github.com/Dutch-MeshCore/MeshCore/tree/dmc-dev) (`34d1c16f`) | port branch `mu/dmc-filter-ps18` | same as mups17: **repeater only, off by default**, own fixes and defaults |
| Airtime reserve for scoped floods | idea and design from [ACETyr/MeshCore](https://github.com/ACETyr/MeshCore) | port branch `mu/scoped-reserve-ps18` | same as mups17: **repeater only, off by default** |
| Own changes | branch `mu/mups18-own` | merge | default settings (radio preset, 10 % duty cycle, repeater values), this document |

### PR #2670 – partial port

Same scope as in mups17 (PR head `33e17bbd`): resend of direct packets
(Dispatcher/Mesh/Packet), `max.resend` setting and statistics, non-invasive
channel check before a resend. **On by default** with `max.resend 3` (the PR
uses `2`); switch off with `set max.resend 0`.

Left out on purpose, because they conflict with RX power saving:

- noise-floor median estimator (needs continuous RX sampling every 50 ms)
- RX-desync watchdog (the radio is legitimately not in RX mode while duty-cycling)

Differences to the mups17 port:

- **`Mesh.cpp`:** merged with v1.18.0 change #3573 – the direct retransmit
  delay is computed before the node removes itself from the path (as in
  v1.18.0), so relayed ACKs get the direct TX delay too.
- **RX power saving guard (new):** the resend channel check of repeater and
  room server skips the RSSI read while RX power saving has the chip in its
  sleep phase – the same condition the PowerSaving code uses for its own SPI
  probes (`isRxPowerSavingSleeping()` in `RadioLibWrappers.h`). A packet being
  received still blocks the resend.
- **Companion:** `get/set max.resend` is part of the official command handler,
  so it works in the rescue CLI and the companion CLI.

### DMC packet filter

Unchanged from mups17 (same upstream commit `34d1c16f`, same own fixes:
channel block by key, limiters in sync, own defaults in
`examples/simple_repeater/FilterDefaultsMups.h`). See
[MUPS17.md](https://github.com/m0urs/MeshCore/blob/mups17-release/MUPS17.md#dmc-packet-filter)
and [`docs/packet_filter_reference.md`](docs/packet_filter_reference.md).
Newer `dmc-dev` changes (after `34d1c16f`) are not included yet.

### Airtime reserve for scoped floods

Unchanged from mups17 (`set fwd.scoped.reserve <0-100>`, default `0` = off,
`get fwd.scoped.stats`). See
[MUPS17.md](https://github.com/m0urs/MeshCore/blob/mups17-release/MUPS17.md#airtime-reserve-for-scoped-floods).

### Core features and optional features

**Core features – on by default:**

| Feature | Default | Switch off |
|---|---|---|
| RX power saving (PowerSaving-v18; all roles) | on | `set radio.rxps off`; companion additionally `powersaving off` (see below) |
| Repeated sending of direct packets (PR #2670, all roles) | on, `max.resend 3` | `set max.resend 0` |

**Optional features – off by default:**

| Feature | Default | Switch on |
|---|---|---|
| DMC packet filter (repeater) | off | `filter on` (start with `filter dryrun on`) |
| Airtime reserve for scoped floods (repeater) | off | `set fwd.scoped.reserve <1-100>` |

Always active (bug fixes / small behaviour changes, not switchable):
PR #1349, PR #2834, PR #3260, PR #3536, PR #2586 (only when `outpath` is set).
The internal parts of PR #2670 (separate ACK de-duplication table, packet
hash cache) are always compiled in; with `max.resend 0` no resend is ever
scheduled.

### Changes compared to mups17

| Topic | mups17 | mups18 |
|---|---|---|
| Base | PowerSaving-v17, MeshCore v1.17.1 | PowerSaving-v18, MeshCore v1.18.0 |
| Companion power saving | yes (fixed `balanced` profile) | from PowerSaving-v18: `powersaving on\|off` (default on), RX duty cycle default `balanced` (level 6, preamble 16) with the full `radio.rxps` commands; ESP32 BLE builds: BLE sleep and CPU 10–80 MHz without light sleep; nRF52: sleeps when idle |
| `outpath` (repeater) | from PowerSaving-v17 | from PR #2586 (PowerSaving-v18 no longer has it) |
| WiFi credentials (WiFi companion) | own: `/wifi_config` via rescue CLI (PR #2706 / #2720 adapted) | **official** v1.18.0 commands `set wifi.ssid` / `set wifi.pwd` etc. (see below); own solution dropped |
| `af` / `dutycycle` on the companion | own rescue CLI commands | **official**: shared radio command set in v1.18.0, available in the rescue CLI **and** the companion CLI (see below); own code dropped |
| `get cad` channel busy counter | own fix (#24) | counted by PowerSaving-v18 itself; own fix dropped |
| Companion rescue CLI CRLF / LF fix | own fix | not taken over yet – to be tested with the v1.18.0 rescue CLI first |
| `get pin` (companion) | own rescue CLI command | dropped (`set pin` exists) |
| `max.resend` on the companion | own rescue CLI command | in the official command handler (rescue CLI **and** companion CLI) |
| Resend channel check in the RXPS sleep phase | reads RSSI without guard | RSSI read skipped in the sleep phase (see PR #2670 above) |

---

## Additional and changed CLI commands (compared to MeshCore `main`)

Full syntax and details: [`docs/cli_commands.md`](docs/cli_commands.md).

### New commands

| Command | Applies to | Source | Description |
|---|---|---|---|
| `get radio.rxps` / `set radio.rxps …` | all roles | PowerSaving-v18 | RX duty-cycle power saving (companions since PowerSaving-v18) |
| `powersaving` / `powersaving on\|off` | Companion | PowerSaving-v18 | Shows / switches the companion power management (default `on`), saved in the preferences. ESP32 BLE builds: BLE sleep and CPU frequency 10–80 MHz (no light sleep), applied immediately; switched off: fixed CPU frequency. nRF52: sleep when idle. Independent of `set radio.rxps`. |
| `get reboot.interval` / `set reboot.interval <hours>` | Repeater, Sensor | PowerSaving-v18 | Periodic reboot, `0` = off (default) |
| `powerlog` | Repeater, Room Server, Sensor | PowerSaving-v18 | Last reset reason; on nRF52 also last shutdown reason and boot voltage |
| `sensor` | Repeater, Room Server, Sensor | PowerSaving-v18 | Shows the I2C and GPS pins of the board and whether GPS is configured |
| `get cad` | all roles | PowerSaving-v18 | Shows `on`/`off` and the channel-busy counter |
| `get outpath [path]` / `set outpath <hop1,hop2,…\|path\|direct\|clear\|flood>` | Repeater (remote admin only) | PR #2586 | Override the direct reply path to the logged-in client; `get/set outpath path` uses the path observed after the client's latest flood login |
| `filter …` | Repeater | DMC packet filter | see `docs/packet_filter_reference.md` |
| `get max.resend` / `set max.resend <0-3>` | all roles | PR #2670 | Max. resend attempts for direct packets, `0` = off, default `3`. On the repeater `get` also shows the resend ratio. |
| `set fwd.scoped.reserve <0-100>` / `get fwd.scoped.reserve` / `get fwd.scoped.stats` | Repeater | own (idea: ACETyr) | Airtime reserve for scoped floods |

### Companion: official v1.18.0 commands used instead of own ones

In v1.18.0 the companion handles CLI commands in one place. The same
commands work in the **rescue CLI** (serial console) and in the **companion
CLI** (protocol command `CMD_RUN_CLI_COMMAND`, protocol V14+).

| Command | Description |
|---|---|
| `get af` / `set af <0-9>` | Airtime factor |
| `get dutycycle` / `set dutycycle <1-100>` | Duty cycle in percent (sets `af = 100/dc - 1`); EU: `set dutycycle 10` |
| `get cad` / `set cad on\|off` | Hardware CAD before TX |
| `get max.resend` / `set max.resend <0-3>` | Resend attempts for direct packets (PR #2670, added by mups), `0` = off |
| `set pin <pin>` | BLE PIN |
| `set wifi.ssid <ssid>` / `get wifi.ssid` | WiFi SSID (WiFi-capable builds, e.g. Heltec V3); reboot to apply |
| `set wifi.pwd <pwd>` / `get wifi.pwd` | WiFi password; **`get wifi.pwd` shows it in plain text** |
| `set wifi.enabled 0\|1` / `get wifi.enabled` | Switch the WiFi interface off/on; reboot to apply |
| `set wifi.clear` | Delete stored SSID and password (back to the compiled-in values) |
| `get wifi.status` / `get wifi.ip` | Connection state / IP address |

Without stored credentials the compiled-in `WIFI_SSID` / `WIFI_PWD` from
`platformio.ini` are used. WiFi starts only if `wifi.enabled` is `1` (default)
and an SSID is known – builds without a compiled-in SSID (e.g. the Heltec V3
BLE companion) keep WiFi off until `set wifi.ssid` is used. The mups17 commands `wifi_ssid`, `wifi_pwd`,
`wifi_commit`, `wifi_show`, `wifi_clear` and the file `/wifi_config` no
longer exist; credentials have to be set again with `set wifi.ssid` /
`set wifi.pwd` after switching from mups17.

### Changed behaviour

Same as mups17 for PR #3536, PR #3260, PR #2834 and PR #1349 – see
[MUPS17.md](https://github.com/m0urs/MeshCore/blob/mups17-release/MUPS17.md#changed-behaviour).

---

## Changed default settings

Apply to **new installs / after erasing the settings** only – devices keep
their stored settings when flashed.

### Radio preset (all roles)

| Setting | MeshCore default | mups18 |
|---|---|---|
| Frequency | 915.0 MHz | **869.618 MHz** |
| Bandwidth | 250 kHz | **62.5 kHz** |
| Spreading factor | 10 | **8** |
| Coding rate | 5 | **8** |

### Duty cycle (Companion, Repeater, Room Server, Sensor)

| Setting | MeshCore default | mups18 |
|---|---|---|
| Airtime factor (`af`) | 1.0 (= 50 % duty cycle) | **9.0** (= **10 %** duty cycle, EU 869.4–869.65 MHz sub-band) |

On existing companions: `set dutycycle 10` in the companion CLI or rescue CLI.

### Repeater

Set in `examples/simple_repeater/MyMesh.cpp`, block `// ---- mups: own defaults`.

| Setting (CLI name) | MeshCore v1.18.0 default | mups18 |
|---|---|---|
| `rxdelay` | 0 | **2** |
| `txdelay` | 0.5 | **1.0** |
| `direct.txdelay` | 0.3 | **0.4** |
| `advert.interval` | 2 min | **240 min** |
| `flood.advert.interval` | 47 h | **167 h** |
| `flood.max` | 64 | **15** |
| `flood.max.unscoped` | 64 | **3** |
| `flood.max.advert` | 8 | **3** |
| `multi.acks` | 0 | **1** |
| `path.hash.mode` | 0 | **2** |
| `loop.detect` | minimal | **moderate** |
| `agc.reset.interval` | 0 (off) | **12 s** |
| `cad` | off | **on** |

---

## Release history

| Tag | Date | Base / changes |
|---|---|---|
| `mups18-1` | 2026-10-28 | PowerSaving-v18 (MeshCore v1.18.0) + PR #1349, #2834, #3260, #3536, #2586 + PR #2670 (partial) + DMC packet filter + airtime reserve + own defaults |
