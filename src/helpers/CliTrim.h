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
// Exception: these commands are left exactly as typed (same behaviour as the
// official firmware):
//  - "password " and "set guest.password ": a password is never changed by this
//    function. (Note: "set guest.password " followed by nothing keeps its
//    trailing blank, which is what clears the guest password.)
//  - "neighbor.remove ": the handler matches the 16 characters INCLUDING the
//    blank, and a lone blank as the prefix means "remove all neighbors" (this is
//    what the companion app sends for "Remove All Neighbours"). Trimming would
//    turn it into the unknown command "neighbor.remove".
//  - "set owner.info " and "set bridge.secret ": the handlers match the key
//    INCLUDING the blank, and an empty value clears the setting. Trimming would
//    turn "set owner.info " into the unknown command "set owner.info".
//
// Used by the repeater, room server and sensor handleCommand().
inline void mupsTrimCommandTail(char* cmd) {
  if (memcmp(cmd, "password ", 9) == 0 || memcmp(cmd, "set guest.password ", 19) == 0 ||
      memcmp(cmd, "neighbor.remove ", 16) == 0 || memcmp(cmd, "set owner.info ", 15) == 0 ||
      memcmp(cmd, "set bridge.secret ", 18) == 0) return;
  char* e = cmd + strlen(cmd);
  while (e > cmd && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) *--e = '\0';
}
