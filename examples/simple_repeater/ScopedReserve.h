#pragma once

#include <Mesh.h>
#include <helpers/IdentityStore.h>   // FILESYSTEM
#include "ScopedReserveGate.h"

// mups: airtime reserve for scoped (region) floods on the repeater.
//
//   set fwd.scoped.reserve <0-100>   % of this node's TX allowance per 60 s kept free for scoped floods
//   get fwd.scoped.reserve
//   get fwd.scoped.stats
//
// OFF by default (0). While off, allow() returns true before touching anything:
// the repeater forwards exactly as without this feature.
// Own config file, so the normal prefs layout is not touched.

#define SCOPED_RESERVE_FILE  "/scoped_reserve"

class ScopedReserve {
  uint8_t _pct = 0;
  ScopedReserveWindow _win;
  uint32_t _fwd_scoped = 0, _fwd_unscoped = 0, _drop_unscoped = 0, _saved_air = 0;

  void save(FILESYSTEM* fs) {
    if (fs == nullptr) return;
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
    fs->remove(SCOPED_RESERVE_FILE);
    File f = fs->open(SCOPED_RESERVE_FILE, FILE_O_WRITE);
#elif defined(RP2040_PLATFORM)
    File f = fs->open(SCOPED_RESERVE_FILE, "w");
#else
    File f = fs->open(SCOPED_RESERVE_FILE, "w", true);
#endif
    if (!f) return;
    f.write(&_pct, 1);
    f.close();
  }

public:
  uint8_t getPct() const { return _pct; }

  void load(FILESYSTEM* fs) {
    _pct = 0;
    if (fs == nullptr || !fs->exists(SCOPED_RESERVE_FILE)) return;
#if defined(RP2040_PLATFORM)
    File f = fs->open(SCOPED_RESERVE_FILE, "r");
#else
    File f = fs->open(SCOPED_RESERVE_FILE);
#endif
    if (!f) return;
    uint8_t v = 0;
    if (f.read(&v, 1) == 1 && v <= 100) _pct = v;
    f.close();
  }

  void resetStats() { _fwd_scoped = _fwd_unscoped = _drop_unscoped = _saved_air = 0; }

  // Called for FLOOD packets only. 'unscoped' = wildcard region (no transport code).
  bool allow(const mesh::Packet* packet, bool unscoped, unsigned long now, unsigned long total_air,
             float airtime_factor, mesh::Radio* radio) {
    if (_pct == 0) return true;   // off: no-op
    if (!unscoped) { _fwd_scoped++; return true; }
    uint32_t est = radio->getEstAirtimeFor(packet->getRawLength());
    if (scopedReserveDrop(scopedReserveWindowMax(airtime_factor), _win.used(now, total_air), _pct, est)) {
      _drop_unscoped++;
      _saved_air += est;
      return false;
    }
    _fwd_unscoped++;
    return true;
  }

  // true if the command was handled
  bool handleCommand(FILESYSTEM* fs, const char* command, char* reply, unsigned long now,
                     unsigned long total_air, float airtime_factor) {
    if (strncmp(command, "set fwd.scoped.reserve ", 23) == 0) {
      int v = atoi(&command[23]);
      if (v < 0 || v > 100) { strcpy(reply, "Err - range 0-100"); return true; }
      _pct = (uint8_t)v;
      save(fs);
      strcpy(reply, "OK");
      return true;
    }
    if (strcmp(command, "get fwd.scoped.reserve") == 0) {
      sprintf(reply, "> %d", (int)_pct);
      return true;
    }
    if (strcmp(command, "get fwd.scoped.stats") == 0) {
      sprintf(reply, "> reserve=%d%% fwd_scoped=%lu fwd_unscoped=%lu drop_unscoped=%lu saved_air=%lums air=%lu/%lums/%lus",
              (int)_pct, (unsigned long)_fwd_scoped, (unsigned long)_fwd_unscoped, (unsigned long)_drop_unscoped,
              (unsigned long)_saved_air, _win.used(now, total_air), scopedReserveWindowMax(airtime_factor),
              SCOPED_RESERVE_WINDOW_MS / 1000);
      return true;
    }
    return false;
  }
};
