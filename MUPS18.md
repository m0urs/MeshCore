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
   - [Diagnosing receive errors and channel access (Repeater)](#diagnosing-receive-errors-and-channel-access-repeater)
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
| DMC packet filter | [Dutch-MeshCore `dmc-dev`](https://github.com/Dutch-MeshCore/MeshCore/tree/dmc-dev) (`ecfb2d41`) | port branch `mu/dmc-filter-ps18` | **repeater only, off by default and in dry-run mode** (after `filter on` it only counts until `filter dryrun off`), own fixes and defaults as in mups17; filter files updated to the Dutch state `ecfb2d41` |
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
- **Busy-wait timer reset (new, `Dispatcher::checkSend()`):** the 4 s limit for a
  busy channel (`cad_busy_start`) is now cleared when no packet is due. PR #2670
  can remove a queued resend while it waits for a free channel (downstream
  forward overheard); the old start time then stayed, and the next packet that
  met a busy channel was sent immediately instead of waiting. In the official
  v1.18.0 a queued packet only leaves the queue by being sent, so the situation
  does not arise there.

### DMC packet filter

Updated to the Dutch-MeshCore state `ecfb2d41` (mups17: `34d1c16f`): the Dutch
refactoring (shared `MessageContent.h`, rule and path list editing) and their
fix that builds the rate limiters from the active prefs are included. Same own
changes as in mups17: channel block by key, limiters in sync (now via the Dutch
`initLimiters()`), own defaults in `examples/simple_repeater/FilterDefaultsMups.h`. See
[MUPS17.md](https://github.com/m0urs/MeshCore/blob/mups17-release/MUPS17.md#dmc-packet-filter)
and [`docs/packet_filter_reference.md`](docs/packet_filter_reference.md).

**Two defaults protect a new install:** the filter is switched **off**, and **dry-run is on**.
After `filter on` the repeater therefore only *counts* what it would drop (`filter stats` marks
this with `(dry-run)`) and still forwards everything. Look at the statistics first, then switch
to real filtering with `filter dryrun off`. `filter reset` restores these defaults, including
dry-run on. A repeater that already has a stored `/filter_prefs` keeps its current settings.

Not taken over from `dmc-dev`: `dc.gate` / true TX duty window, the region fix for
transport code 0, the companion receive filter, companion SNMP / OTA – they are
outside the repeater packet filter.

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
| DMC packet filter (repeater) | off, **dry-run on** | `filter on` (it then only counts), `filter dryrun off` (it then really drops) |
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
| `get cad` | all roles | PowerSaving-v18 | Shows `on`/`off` and the channel-busy counter (`channel busy` = CAD scans that detected activity, so one waiting packet can add several). **Repeater, mups:** the reply also shows `blocked <n>` (send attempts that found the channel busy and had to wait = busy episodes) and `forced <n>` (transmissions started anyway after the 4 s busy limit). Example: `> on; channel busy: 812, blocked 57, forced 2`. Counted since boot / `clear stats`. |
| `get rx.errors` / `get rx.errors snr\|rssi\|len\|last` | Repeater | own | Receive errors by cause (`ok`, `crc` split into `pay`/`hdr`/`unk`, `other`, `err share`), histograms by SNR, RSSI and frame length, and the latest damaged frames; usable remotely. Example: `> ok 2745, crc 377 (pay 372, hdr 5, unk 0), other 0 (last 0), err share 12%`. Counted since boot / `clear stats`. Explanation of every option and how to read the results: [Diagnosing receive errors](#diagnosing-receive-errors-and-channel-access-repeater). |
| `get outpath [path]` / `set outpath <hop1,hop2,…\|path\|direct\|clear\|flood>` | Repeater (remote admin only) | PR #2586 | Override the direct reply path to the logged-in client; `get/set outpath path` uses the path observed after the client's latest flood login |
| `filter …` | Repeater | DMC packet filter | see `docs/packet_filter_reference.md` |
| `get max.resend` / `set max.resend <0-3>` | all roles | PR #2670 | Max. resend attempts for direct packets, `0` = off, default `3`. On the repeater `get` also shows the resend ratio. |
| `set fwd.scoped.reserve <0-100>` / `get fwd.scoped.reserve` / `get fwd.scoped.stats` | Repeater | own (idea: ACETyr) | Airtime reserve for scoped floods |

### Diagnosing receive errors and channel access (Repeater)

These commands exist only in the repeater firmware and also work remotely
(from the app's command line); the `stats-*` commands are serial only. They
help to find out **why** a repeater shows many "Received Packet Errors" in the
app. They do not change any radio behaviour.

#### What is counted

Every time the radio chip reports "a frame was received" the driver reads it.
Frames that arrive correctly are counted as `ok` (this is the "received
packets" figure of the statistics, duplicates included). Frames for which the
read failed are errors:

| Counter | Meaning |
|---|---|
| `ok` | Frames received correctly |
| `crc` | Frames the driver reported as CRC mismatch. Split into `pay`, `hdr`, `unk` (see below) |
| `pay` | **Payload CRC error**: the LoRa header was valid, the data part was damaged. Typical for weak signals, collisions with other transmitters and interference during the frame |
| `hdr` | **Header error**: the chip could not decode a valid LoRa header. Rare (a few per thousand) because the chip usually does not report such frames at all |
| `unk` | The chip could not say whether it was payload or header (chips without interrupt flags). Stays `0` on SX126x |
| `other` | Other read errors (for example `-705` = SPI timeout); `(last <code>)` shows the RadioLib code of the latest one, `0` = none. A value above 0 points to a hardware or wiring problem and not to radio interference |
| `err share` | `(crc + other) * 100 / (ok + crc + other)`, rounded **down** to whole percent (11.9 % is shown as 11 %) |

All counters start at 0 at boot and are reset by `clear stats`. Not reset by
`clear stats`: the airtime counters (TX/RX) on the statistics page; they run
since boot. Frames of length 0 and frames the chip never reported (for example
a preamble without a valid header) are not counted anywhere.

#### Command reference

| Command | Output | How to use it |
|---|---|---|
| `get rx.errors` | `> ok 2745, crc 377 (pay 372, hdr 5, unk 0), other 0 (last 0), err share 12%` | Overall picture. One look tells whether errors are a rare effect or the majority |
| `get rx.errors snr` | `> snr ok 528,739,724,480,275 pay 163,78,90,41,0 hdr 3,1,0,0,1 [<=-10,<=-5,<=0,<=5,>5 dB]` | Frames sorted by signal-to-noise ratio of the frame. Five buckets; the limits are printed at the end of the reply |
| `get rx.errors rssi` | `> rssi ok … pay … hdr … [<=-120,<=-110,<=-100,<=-90,>-90 dBm]` | Same, sorted by signal strength (packet RSSI, averaged over the frame) |
| `get rx.errors len` | `> len ok … pay … hdr … [<=30,<=60,<=100,<=160,>160 B]` | Same, sorted by the frame length in bytes (including header and path) |
| `get rx.errors last` | `> last 61s/-121/-7.5/49/011B567A 81s/-108/5.0/121/11073223 …` | The latest damaged frames, newest first (up to 6 are kept, as many as fit into the 150-character reply are shown). Each entry: `<age in s>/<RSSI dBm>/<SNR dB>/<length B>/<first 4 bytes in hex>`. Prefix `h` = header error, `u` = unknown, no prefix = payload error. `> last: none since boot / clear stats` if there is nothing yet |
| `get cad` | `> on; channel busy: 137, blocked 134, forced 1` | Channel access, see the end of this section |
| `clear stats` | `(OK - stats reset)` | Starts a new measurement: resets all counters above and the channel counters. Note the time |

Each histogram line has three groups of five numbers (buckets 1 to 5, in the
order of the limits printed at the end): `ok` = good frames, `pay` = payload
errors, `hdr` = header errors.

#### How to read the histograms

For every bucket calculate the **error rate = pay / (ok + pay)**. A single bucket with
few frames means little; look at buckets with several hundred frames.

| What you see | What it usually means |
|---|---|
| High error rate only in the weakest SNR bucket (`<= -10 dB`, around the SF8 demodulation limit of about -10 dB), falling steadily to almost 0 at `> 5 dB` | Normal weak-signal errors on a clean channel |
| High error rate (clearly above 40 %) even in good buckets (SNR `> -5 dB`, RSSI `> -110 dBm`) | The frame was strong enough but still damaged: interference or collisions, not just weak signals |
| Error rate rises strongly with the frame length (for example 10 % at `<= 60 B` but 90 % or more at `> 160 B`) | The damage happens somewhere **during** the frame (longer frames have more chances to be hit): bursts of interference or colliding transmitters |
| One RSSI bucket with many errors and almost no good frames | A foreign signal at that level; compare the same bucket at another place or time |
| `hdr` consistently 0 | Normal: the chip rarely reports header errors |

These are hints and not proofs: the same pattern can have more than one cause.
SNR and RSSI of a damaged frame are the values the chip stored for that frame;
on an SX1262 they behave plausibly (on a clean channel the error rate falls
with the SNR), but this was not verified against a reference measurement.

#### How to read `get rx.errors last`

The 4 bytes are the (possibly damaged) **start of the frame** and show what the
sender looks like.

- **Byte 1 = header.** Bits 0-1 = route: `0` transport flood, `1` flood, `2` direct,
  `3` transport direct. Bits 2-5 = payload type: `0` REQ, `1` RESPONSE, `2` TXT_MSG,
  `3` ACK, `4` ADVERT, `5` GRP_TXT, `6` GRP_DATA, `7` ANON_REQ, `8` PATH, `9` TRACE,
  `A` MULTIPART, `B` CONTROL, `F` RAW_CUSTOM. Bits 6-7 = version: only `0` is valid.
  Examples: `01` = flood REQ, `11` = flood ADVERT, `21` = flood PATH, `14` = transport-flood GRP_TXT.
  A first byte such as `41`, `54` or `C5` (version bits not 0) shows that the start of this frame itself is damaged.
- **Transport routes** (`0` and `3`) have 4 transport-code bytes after byte 1; the path byte follows after them.
- **Path byte** (byte 2 for plain flood/direct routes): bits 6-7 = hash size minus 1, bits 0-5 = number of hops. `86` = 3-byte hashes, 6 hops (18 path bytes); `4A` = 2-byte hashes, 10 hops.
  Expected frame length = 1 (+ 4 for transport routes) + 1 + hops x hash size + payload.
- **Path bytes** (after the path byte) start with the hash of the first relay. The same start in several entries means the same packet was heard several times, usually from different relays.
- **Reading the values:** an entry with a valid first byte and weak signal (RSSI around -120 dBm, SNR at or below -10 dB) is an ordinary weak-signal error. An entry that is strong (SNR above 0 dB) and still damaged hints at interference or a collision.

#### Channel access (`get cad`)

| Field | Meaning |
|---|---|
| `on` / `off` | Hardware CAD (channel activity detection) before sending |
| `channel busy` | CAD scans that detected activity (one waiting packet can add several) |
| `blocked` | Send attempts that found the channel busy (a packet in reception, a CAD hit or the RSSI gate) and had to wait = busy episodes |
| `forced` | Transmissions started anyway after the 4 s busy limit. A few per day were seen on a repeater with a clean channel; many forced sends mean the channel was often busy for 4 s or longer |

Counted since boot or `clear stats`.

#### Recommended measurement

1. `clear stats`; note the time on **one** clock (for example your phone). The repeater clock (`clock`) shows UTC.
2. Wait until at least 1000 good frames have arrived (about one to two hours on a busy repeater).
3. Run `get rx.errors`, `get rx.errors snr`, `get rx.errors rssi`, `get rx.errors len`, `get rx.errors last`, `get cad`, and note the time again.
4. Per hour = counter / hours. Compare periods only with the same settings (power saving, rxgain, CAD, antenna and place unchanged); repeat at another time of day, because errors follow the traffic.

#### Example (one RAK4631 repeater, EU 62.5 kHz SF8)

| | disturbed period | clean period |
|---|---|---|
| `err share` | 58 % (ok 3087, crc 4367) | 11 % (ok 11959, crc 1588) |
| error rate by length, `<= 30 B` … `> 160 B` | 30 %, 47 %, 73 %, 96 %, 98 % | 9 %, 10 %, 12 %, 16 %, 20 % |
| error rate by SNR, `<= -10 dB` … `> 5 dB` | 81 %, 56 %, 53 %, 48 %, 32 % | 25 %, 10 %, 13 %, 7 %, 2 % |
| good / damaged frames per hour | about 330 / 470 | about 800 / 110 |

In the disturbed period frames with good SNR were still damaged and long frames
almost never survived; in the clean period the errors are concentrated in the
weakest SNR bucket. The cause of the disturbed period was not identified.

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

**CLI: trailing blanks are ignored (Repeater, Room Server, Sensor).** Blanks, tabs and line ends at the end of a
command are cut off before the command is evaluated, so `get txdelay ` works like `get txdelay` (before, many commands
were rejected as unknown because they are matched against the whole text). Exception: `password …` and
`set guest.password …` are left exactly as typed. Companions are not changed. The code is in `src/helpers/CliTrim.h`.

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

### Companion: CAD

| Setting | MeshCore default | mups18 |
|---|---|---|
| `cad` (hardware channel activity detection before TX) | off | **on** (same as the mups repeater default) |

Applies when the stored settings contain no `cad` value (new install, or settings
written by mups17); a companion that already saved its settings with mups18 keeps
its stored value. Switch with `set cad on|off` in the companion CLI or rescue CLI.

### Repeater

Set in `examples/simple_repeater/MyMesh.cpp`, block `// ---- mups: own defaults`.

| Setting (CLI name) | MeshCore v1.18.0 default | mups18 |
|---|---|---|
| `rxdelay` | 0 | **2** |
| `txdelay` | 0.5 | **1.0** |
| `direct.txdelay` | 0.3 | **0.4** |
| `advert.interval` | 2 min | **238 min** |
| `flood.advert.interval` | 47 h | **167 h** |
| `flood.max` | 64 | **15** |
| `flood.max.unscoped` | 64 | **3** |
| `flood.max.advert` | 8 | **3** |
| `multi.acks` | 0 | **1** |
| `path.hash.mode` | 0 | **2** |
| `loop.detect` | minimal | **moderate** |
| `cad` | off | **on** |

---

## Release history

| Tag | Date | Base / changes |
|---|---|---|
| `mups18-1` | 2026-10-28 | PowerSaving-v18 (MeshCore v1.18.0) + PR #1349, #2834, #3260, #3536, #2586 + PR #2670 (partial) + DMC packet filter + airtime reserve + own defaults |
