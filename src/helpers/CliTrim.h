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
// Exception: the "password " and "set guest.password " commands are left
// exactly as typed, so that a password is never changed by this function.
// (Note: "set guest.password " followed by nothing keeps its trailing blank,
// which is what clears the guest password.)
//
// Used by the repeater, room server and sensor handleCommand().
inline void mupsTrimCommandTail(char* cmd) {
  if (memcmp(cmd, "password ", 9) == 0 || memcmp(cmd, "set guest.password ", 19) == 0) return;
  char* e = cmd + strlen(cmd);
  while (e > cmd && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) *--e = '\0';
}
