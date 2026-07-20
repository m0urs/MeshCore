# CLI Commands

This document provides an overview of CLI commands which are specific to the MU firmware version.

## Navigation

- [Enable or disable hardware Channel Activity Detection (CAD)](#enable-or-disable-hardware-channel-activity-detection)
- [View or change the maximum direct-route resend attempts](#view-or-change-the-maximum-direct-route-resend-attempts)
  
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
