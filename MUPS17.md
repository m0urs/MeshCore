# mups17 – MU PowerSaving firmware (based on PowerSaving-v17)

Custom MeshCore firmware maintained by Michael Urspringer.
Branch `mups17` is **generated** by `build-mups17.sh` and force-pushed on every
rebuild – never commit to it directly. Releases are git tags `mups17-<n>`;
the firmware reports them as `v1.17.1.mups<n>` (test builds: `v1.17.1.dev-<hash>`).

## Contents

1. [What is included](#what-is-included)
2. [Additional and changed CLI commands](#additional-and-changed-cli-commands-compared-to-meshcore-main)
3. [Changed default settings](#changed-default-settings)
4. [Release history](#release-history)

---

## What is included

| Layer | Source | How it gets in | Notes |
|---|---|---|---|
| Base | [IoTThinks/MeshCore `PowerSaving-v17`](https://github.com/IoTThinks/MeshCore/tree/PowerSaving-v17) | branch base | RX duty-cycle power saving, based on MeshCore `main` (v1.17.1) |
| Release fixes | [meshcore-dev/MeshCore `main`](https://github.com/meshcore-dev/MeshCore/tree/main) | merge | keeps the base up to date with official releases |
| PR #1349 | [Allow setting RTC clock backwards, fix elapsed-time underflow](https://github.com/meshcore-dev/MeshCore/pull/1349) | squashed cherry-pick | conflict in `ArduinoHelpers.h` resolved automatically (rerere) |
| PR #2670 | [Repeated sending of direct packets](https://github.com/meshcore-dev/MeshCore/pull/2670) | port branch `mu/pr-2670-ps17` | **partial port**, see below |
| PR #1896 | [Fix 1970 date after crash/watchdog/brownout (ESP32)](https://github.com/meshcore-dev/MeshCore/pull/1896) | already in PowerSaving-v17 | – |
| PR #2704 | [Time keeping for nRF52 across resets](https://github.com/meshcore-dev/MeshCore/pull/2704) | already in PowerSaving-v17 | – |
| Own changes | branch `mu/mups17-own` | merge | default settings, this document |

### PR #2670 – partial port

Taken over: resend of direct packets (Dispatcher/Mesh/Packet), `max.resend`
setting and statistics, non-invasive channel check before a resend.

Left out on purpose, because they conflict with RX power saving:

- noise-floor median estimator (needs continuous RX sampling every 50 ms)
- RX-desync watchdog (the radio is legitimately not in RX mode while duty-cycling)

The RSSI part of the resend channel check is skipped while the radio sleeps
between RX windows (same guard as the PowerSaving code), so it never wakes the
radio and never breaks the duty cycle.

---

## Additional and changed CLI commands (compared to MeshCore `main`)

Full syntax and details: [`docs/cli_commands.md`](docs/cli_commands.md).

### New commands

| Command | Applies to | Source | Description |
|---|---|---|---|
| `get radio.rxps` / `set radio.rxps <off\|on\|conservative\|balanced\|max\|overdrive\|riskyWorkingMax\|1-10\|level …\|<rx_us> <sleep_us>>` | Repeater, Room Server | PowerSaving-v17 | RX duty-cycle power saving. Companions always use the fixed `balanced` profile. |
| `get reboot.interval` / `set reboot.interval <hours>` | Repeater, Room Server | PowerSaving-v17 | Periodic reboot, `0` = off (default) |
| `powerlog` | Repeater, Room Server, Sensor | PowerSaving-v17 | Last reset reason; on nRF52 also last shutdown reason and boot voltage |
| `sensor` | Repeater, Room Server, Sensor | PowerSaving-v17 | Shows the I2C and GPS pins of the board and whether GPS is configured |
| `get outpath` / `set outpath <hops\|direct\|clear\|flood>` | Repeater (remote admin only) | PowerSaving-v17 | Override the direct return path to the logged-in client |
| `get max.resend` / `set max.resend <0-3>` | Repeater, Room Server (Companion: via app) | PR #2670 | Max. resend attempts for direct packets, `0` = off, default `2`. `get` also shows the resend ratio. |

### Changed behaviour

| Command | Source | Change |
|---|---|---|
| `clock sync`, `time <epoch>`, setting the time from the companion app | PR #1349 | The clock may now be set **backwards** (previously rejected with "clock cannot go backwards"). Elapsed-time displays (e.g. `neighbors`) no longer jump to huge values afterwards. |

---

## Changed default settings

Apply to **new installs / after erasing the settings** only – devices keep
their stored settings when flashed.

### Radio preset (all roles)

| Setting | MeshCore default | mups17 |
|---|---|---|
| Frequency | 915.0 MHz | **869.618 MHz** |
| Bandwidth | 250 kHz | **62.5 kHz** |
| Spreading factor | 10 | **8** |
| Coding rate | 5 | **8** |

### Repeater

Set in `examples/simple_repeater/MyMesh.cpp`, block `// ---- mups: own defaults`.

| Setting (CLI name) | MeshCore default | mups17 |
|---|---|---|
| `af` (airtime factor) | 1.0 | **9.0** (= 10 % duty cycle, EU) |
| `rxdelay` | 0 | **2** |
| `txdelay` | 0.5 | **1.0** |
| `direct.txdelay` | 0.3 | **0.4** |
| `advert.interval` | 2 min | **240 min** |
| `flood.advert.interval` | 47 h | **167 h** |
| `flood.max` | 64 | **18** |
| `flood.max.unscoped` | 64 | **4** |
| `flood.max.advert` | 8 | **4** |
| `multi.acks` | 0 | **1** |
| `path.hash.mode` | 0 | **2** |
| `loop.detect` | off | **minimal** |
| `agc.reset.interval` | 0 | 0 |

---

## Release history

| Tag | Date | Base / changes |
|---|---|---|
| `mups17-1` | _tbd_ | PowerSaving-v17 + main + PR #1349 + PR #2670 (partial) + own defaults |
