# CLI Commands

This document provides an overview of CLI commands which are specific to the MU firmware version.

## Navigation

- [Enable or disable hardware Channel Activity Detection (CAD)](#enable-or-disable-hardware-channel-activity-detection)
- [View or change the maximum direct-route resend attempts](#view-or-change-the-maximum-direct-route-resend-attempts)
- [View or set the reply path override for the current remote client](#view-or-set-the-reply-path-override-for-the-current-remote-client)
- [Wifi Companion Configuration via Rescue CLI](#wifi-companion-configuration-via-rescue-cli)
  
---
### Enable or disable hardware Channel Activity Detection
**Usage:**
- `get cad`
- `set cad <on|off>`

**Description:** When enabled, the radio performs a hardware Channel Activity Detection scan before transmitting and defers if the channel is busy. Runs independently of `int.thresh` — either, both, or none may be active.

**Parameters:**
- `on|off`: Enable or disable hardware CAD

**Default:** `off`

---
### View or change the maximum direct-route resend attempts

**Usage:**
- `get max.resend`
- `set max.resend <value>`

**Parameters:**
- `value`: Maximum number of resend attempts for direct-routed packets (0–3). `0` disables resending entirely.

**Default:** `2`

---

### View or set the reply path override for the current remote client
**Usage:**
- `get outpath`
- `set outpath <hop1_hex,hop2_hex,...>`
- `set outpath direct`
- `set outpath clear`
- `set outpath flood`

**Parameters:**
- `hopN_hex`: Hop hash, `2`, `4`, or `6` hex characters. All hops must use the same width.

**Notes:**
- These commands require remote client context (they target the caller's ACL entry).
- The path hash size is inferred from the hop hash width.
- A configured hop list replaces the stored direct reply route used for that caller.
- `direct` sets a zero-hop direct route for a caller reachable without repeaters.
- `clear` forgets the current direct path and allows normal path discovery to repopulate it.
- `flood` forces replies to use flood packets until the client logs in again.

---
### Wifi Companion Configuration via Rescue CLI

**Description:** Configure Wifi SSID and Password for a ESP32 companion via the rescue command line interface

**Usage:**
- `wifi_ssid <ssid>`
- `wifi_pwd <pwd>`
- `wifi_commit`
- `wifi_clear`
