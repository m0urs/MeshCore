# mups17 – Handbuch für die Pflege der eigenen Firmware

Kurzanleitungen für alle wiederkehrenden Aufgaben. Inhalt der Firmware
(PRs, Befehle, Voreinstellungen): siehe [`MUPS17.md`](MUPS17.md).

## Inhalt

1. [Überblick: Branches, Remotes, Skripte, Pfade](#1-überblick)
2. [Normaler Ablauf: neu bauen und kompilieren](#2-normaler-ablauf)
3. [Release: Tag setzen, löschen, neu setzen](#3-release-tags)
4. [GitHub-Release anlegen](#4-github-release)
5. [Voreinstellungen ändern](#5-voreinstellungen-ändern)
6. [Eigene Code-Änderungen](#6-eigene-code-änderungen)
7. [PR hinzufügen oder entfernen](#7-pr-hinzufügen-oder-entfernen)
8. [Portierte Teile aktualisieren (#2670, DMC-Filter)](#8-portierte-teile-aktualisieren)
9. [Neue PowerSaving-Version (Basis wechseln)](#9-neue-powersaving-version)
10. [Konflikte lösen](#10-konflikte-lösen)
11. [Firmware flashen](#11-firmware-flashen)
12. [Geräte konfigurieren (CLI, Rescue-CLI, WLAN, Filter)](#12-geräte-konfigurieren)
13. [Repo aufräumen](#13-repo-aufräumen)
14. [Fehlerbehebung](#14-fehlerbehebung)

---

## 1. Überblick

### Branches

| Branch | Inhalt | Wer ändert |
|---|---|---|
| `mups17` | **Fertige Firmware.** Wird von `build-mups17.sh` bei jedem Lauf **neu gebaut** und per Force-Push überschrieben. | nur das Skript – **nie direkt committen** |
| `mu/mups17-own` | Eigene Änderungen: Voreinstellungen, Rescue-CLI, WLAN, `MUPS17.md`, dieses Handbuch | du |
| `mu/pr-2670-ps17` | Port von PR #2670 (Repeated Sending) auf PowerSaving-v17 | du (bei PR-Updates) |
| `mu/dmc-filter-ps17` | Port des Dutch-MeshCore-Paketfilters (+ eigener Fix: Kanalsperre per Schlüssel) | du (bei Filter-Updates) |
| `mu/scoped-reserve-ps17` | Airtime-Reserve für scoped Floods (basiert auf `mu/dmc-filter-ps17`) | du |
| `ufo6`, `ufo7`, `ufo8` | alte Firmware-Stände | – |

`mups17` entsteht so (Reihenfolge im Skript):

```
iotthings/PowerSaving-v17
  + upstream/main                     (MERGE_MAIN=yes)
  + PRs aus PRS=(…)                   (je ein Squash-Commit)
  + mu/pr-2670-ps17, mu/dmc-filter-ps17, mu/scoped-reserve-ps17, mu/mups17-own   (OWN=(…))
```

### Remotes

| Remote | Repo | Was wird geholt |
|---|---|---|
| `origin` | m0urs/MeshCore (dein Fork) | alles |
| `upstream` | meshcore-dev/MeshCore | nur `dev` und `main`; PRs holt das Skript gezielt nach `refs/pr/<nr>` |
| `iotthings` | IoTThinks/MeshCore | nur `PowerSaving-v17` |
| `dutch` | Dutch-MeshCore/MeshCore | nur `dmc-dev`, ohne Tags |
| `usrflo` | usrflo/MeshCore | nichts bei `fetch --all` (`skipFetchAll`) |

Anzeigen: `git remote -v` und `git config --get-regexp 'remote\..*\.fetch'`

### Skripte und Pfade

| Was | Wo |
|---|---|
| Repo | `~/meshcore/MeshCore` |
| Neubau von `mups17` | `~/meshcore/build-mups17.sh` |
| Firmware kompilieren | `~/meshcore/mc-build.sh` |
| Python-venv (PlatformIO) | `~/meshcore/bin/activate` (lädt `mc-build.sh` selbst) |
| Fertige Firmware (lokal) | `~/meshcore/Firmware-MU/<Modell>/` |
| Fertige Firmware (Windows/iCloud) | `/mnt/c/Users/micha/iCloudDrive/Meshcore/Firmware-MU/<Modell>/` |
| Downloads (Patches von Claude) | `/mnt/c/Users/micha/Downloads` |
| Lokale WLAN-Overrides (nicht im Git) | `~/meshcore/MeshCore/platformio.local.ini` |

> Die beiden Skripte liegen **außerhalb** des Repos und sind nirgends sonst gesichert – Kopie aufbewahren.

---

## 2. Normaler Ablauf

```bash
cd ~/meshcore/MeshCore
~/meshcore/build-mups17.sh          # mups17 neu bauen + pushen (ohne Tag)
git switch mups17
~/meshcore/mc-build.sh              # Menü: Alle / Repeater / Room Server / Companions / einzelnes Modell
```

- Ohne Tag auf dem Commit heißt die Firmware `v1.17.1.dev-<hash>` (Testbuild).
- Mit Tag `mups17-N` auf dem Commit heißt sie `v1.17.1.mupsN` (Release).
- `mc-build.sh` bricht ab, wenn nicht `mups17` ausgecheckt ist oder es uncommittete Änderungen gibt.

Am Ende von `build-mups17.sh` erscheint ggf. ein **HINWEIS**, wenn sich ein portierter PR (#2670) oder die DMC-Filterdateien geändert haben → [Abschnitt 8](#8-portierte-teile-aktualisieren).

---

## 3. Release-Tags

Schema: Tag `mups17-1`, `mups17-2`, … → Firmware-Version `v1.17.1.mups1`, `v1.17.1.mups2`, …
Erst testen, dann taggen.

### Vorhandene Tags anzeigen

```bash
git tag -l 'mups17-*'                      # lokal
git ls-remote --tags origin 'mups17-*'     # auf GitHub
git log --oneline -1 mups17-1              # auf welchen Commit zeigt der Tag?
```

### Neues Release taggen (Beispiel `mups17-2`)

```bash
cd ~/meshcore/MeshCore

# 1. Release-Historie in MUPS17.md ergänzen (neue Zeile unter "Release history")
git switch mu/mups17-own
nano MUPS17.md                             # Zeile: | `mups17-2` | 2026-11-01 | was ist neu |
git commit -am "mups: release mups17-2"
git push origin mu/mups17-own

# 2. mups17 neu bauen UND taggen (Skript setzt + pusht den Tag)
~/meshcore/build-mups17.sh mups17-2

# 3. Release-Firmware bauen
git switch mups17
~/meshcore/mc-build.sh                     # Kopfzeile muss "v1.17.1.mups2 (Tag mups17-2)" zeigen
```

### Tag löschen

```bash
git tag -d mups17-2                        # lokal
git push origin :refs/tags/mups17-2        # auf GitHub
```

Ein evtl. GitHub-Release zu dem Tag wird dabei zum "Draft" bzw. muss separat gelöscht werden.

### Tag neu setzen (z. B. nach einem Fix, solange noch niemand die Firmware hat)

```bash
git tag -d mups17-2 && git push origin :refs/tags/mups17-2
~/meshcore/build-mups17.sh mups17-2
```

Ist die Firmware schon verteilt: **nicht** neu setzen, sondern `mups17-3` machen.

### Tag nachträglich auf den aktuellen Stand setzen (ohne Neubau)

```bash
git switch mups17
git tag -a mups17-2 -m "Release mups17-2"
git push origin mups17-2
```

> Jeder Lauf von `build-mups17.sh` erzeugt neue Merge-Commits. Danach zeigt der Tag nicht mehr
> auf `mups17` – `mc-build.sh` baut dann wieder eine `dev`-Version. Das ist gewollt.

---

## 4. GitHub-Release

**Web:** `https://github.com/m0urs/MeshCore/releases/new`
→ Tag `mups17-N` wählen → Titel `mups17-N (v1.17.1.mupsN)` → Beschreibung einfügen →
Firmware-Dateien anhängen → ggf. "Set as a pre-release" → "Publish release".

**Kommandozeile:**

```bash
cd ~/meshcore/Firmware-MU
gh release create mups17-N --repo m0urs/MeshCore \
  --title "mups17-N (v1.17.1.mupsN)" \
  --notes-file /mnt/c/Users/micha/Downloads/release-mups17-N.md \
  --prerelease \
  */*-v1.17.1.mupsN*
```

Beschreibung: aus `MUPS17.md` (What is included, Changed defaults) zusammenstellen.
Link auf die Doku immer auf den **Tag** setzen: `https://github.com/m0urs/MeshCore/blob/mups17-N/MUPS17.md`.

**Nicht anhängen:** WLAN-Firmware, die mit echten Zugangsdaten aus `platformio.local.ini` gebaut wurde.

---

## 5. Voreinstellungen ändern

Gelten nur für **Neuinstallationen bzw. nach Löschen der Einstellungen**. Bestehende Geräte → per CLI ([Abschnitt 12](#12-geräte-konfigurieren)).

### Wo steht was (Branch `mu/mups17-own`)

| Was | Datei | Stelle |
|---|---|---|
| Radio-Preset (Freq, BW, SF, CR) | `examples/companion_radio/MyMesh.h`, `examples/simple_repeater/MyMesh.cpp`, `examples/simple_room_server/MyMesh.h`, `examples/simple_sensor/SensorMesh.cpp`, `examples/simple_secure_chat/main.cpp` | `#define LORA_FREQ / LORA_BW / LORA_SF / LORA_CR` am Dateianfang |
| Repeater-Werte (af, Delays, Advert, flood.*, multi.acks, path.hash.mode, loop.detect, agc.reset.interval) | `examples/simple_repeater/MyMesh.cpp` | Block `// ---- mups: own defaults …` bis `// ---- end mups defaults ----` im Konstruktor |
| Duty Cycle Companion / Room Server / Sensor | `examples/companion_radio/MyMesh.cpp`, `examples/simple_room_server/MyMesh.cpp`, `examples/simple_sensor/SensorMesh.cpp` | `_prefs.airtime_factor = 9.0;   // mups: …` |

Alles finden:

```bash
grep -rn "mups" examples/ | grep -iv rescue
grep -rn "define LORA_\(FREQ\|BW\|SF\|CR\) " examples/
```

### Umrechnungen / Einheiten

| CLI | Im Code | Umrechnung |
|---|---|---|
| `set dutycycle 10` | `airtime_factor = 9.0` | `af = 100 / dc − 1` |
| `set advert.interval 240` (Minuten) | `advert_interval = 120` | Code = Minuten / 2 |
| `set flood.advert.interval 167` (Stunden) | `flood_advert_interval = 167` | 1:1 |
| `set agc.reset.interval 12` (Sekunden) | `agc_reset_interval = 3` | Code = Sekunden / 4 |
| `set loop.detect moderate` | `LOOP_DETECT_MODERATE` | off / minimal / moderate / strict |

### Ablauf

```bash
cd ~/meshcore/MeshCore
git switch mu/mups17-own
nano examples/simple_repeater/MyMesh.cpp         # Wert im mups-Block ändern
nano MUPS17.md                                   # Tabelle "Changed default settings" anpassen
git commit -am "mups: <was geändert wurde>"
git push origin mu/mups17-own
~/meshcore/build-mups17.sh
```

Prüfen, dass der Wert im Firmware-Stand ankommt:

```bash
git switch mups17
grep -n "flood_max = " examples/simple_repeater/MyMesh.cpp
```

---

## 6. Eigene Code-Änderungen

Immer in `mu/mups17-own`, nie in `mups17`:

```bash
git switch mu/mups17-own
# ändern …
git commit -am "mups: …"
git push origin mu/mups17-own
~/meshcore/build-mups17.sh
```

Hinweis: `mu/mups17-own` allein kompiliert nicht vollständig (z. B. braucht `max.resend` den
Port von #2670). Kompiliert wird immer `mups17`.

Änderung zurücknehmen:

```bash
git switch mu/mups17-own
git log --oneline -5
git revert <commit>                 # sauberer Gegen-Commit
git push origin mu/mups17-own
```

Patch-Datei (von Claude) einspielen:

```bash
git switch mu/mups17-own
git am /mnt/c/Users/micha/Downloads/<datei>.patch.txt
git push origin mu/mups17-own
```

---

## 7. PR hinzufügen oder entfernen

### Vorher prüfen

```bash
git fetch upstream +pull/<NR>/head:refs/pr/<NR>
git log --oneline -5 refs/pr/<NR>
git merge-base --is-ancestor refs/pr/<NR> upstream/dev  && echo "schon in dev"
git merge-base --is-ancestor refs/pr/<NR> upstream/main && echo "schon in main"
```

Ist der PR schon in PowerSaving-v17 drin (IoTThinks übernimmt manche PRs selbst), ist nichts zu tun.

### Hinzufügen

```bash
nano ~/meshcore/build-mups17.sh        # PRS=(1349 2834 3260 3536 <NR>)
~/meshcore/build-mups17.sh
```

- Läuft er ohne Konflikt durch → fertig.
- Bei Konflikt → [Abschnitt 10](#10-konflikte-lösen). Die Lösung merkt sich `rerere`, danach geht es automatisch.
- Danach `MUPS17.md` ergänzen (Tabelle "What is included", ggf. "Changed behaviour", Release-Historie).

### Entfernen

```bash
nano ~/meshcore/build-mups17.sh        # Nummer aus PRS=(…) löschen
~/meshcore/build-mups17.sh             # mups17 ist danach ohne den PR – rückstandsfrei
```

`MUPS17.md` anpassen. Bereits gesetzte Tags enthalten den PR weiterhin (gewollt).

### Faustregeln für die Bewertung

- Basiert der PR auf `dev` und fasst Dateien an, die PowerSaving stark umgebaut hat
  (`RadioLibWrappers.*`, Boards, CLI), gibt es Konflikte → ggf. als Port-Branch wie #2670.
- Neue Einstellungen (Prefs) bleiben nach Entfernen des PRs als ungenutzter Wert gespeichert – harmlos.

---

## 8. Portierte Teile aktualisieren

### PR #2670 (Branch `mu/pr-2670-ps17`)

Meldung: `PR #2670 hat sich geändert: git log --oneline <alt>..refs/pr/2670`

```bash
git log --oneline <alt>..refs/pr/2670            # was ist neu?
git switch mu/pr-2670-ps17
git cherry-pick <neuer-commit>                   # einzeln übernehmen, Konflikte lösen
git push origin mu/pr-2670-ps17
nano ~/meshcore/build-mups17.sh                  # PORTED: "2670=<neuer Head-Hash>"
git rev-parse refs/pr/2670                       # → neuer Head-Hash
~/meshcore/build-mups17.sh
```

Nicht übernehmen: Änderungen am Noise-Floor-Median-Schätzer und RX-Desync-Watchdog
(`RadioLibWrappers.*`, `Custom*Wrapper.h`) – kollidieren mit Power Saving.

### DMC-Paketfilter (Branch `mu/dmc-filter-ps17`)

Meldung: `dutch/dmc-dev: übernommene Dateien geändert: …`

```bash
git log --oneline <alt>..dutch/dmc-dev -- examples/simple_repeater docs/packet_filter_reference.md
git switch mu/dmc-filter-ps17
git checkout dutch/dmc-dev -- examples/simple_repeater/{Filter.h,Filter.cpp,FilterStats.h,Limiter.h,AdvertLimiter.h,MessageAge.h,PathBlock.h,SenderRules.h} docs/packet_filter_reference.md
git commit -m "Update DMC filter to dutch/dmc-dev $(git rev-parse --short dutch/dmc-dev)"
# eigenen Fix (Kanalsperre per Schlüssel) wieder einspielen – er steckt in Filter.cpp
git cherry-pick $(git log --format=%h -1 --grep="match blocked channels by key")
git push origin mu/dmc-filter-ps17
nano ~/meshcore/build-mups17.sh                  # TRACKED: neuen Hash eintragen
git rev-parse dutch/dmc-dev                      # → neuer Hash
~/meshcore/build-mups17.sh
```

Meldet der Cherry-Pick „empty“, hat Dutch-MeshCore den Fix selbst übernommen:
`git cherry-pick --skip`. Bei Konflikt in `Filter.cpp`: Kanal-Sperrschleife muss
`ChannelMAC::matches(...)` enthalten, Malformed-Prüfung `if (want_malformed && len > 0)`.

Prüfen, ob neue Filterdateien dazugekommen sind oder sich die Einhängepunkte in
`examples/simple_repeater/MyMesh.*` geändert haben (`git log -p … -- examples/simple_repeater/MyMesh.cpp`).

---

## 9. Neue PowerSaving-Version

Updates **innerhalb** von `PowerSaving-v17` kommen automatisch mit jedem `build-mups17.sh`.

Wechsel auf z. B. `PowerSaving-v18`:

```bash
git config --add remote.iotthings.fetch '+refs/heads/PowerSaving-v18:refs/remotes/iotthings/PowerSaving-v18'
git fetch iotthings
git rev-list --left-right --count upstream/main...iotthings/PowerSaving-v18   # basiert auf main?
```

Dann am besten eine neue Firmware-Linie (`mups18`): Kopie von `build-mups17.sh` mit
`BASE=iotthings/PowerSaving-v18`, `TARGET=mups18`, Port-Branches `mu/…-ps18` neu erstellen
(Ports von PS17 als Vorlage: `git cherry-pick` der Port-Commits auf den neuen Basis-Branch),
`FIRMWARE_VERSION`-Basis in `mc-build.sh` liest die Version automatisch aus dem Code.
Prüfen, welche PRs inzwischen schon in PS18 enthalten sind.

---

## 10. Konflikte lösen

Das Skript stoppt mit `KONFLIKT bei: …`.

```bash
git status                                  # welche Dateien?
git diff                                    # Konfliktstellen <<<<<<< ======= >>>>>>>
nano <datei>                                # auflösen, Marker entfernen
git add <datei>
git -c core.editor=true cherry-pick --continue   # bei PRs
# oder: git commit --no-edit                     # bei Merges (Branches, upstream/main)
~/meshcore/build-mups17.sh                  # neu starten – rerere wendet die Lösung künftig selbst an
```

Abbrechen statt lösen:

```bash
git cherry-pick --abort      # bzw.  git merge --abort
```

Falsche Lösung gemerkt? (während der Konflikt ansteht)

```bash
git rerere forget <datei>
```

Bekannte, von `rerere` gemerkte Konflikte: PR #1349 (`ArduinoHelpers.h`: beide Blöcke behalten),
PR #3260 (`simple_repeater/MyMesh.cpp`: `if (isValidPathLen(...) && !is_retry)`).

Merke: `.git/rr-cache` enthält die Lösungen – nur lokal, nicht auf GitHub.

---

## 11. Firmware flashen

| Gerät | Datei | Zweck |
|---|---|---|
| ESP32 (Heltec V3) | `*-v1.17.1.mupsN.bin` | Update (Web-Flasher ohne "merged" oder OTA), Einstellungen bleiben |
| ESP32 (Heltec V3) | `*-merged.bin` | Neuinstallation / Rettung; Web-Flasher löscht dabei den ganzen Flash |
| nRF52 (RAK4631, SenseCAP Solar, T1000-E, Wio Tracker L1) | `*.zip` | Update per DFU (Web-Flasher seriell oder nRF Connect BLE) |
| nRF52 (dieselben) | `*.uf2` | Update per Drag & Drop (Bootloader-Modus, meist Doppel-Reset) |

`.zip` und `.uf2` = gleiche Firmware, nur zwei Wege. Saubere Neuinstallation auf nRF52:
vorher löschen (Web-Flasher "Erase" oder Rescue-CLI `erase`).

Prüfen nach dem Flashen: `ver` → `v1.17.1.mupsN`.

### Build-Einstellungen (`mc-build.sh`)

- `DISABLE_DEBUG=0` lassen. Mit `=1` entfernt `build.sh` u. a. `CFG_DEBUG`, und nRF52-Builds brechen ab.
- Modelle: Liste `MODELS=(…)` sowie die Gruppen `REPEATERS`, `ROOMSERVERS`, `COMPANIONS`.
- Zielordner Windows: `WINDOWS_COPY_TARGET` (Unterordner `Firmware-MU` wird angehängt).

---

## 12. Geräte konfigurieren

### Repeater / Room Server (CLI, per App oder seriell)

Voreinstellungen auf einem **bestehenden** Repeater nachziehen:

```
set dutycycle 10
set rxdelay 2
set txdelay 1.0
set direct.txdelay 0.4
set advert.interval 240
set flood.advert.interval 167
set flood.max 10
set flood.max.unscoped 3
set flood.max.advert 3
set multi.acks 1
set path.hash.mode 2
set loop.detect moderate
set agc.reset.interval 12
```

Prüfen: `ver`, `get dutycycle`, `get max.resend` (zeigt auch die Resend-Quote), `get radio.rxps`.

### Optionale Funktionen (alle ab Werk aus, außer Power Saving)

| Funktion | Einschalten | Ausschalten |
|---|---|---|
| Wiederholtes Senden (PR #2670) | `set max.resend 2` (1–3) | `set max.resend 0` |
| DMC-Paketfilter | `filter on` (vorher `filter dryrun on`) | `filter off` |
| Airtime-Reserve für scoped Floods | `set fwd.scoped.reserve 40` | `set fwd.scoped.reserve 0` |
| RX Power Saving (PS17, ab Werk **an**) | `set radio.rxps on` | `set radio.rxps off` |

Geräte, die schon mit `mups17-1` liefen, haben `max.resend 2` gespeichert und
behalten das – bei Bedarf `set max.resend 0`.

Radio-Preset: `set radio 869.618,62.5,8,8` (Format siehe `docs/cli_commands.md`), danach Neustart.

### Companion – Rescue-CLI

Einstieg: Gerät neu starten, **innerhalb von 8 s** User-/PRG-Taste **lang** (≥ 3 s) drücken.
T1000-E (nur eine Taste): einschalten, loslassen, sofort 3 s halten. Zu spät → Gerät schaltet sich aus.
Konsole: serielles Terminal 115200 Baud oder Console auf flasher.meshcore.io.

| Befehl | Wirkung |
|---|---|
| `get dutycycle` / `set dutycycle 10` | Duty Cycle (EU: 10) |
| `get af` / `set af <0-9>` | Airtime-Faktor direkt |
| `get max.resend` / `set max.resend <0-3>` | Resend-Versuche |
| `get pin` | BLE-PIN anzeigen |
| `wifi_ssid <ssid>`, `wifi_pwd <pwd>`, `wifi_commit` | WLAN-Zugangsdaten speichern (nur WLAN-Companion), Neustart |
| `wifi_show` / `wifi_clear` | Status / gespeicherte WLAN-Daten löschen |
| `ls`, `cat`, `rm`, `erase`, `rebuild`, `reboot` | Dateisystem / Neustart |

Alternative für den Duty Cycle ohne Rescue-CLI: `meshcore-cli <verbindung> set tuning 0,9000`.

### WLAN-Companion beim Bauen (Alternative zur Rescue-CLI)

`~/meshcore/MeshCore/platformio.local.ini` (wird nie committet) mit vollständigem
`[env:Heltec_v3_companion_radio_wifi]`-Block und eigenen `WIFI_SSID` / `WIFI_PWD`.
Solche Firmware enthält das Passwort → nicht veröffentlichen.

### DMC-Paketfilter (nur Repeater)

```
filter                 # Status (Standard: off)
filter dryrun on       # nur zählen, trotzdem weiterleiten
filter on
filter stats           # was wurde (bzw. würde) gefiltert
filter dryrun off      # erst wenn die Statistik plausibel ist
filter help
```

Referenz: `docs/packet_filter_reference.md`.

### Airtime-Reserve für scoped Floods (nur Repeater)

```
get fwd.scoped.reserve
set fwd.scoped.reserve 40     # 40 % der eigenen Sendezeit je 60 s für Floods mit Region freihalten
get fwd.scoped.stats          # weitergeleitet scoped/unscoped, verworfen, gesparte Airtime, air=genutzt/Budget
set fwd.scoped.reserve 0      # aus (Standard)
```

Vorher prüfen, ob es sich lohnt: zwei Status-Abfragen im Abstand von einigen
Minuten zu einer belebten Zeit, Δ Sendezeit ÷ Δ Laufzeit = Duty Cycle. Nur wenn
der in die Nähe von 10 % kommt bzw. die TX-Warteschlange öfter belegt ist,
greift die Reserve überhaupt. Zähler stehen nach Neustart wieder auf 0.

---

## 13. Repo aufräumen

```bash
git fetch --all --prune
git branch -vv | grep ': gone]'                       # lokale Branches, deren Remote weg ist

# lokale Branches, die es nicht auf origin gibt
git for-each-ref --format='%(refname:short)' refs/heads/ | while read b; do
  git show-ref --verify --quiet "refs/remotes/origin/$b" || echo "$b"
done

# Commits, die auf keinem Remote liegen (vor dem Löschen prüfen!)
git log --oneline <branch> --not --remotes

git branch -d <branch>                                # sicher löschen
git branch -D <branch>                                # erzwingen
git push origin --delete <branch>                     # auf GitHub löschen

git update-ref -d refs/pr/<NR>                        # alten PR-Ref entfernen
git tag -d <fremder-tag>                              # z. B. versehentlich geholte dmc-* Tags
```

---

## 14. Fehlerbehebung

| Symptom | Ursache / Lösung |
|---|---|
| `Your branch and 'origin/mups17' have diverged` | normal – `mups17` wird neu gebaut und per Force-Push überschrieben |
| `fatal: cannot switch branch in the middle of an am session` | `git am --abort`, Ursache prüfen (Patch evtl. schon eingespielt: `git log --oneline -5`) |
| `git am` → `patch does not apply` | Patch ist (in älterer Fassung) schon drin oder Basis passt nicht → `git am --abort`, `git log` prüfen, ggf. alten Commit mit `git reset --hard HEAD~1` entfernen |
| Skript: `Abbruch: Es läuft noch ein Merge/Cherry-Pick` | Konflikt zu Ende lösen ([10](#10-konflikte-lösen)) oder `--abort` |
| Skript: `Abbruch: Tag … existiert bereits` | Tag erst löschen ([3](#3-release-tags)) oder nächste Nummer nehmen |
| nRF52-Build: `'CFG_DEBUG' was not declared` | `DISABLE_DEBUG=0` in `mc-build.sh` |
| `mc-build.sh`: `Ausgecheckt ist '…', erwartet 'mups17'` | `git switch mups17` |
| Rescue-CLI-Antwort erscheint erst beim nächsten Befehl | alte Firmware ohne CRLF-Fix – aktuelle `mups17` flashen |
| Rescue-CLI-Modus kommt nicht | Taste zu spät (> 8 s) oder zu kurz (< 3 s) gedrückt |
| Heruntergeladene `.sh` lässt sich nicht öffnen | als `.txt` herunterladen, mit `mv … .sh` umbenennen, `chmod +x` |
| Neue Defaults wirken nicht | Gerät hat gespeicherte Einstellungen → per CLI setzen oder Einstellungen löschen |
