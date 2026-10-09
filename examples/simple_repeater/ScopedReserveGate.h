#pragma once

#include <stdint.h>

// mups: airtime reserve for scoped floods -- the pure decision, no node state,
// so it can be unit-tested natively. Idea and numbers: ACETyr/MeshCore
// (fwd.scoped.reserve, forward-filter Stage 4).
//
// The reserve is measured over a short tumbling window, not the Dispatcher's
// one-hour duty-cycle bucket: the hour bucket has so much slack that a burst
// is invisible in it. The allowance per window follows the configured duty
// cycle: window * 1 / (1 + af), e.g. 60 s at 10 % (af 9) = 6000 ms.

#define SCOPED_RESERVE_WINDOW_MS  60000UL

struct ScopedReserveWindow {
  bool started = false;
  unsigned long start = 0;   // millis at window start
  unsigned long base = 0;    // total TX airtime (ms) at window start

  // TX airtime used in the current window
  unsigned long used(unsigned long now, unsigned long total_air) {
    if (!started || (now - start) >= SCOPED_RESERVE_WINDOW_MS) {
      started = true;
      start = now;
      base = total_air;
    }
    return total_air >= base ? total_air - base : 0;
  }
};

// TX allowance for one window in ms
inline unsigned long scopedReserveWindowMax(float airtime_factor) {
  if (airtime_factor < 0) airtime_factor = 0;
  return (unsigned long)(SCOPED_RESERVE_WINDOW_MS / (1.0f + airtime_factor));
}

// true = drop this unscoped flood: forwarding it would eat into the reserve
inline bool scopedReserveDrop(unsigned long win_max, unsigned long used, uint8_t pct, uint32_t est_airtime) {
  if (pct == 0) return false;
  unsigned long remaining = win_max > used ? win_max - used : 0;
  unsigned long reserve = (unsigned long)((uint64_t)win_max * pct / 100);
  return remaining < reserve + est_airtime;
}
