# mups17 – MU PowerSaving firmware (based on PowerSaving-v17)

Custom MeshCore firmware maintained by Michael Urspringer.
Branch `mups17` is **generated** by `build-mups17.sh` and force-pushed on every
rebuild – never commit to it directly. Releases are git tags `mups17-<n>`;
the firmware reports them as `v1.17.1.mups<n>` (test builds: `v1.17.1.dev-<hash>`).

## Contents

1. [What is included](#what-is-included)
   - [Core features and optional features](#core-features-and-optional-features)
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
| PR #2834 | [Separate replay counters for login and messages](https://github.com/meshcore-dev/MeshCore/pull/2834) | squashed cherry-pick | repeater / room server / sensor: login and CLI/DM timestamps no longer block each other when the companion clock and the app clock differ |
| PR #3260 | [Send flood ACK on DM retries](https://github.com/meshcore-dev/MeshCore/pull/3260) | squashed cherry-pick | conflict with the PowerSaving `outpath` code in `simple_repeater/MyMesh.cpp` resolved once, then automatic (rerere) |
| PR #3536 | [Beeps for silence and unsilence](https://github.com/meshcore-dev/MeshCore/pull/3536) | squashed cherry-pick | companion UI only; T1000-E button debounce 20 ms |
| PR #2670 | [Repeated sending of direct packets](https://github.com/meshcore-dev/MeshCore/pull/2670) | port branch `mu/pr-2670-ps17` | **partial port**, see below |
| DMC packet filter | [Dutch-MeshCore `dmc-dev`](https://github.com/Dutch-MeshCore/MeshCore/tree/dmc-dev) (`34d1c16f`) | port branch `mu/dmc-filter-ps17` | **repeater only**, filter files taken unchanged + own fix (channel block by key) + 6 hooks in `simple_repeater/MyMesh.*`; region gating / duty-cycle limits of DMC **not** included; **off by default** |
| Airtime reserve for scoped floods | idea and design from [ACETyr/MeshCore](https://github.com/ACETyr/MeshCore) (`fwd.scoped.reserve`) | port branch `mu/scoped-reserve-ps17` | **repeater only, off by default**; own implementation, own config file `/scoped_reserve` |
| PR #2706 | [WiFi companion robustness](https://github.com/meshcore-dev/MeshCore/pull/2706) | adapted in `mu/mups17-own` | **only** the credential loading from `/wifi_config` at boot; the reconnect rewrite of the PR is **not** taken (conflicts with the interface manager in `main`) |
| PR #2720 | [WiFi companion configuration via rescue CLI](https://github.com/meshcore-dev/MeshCore/pull/2720) | adapted in `mu/mups17-own` | `wifi_ssid`, `wifi_pwd`, `wifi_commit`, `wifi_show`, `wifi_clear`; replies with CRLF |
| PR #1896 | [Fix 1970 date after crash/watchdog/brownout (ESP32)](https://github.com/meshcore-dev/MeshCore/pull/1896) | already in PowerSaving-v17 | – |
| PR #2704 | [Time keeping for nRF52 across resets](https://github.com/meshcore-dev/MeshCore/pull/2704) | already in PowerSaving-v17 | – |
| Own changes | branch `mu/mups17-own` | merge | default settings (incl. 10 % duty cycle), companion rescue CLI extensions and fixes, WiFi credentials via rescue CLI (adapted from PR #2706 + #2720), this document |

### PR #2670 – partial port

Taken over: resend of direct packets (Dispatcher/Mesh/Packet), `max.resend`
setting and statistics, non-invasive channel check before a resend.

**On by default in mups17** with the maximum of 3 attempts (`max.resend 3`;
the PR uses `2`). It is a core feature of this firmware, like RX power saving.
Switch off with `set max.resend 0`.

Left out on purpose, because they conflict with RX power saving:

- noise-floor median estimator (needs continuous RX sampling every 50 ms)
- RX-desync watchdog (the radio is legitimately not in RX mode while duty-cycling)

The RSSI part of the resend channel check is skipped while the radio sleeps
between RX windows (same guard as the PowerSaving code), so it never wakes the
radio and never breaks the duty cycle.

### DMC packet filter

Repeater packet filter from Dutch-MeshCore (`dmc-dev`, commit `34d1c16f`),
developed in their branch `enhancement/dmc-dev-filtering`.

- **Disabled by default** – nothing changes until `filter on`.
- Own config file `/filter_prefs` on the repeater (independent of the normal
  prefs). Removing the filter later leaves only this file behind.
- Runs only in `allowPacketForward()`, i.e. on packets the repeater would
  otherwise forward; it never touches own packets or PR #2670 resends.
- RAM: about 5 KB.
- **Own fix – channel block by key:** a group packet carries its channel only
  as a one-byte hash, which many channels share. Upstream `filter channel`
  drops every channel on that byte. In mups17 a hash match must also verify
  the packet's MAC under the channel key, so only the real channel is dropped
  (the repeater still cannot read the messages). The malformed scan no longer
  drops other channels that share the Public hash byte `0x11`.
- Recommended start: `filter dryrun on` (count what would be dropped, but
  still forward), check `filter stats`, then `filter dryrun off`.
- Full reference: [`docs/packet_filter_reference.md`](docs/packet_filter_reference.md).
- Updates: `build-mups17.sh` warns when the filter files change in
  `dutch/dmc-dev`; then copy the new files into `mu/dmc-filter-ps17` and
  re-apply the own fix (see `MUPS17-HOWTO.md`, section 8).

### Airtime reserve for scoped floods

Repeater only, **off by default** (`fwd.scoped.reserve 0`). Idea and design
from ACETyr/MeshCore (forward-filter Stage 4), implemented separately here.

- Keeps a percentage of this repeater's **own TX allowance per 60 s** free for
  **scoped** floods (packets with a region / transport code).
- Allowance per window = 60 s × duty cycle, e.g. 6000 ms at 10 %.
  With `set fwd.scoped.reserve 40`, 2400 ms of it stay reserved.
- An **unscoped** flood is dropped only if forwarding it would eat into the
  reserve. Quiet channel: everything is forwarded. Busy repeater: scoped
  traffic gets priority.
- Scoped floods and all direct traffic always pass. Repeaters never change a
  packet's scope when forwarding, so an unscoped flood stays unscoped on every
  hop.
- Complements `flood.max.unscoped` (fixed hop limit) with a load-dependent limit.
- Reserves only this node's TX time, not the radio channel.
- Runs after the DMC filter. While `0`, the check returns before doing
  anything (no counters, no airtime estimate).
- Own config file `/scoped_reserve`; the normal prefs are untouched.
- Counters (`get fwd.scoped.stats`) are in RAM and reset on reboot and with
  `clear stats`.

Worth it only if the repeater actually gets near its duty-cycle limit:
compare two status requests (TX airtime / uptime) taken some minutes apart
at a busy time, and watch the TX queue length.

### Core features and optional features

**Core features – on by default.** They are what mups17 is about:

| Feature | Default | Switch off |
|---|---|---|
| RX power saving (PowerSaving-v17; repeater / room server / sensor, companions always) | on | `set radio.rxps off` (not on companions) |
| Repeated sending of direct packets (PR #2670, all roles) | on, `max.resend 3` | `set max.resend 0` (companion: rescue CLI) |

**Optional features – off by default.** A feature that changes how packets
are forwarded beyond that must be switched on by the user. While off, the
firmware behaves as without the feature.

| Feature | Default | Switch on |
|---|---|---|
| DMC packet filter (repeater) | off | `filter on` (start with `filter dryrun on`) |
| Airtime reserve for scoped floods (repeater) | off | `set fwd.scoped.reserve <1-100>` |
| WiFi credentials from flash (WiFi companion) | compiled-in values | rescue CLI `wifi_ssid` / `wifi_pwd` / `wifi_commit` |

Always active (bug fixes / small behaviour changes, not switchable):
PR #1349, PR #2834, PR #3260, PR #3536 – see [Changed behaviour](#changed-behaviour).
The internal parts of PR #2670 (separate ACK de-duplication table, packet
hash cache) are always compiled in; with `max.resend 0` no resend is ever
scheduled.

The own default settings (radio preset, duty cycle, repeater values) are
deliberate presets, not features – see [Changed default settings](#changed-default-settings).

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
| `filter …` (`on`/`off`, `dryrun`, `types`, `advert`, `path`, `sender`, `text`, `age`, `stats`, `reset`, `help`) | Repeater | DMC packet filter | Configurable forwarding filter per packet type, per-origin advert window, path-prefix block, group-text sender/text rules and age limit, statistics incl. saved airtime. See `docs/packet_filter_reference.md`. |
| `get max.resend` / `set max.resend <0-3>` | Repeater, Room Server (Companion: rescue CLI) | PR #2670 | Max. resend attempts for direct packets, `0` = off, default `3`. `get` also shows the resend ratio. |
| `set fwd.scoped.reserve <0-100>` / `get fwd.scoped.reserve` | Repeater | own (idea: ACETyr) | Airtime reserve for scoped floods, % of the TX allowance per 60 s; `0` = off (default) |
| `get fwd.scoped.stats` | Repeater | own | `reserve`, forwarded scoped / unscoped floods, dropped unscoped floods, saved airtime, TX airtime used / allowance in the current 60 s window |

### Companion rescue CLI (new commands)

Rescue mode: hold the user button for a long press within 8 s after boot,
then use a serial terminal (115200 baud) or the console on
<https://flasher.meshcore.io>. Original commands: `set pin`, `rebuild`,
`erase`, `ls`, `cat`, `rm`, `reboot`.

| Command | Description |
|---|---|
| `get pin` | Show the BLE PIN |
| `get af` / `set af <0-9>` | Airtime factor |
| `get dutycycle` / `set dutycycle <10-100>` | Duty cycle in percent (sets `af = 100/dc - 1`); EU: `set dutycycle 10` |
| `get max.resend` / `set max.resend <0-3>` | Resend attempts for direct packets (PR #2670), `0` = off |
| `wifi_ssid <ssid>`, `wifi_pwd <pwd>`, `wifi_commit` | **WiFi companions only:** stage SSID and password, `wifi_commit` saves them to `/wifi_config` and reboots (PR #2720, adapted) |
| `wifi_show` / `wifi_clear` | Show staged values and whether `/wifi_config` exists / delete it (back to compiled-in defaults) and reboot |

Changes are saved immediately; `reboot` to leave rescue mode.

**WiFi companion (e.g. `Heltec_v3_companion_radio_wifi`):** on boot the
firmware reads `/wifi_config` (written by `wifi_commit`); without it, the
compiled-in `WIFI_SSID` / `WIFI_PWD` from `platformio.ini` are used
(PR #2706, credential loading only – the reconnect logic of PowerSaving-v17 /
`main` is kept). One firmware file can therefore be shared without anybody's
WiFi password in it. Note: the rescue CLI echoes what you type, so the
password is visible in the terminal while entering it.

Rescue mode on devices with a single button (e.g. **T1000-E**): power on,
release, then immediately press and hold the button for ~3 s (within the
first 8 s). A long press later than 8 s after boot powers the device off.

**Fixes compared to the original rescue CLI** (affect all commands, incl.
`set pin`, `ls`, `cat`):

- All replies end with `\r\n`. The web console on flasher.meshcore.io only
  displays a line once it receives `\r\n`; with `\n` only, each reply
  appeared one command late (or only after an extra Enter).
- `\r`, `\n` and `\r\n` are all accepted as line end; empty lines are
  ignored (no more `Error: unknown command` after an extra Enter).
- Replies are flushed immediately.

### Changed behaviour

| Command | Source | Change |
|---|---|---|
| Companion: toggling the buzzer (button / UI) | PR #3536 | Buzzer **on** plays a rising tone, buzzer **off** plays a falling tone and then mutes (previously: on = ACK tone, off = no sound). On the T1000-E the button debounce time is 20 ms instead of 50 ms (more reliable multi-press detection). |
| ACKs for DMs and legacy CLI messages (companion, repeater, room server as receiver) | PR #3260 | When the sender **retries** a message, the ACK is sent as **flood** instead of over the stored direct path. Complements PR #2670 (per-hop resends cannot fix a broken path; a flood ACK can). Costs a little extra airtime on retries only; the sender does not learn a new path from it. |
| Login to repeater / room server / sensor after CLI commands (and vice versa) | PR #2834 | Separate replay counters: a login (always stamped with the companion clock) and CLI commands/DMs (stamped with the app clock unless "Use companion clock …" is enabled) no longer reject each other as "replay" when the two clocks differ. |
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

### Duty cycle (all roles: Companion, Repeater, Room Server, Sensor)

| Setting | MeshCore default | mups17 |
|---|---|---|
| Airtime factor (`af`) | 1.0 (= 50 % duty cycle) | **9.0** (= **10 %** duty cycle, EU 869.4–869.65 MHz sub-band) |

`set dutycycle 10` on a repeater is the same setting: it just writes
`af = 100/10 - 1 = 9.0`.

The MeshCore app has no setting for it. On existing companions use the
**rescue CLI** (see below: `set dutycycle 10`), or
[meshcore-cli](https://pypi.org/project/meshcore-cli/) (companion protocol
command `SET_TUNING_PARAMS`, values x1000: rx delay, airtime factor):

    meshcore-cli <connection options> set tuning 0,9000

### Repeater

Set in `examples/simple_repeater/MyMesh.cpp`, block `// ---- mups: own defaults`.

| Setting (CLI name) | MeshCore default | mups17 |
|---|---|---|
| `rxdelay` | 0 | **2** |
| `txdelay` | 0.5 | **1.0** |
| `direct.txdelay` | 0.3 | **0.4** |
| `advert.interval` | 2 min | **240 min** |
| `flood.advert.interval` | 47 h | **167 h** |
| `flood.max` | 64 | **10** |
| `flood.max.unscoped` | 64 | **3** |
| `flood.max.advert` | 8 | **3** |
| `multi.acks` | 0 | **1** |
| `path.hash.mode` | 0 | **2** |
| `loop.detect` | off | **moderate** |
| `agc.reset.interval` | 0 (off) | **12 s** |

---

## Release history

| Tag | Date | Base / changes |
|---|---|---|
| `mups17-1` | 2026-10-08 | PowerSaving-v17 + main + PR #1349 + PR #2834 + PR #3260 + PR #3536 + PR #2670 (partial) + DMC packet filter + own defaults (10 % duty cycle all roles) + companion rescue CLI (`af`, `dutycycle`, `max.resend`, `wifi_*`, CRLF fix) + WiFi credentials via rescue CLI (PR #2706 / #2720 adapted) |
