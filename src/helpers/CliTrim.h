#pragma once

#include <string.h>

// mups: strip trailing blanks from a CLI command line.
//
// Why: many CLI commands are matched against the WHOLE command text
// (e.g. strcmp(command, "get txdelay")), so "get txdelay " (with a blank at
// the end, easily produced by copy & paste or phone keyboards) was rejected
// as an unknown command. Leading blanks were already skipped by the callers.
//
// What it does: removes trailing ' ', '\t', '\r' and '\n' from 'cmd', in place
// (writes a new NUL terminator; the buffer must be writable).
//
// Exception: the "password ", "set guest.password " and "neighbor.remove "
// commands are left exactly as typed.
//  - passwords: a password is never changed by this function.
//    (Note: "set guest.password " followed by nothing keeps its trailing blank,
//    which is what clears the guest password.)
//  - "neighbor.remove ": the handler matches the 16 characters INCLUDING the
//    blank, and a lone blank as the prefix means "remove all neighbors" (this is
//    what the companion app sends for "Remove All Neighbours"). Trimming would
//    turn it into the unknown command "neighbor.remove".
//
// Used by the repeater, room server and sensor handleCommand().
inline void mupsTrimCommandTail(char* cmd) {
  if (memcmp(cmd, "password ", 9) == 0 || memcmp(cmd, "set guest.password ", 19) == 0 ||
      memcmp(cmd, "neighbor.remove ", 16) == 0) return;
  char* e = cmd + strlen(cmd);
  while (e > cmd && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) *--e = '\0';
}
