# mups18 – Handbuch für die Pflege der eigenen Firmware

Kurzanleitungen für alle wiederkehrenden Aufgaben. Inhalt der Firmware
(PRs, Befehle, Voreinstellungen): siehe [`MUPS18.md`](MUPS18.md).

Dieses Handbuch ist aus dem mups17-Handbuch abgeleitet. Stellen, die sich gegenüber mups17
geändert haben, sind im Text als **„neu in 18“** oder **„anders als bei mups17“** markiert.
Aussagen, die nur aus dem Quellcode abgeleitet und noch nicht am Gerät geprüft sind, stehen
als **„(aus dem Code, nicht am Gerät getestet)“** dabei.

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
| `mups18` | **Fertige Firmware.** Wird von `build-mups18.sh` bei jedem Lauf **neu gebaut** und per Force-Push überschrieben. | nur das Skript – **nie direkt committen** |
| `mu/mups18-own` | Eigene Änderungen: Voreinstellungen (inkl. Companion-CAD), `MUPS18.md`, dieses Handbuch. **Anders als bei mups17:** keine eigene Rescue-CLI und kein eigenes WLAN mehr – das kommt jetzt aus v1.18.0 | du |
| `mu/pr-2670-ps18` | Port von PR #2670 (Repeated Sending) auf PowerSaving-v18 / v1.18.0 | du (bei PR-Updates) |
| `mu/dmc-filter-ps18` | Port des Dutch-MeshCore-Paketfilters (Stand `ecfb2d41`) samt eigenen Änderungen | du (bei Filter-Updates) |
| `mu/scoped-reserve-ps18` | Eigene Sendezeit-Reserve für Scoped Floods (`fwd.scoped.reserve`) | du |
| `mups17`, `mu/*-ps17` | alte Linie (letztes Release `mups17-3`, v1.17.1) – nicht mehr pflegen | – |
| `ufo6`, `ufo7`, `ufo8` | alte Firmware-Stände | – |

`mups18` entsteht so (Reihenfolge im Skript):

```
iotthings/PowerSaving-v18
  + MAIN_REF = 81dbae59 (v1.18.0)     (MERGE_MAIN=yes)
  + PRs aus PRS=(1349 2834 3260 3536 2586)   (je ein Squash-Commit)
  + OWN=(mu/pr-2670-ps18 mu/dmc-filter-ps18 mu/scoped-reserve-ps18 mu/mups18-own)
```

### Remotes

| Remote | Repo | Was wird geholt |
|---|---|---|
| `origin` | m0urs/MeshCore (dein Fork) | alles |
| `upstream` | meshcore-dev/MeshCore | nur `dev` und `main`; PRs holt das Skript gezielt nach `refs/pr/<nr>` |
| `iotthings` | IoTThinks/MeshCore | nur die eingetragenen `PowerSaving-vNN`-Branches (Liste: `git config --get-regexp 'remote.iotthings.fetch'`) |
| `dutch` | Dutch-MeshCore/MeshCore | nur `dmc-dev`, ohne Tags |
| `usrflo` | usrflo/MeshCore | nichts bei `fetch --all` (`skipFetchAll`) |

**Neu in 18:** Das Skript merged nicht mehr `upstream/main`, sondern den fest eingetragenen Commit
`MAIN_REF` (v1.18.0, `81dbae59`). Grund: `upstream/main` wandert mit jedem neuen offiziellen
Release weiter (v1.19 gehört in eine neue Linie). Bei einem 1.18.x-Bugfix-Release trägst du
den neuen Commit in `MAIN_REF` ein.

Anzeigen: `git remote -v` und `git config --get-regexp 'remote\..*\.fetch'`

### Skripte und Pfade

| Was | Wo |
|---|---|
| Repo | `~/meshcore/MeshCore` |
| Neubau von `mups18` | `~/meshcore/build-mups18.sh` (Sicherung: `build-mups18.sh.bak`) |
| Firmware kompilieren | `~/meshcore/mc-build.sh` |
| Python-venv (PlatformIO) | `~/meshcore/bin/activate` (lädt `mc-build.sh` selbst) |
| Fertige Firmware (lokal) | `~/meshcore/Firmware-MU/<Modell>/` |
| Fertige Firmware (Windows/iCloud) | `/mnt/c/Users/micha/iCloudDrive/Meshcore/Firmware-MU/<Modell>/` |
| Downloads (Patches von Claude) | `/mnt/c/Users/micha/Downloads` |
| Lokale WLAN-Overrides (nicht im Git) | `~/meshcore/MeshCore/platformio.local.ini` |

> Die beiden Skripte liegen **außerhalb** des Repos und sind nirgends sonst gesichert – Kopie aufbewahren.
> Sie sind als Kommentare im Skript selbst dokumentiert (Rezept-Block oben: `BASE`, `TARGET`, `MAIN_REF`, `PRS`, `PORTED`, `TRACKED`, `OWN`).

---

## 2. Normaler Ablauf

```bash
cd ~/meshcore/MeshCore
~/meshcore/build-mups18.sh          # mups18 neu bauen + pushen (ohne Tag)
git switch mups18
~/meshcore/mc-build.sh              # Menü: Alle / Repeater / Room Server / Companions / einzelnes Modell
```

- **Reihenfolge einhalten:** erst `build-mups18.sh` (ggf. mit Tag), dann `mc-build.sh`. Wird das Skript
  vergessen, baut `mc-build.sh` den alten Stand – erkennbar an der Kopfzeile des Menüs
  (z. B. `dev-<hash>` statt `v1.18.0.mups1`).
- Ohne Tag auf dem Commit heißt die Firmware `v1.18.0.dev-<hash>` (Testbuild).
- Mit Tag `mups18-N` auf dem Commit heißt sie `v1.18.0.mupsN` (Release).
- `mc-build.sh` bricht ab, wenn nicht `mups17` oder `mups18` ausgecheckt ist oder es uncommittete Änderungen gibt (Liste `BUILD_BRANCHES` im Skript).
- `mc-build.sh` ist ein **interaktives Menü** (Auswahl per Zahl), kein Aufruf mit Parametern.

Am Ende von `build-mups18.sh` erscheint ggf. ein **HINWEIS**, wenn sich ein portierter PR (#2670) oder die DMC-Filterdateien (`TRACKED`) geändert haben → [Abschnitt 8](#8-portierte-teile-aktualisieren).

---

## 3. Release-Tags

Schema: Tag `mups18-1`, `mups18-2`, … → Firmware-Version `v1.18.0.mups1`, `v1.18.0.mups2`, …
Erst testen, dann taggen.

### Vorhandene Tags anzeigen

```bash
git tag -l 'mups18-*'                      # lokal
git ls-remote --tags origin 'mups18-*'     # auf GitHub
git log --oneline -1 mups18-1              # auf welchen Commit zeigt der Tag?
```

### Neues Release taggen (Beispiel `mups18-2`)

```bash
cd ~/meshcore/MeshCore

# 1. Release-Historie in MUPS18.md ergänzen (neue Zeile unter "Release history"; Statusblock oben mit anpassen)
git switch mu/mups18-own
nano MUPS18.md                             # Zeile: | `mups18-2` | 2026-11-01 | was ist neu |
git commit -am "mups: release mups18-2"
git push origin mu/mups18-own

# 2. mups18 neu bauen UND taggen (Skript setzt + pusht den Tag)
~/meshcore/build-mups18.sh mups18-2

# 3. Release-Firmware bauen
git switch mups18
~/meshcore/mc-build.sh                     # Kopfzeile muss "v1.18.0.mups2 (Tag mups18-2)" zeigen
```

Erstes Release der Linie war `mups18-1` (2026-10-28, Tag auf `04f070ae`).

### Tag löschen

```bash
git tag -d mups18-2                        # lokal
git push origin :refs/tags/mups18-2        # auf GitHub
```

Ein evtl. GitHub-Release zu dem Tag wird dabei zum "Draft" bzw. muss separat gelöscht werden.

### Tag neu setzen (z. B. nach einem Fix, solange noch niemand die Firmware hat)

```bash
git tag -d mups18-2 && git push origin :refs/tags/mups18-2
~/meshcore/build-mups18.sh mups18-2
```

Ist die Firmware schon verteilt: **nicht** neu setzen, sondern die nächste Nummer nehmen.
Schon bei `mups18-1` musste der Tag einmal neu gesetzt werden (Filter-Update nach dem ersten Tag); das ging so
(`git tag -d` lokal, `git push origin :refs/tags/…` auf GitHub, dann das Skript mit Tag erneut).

### Tag nachträglich auf den aktuellen Stand setzen (ohne Neubau)

```bash
git switch mups18
git tag -a mups18-2 -m "Release mups18-2"
git push origin mups18-2
```

> Jeder Lauf von `build-mups18.sh` erzeugt neue Merge-Commits. Danach zeigt der Tag nicht mehr
> auf `mups18` – `mc-build.sh` baut dann wieder eine `dev`-Version. Das ist gewollt.

---

## 4. GitHub-Release

Ablauf, wie er für `mups18-1` gelaufen ist (Entwurf prüfen, dann veröffentlichen):

```bash
cd ~/meshcore/Firmware-MU
ls */*-v1.18.0.mupsN* | wc -l              # erwartet: 16 Dateien
gh release create mups18-N --repo m0urs/MeshCore \
  --title "mups18-N (v1.18.0.mupsN)" \
  --notes-file /mnt/c/Users/micha/Downloads/release-mups18-N.md \
  --draft \
  */*-v1.18.0.mupsN*
```

- **Die 16 Dateien:** 4 ESP32-Modelle (Heltec V3: Repeater, Room Server, Companion BLE, Companion WLAN) je
  `*.bin` + `*-merged.bin` = 8, dazu 4 nRF52-Modelle (RAK4631, SenseCAP Solar, T1000-E, Wio Tracker L1) je
  `*.zip` + `*.uf2` = 8.
- `--draft` legt zunächst nur einen **Entwurf** an. Im Web (`https://github.com/m0urs/MeshCore/releases`)
  Entwurf und Dateien ansehen, dann „Publish release“.
- **Nicht** „Set as a pre-release“ / `--prerelease` verwenden: Ein Pre-Release zählt nicht als „Latest“, und dann
  zeigt `https://github.com/m0urs/MeshCore/releases/latest` (Link auf der Webseite) nicht auf die neue Version.
- Beschreibung aus `MUPS18.md` (What is included, Changed defaults) zusammenstellen; Vorlage:
  `release-mups18-1.md`. Link auf die Doku immer auf den **Tag**: `https://github.com/m0urs/MeshCore/blob/mups18-N/MUPS18.md`.
- **Nicht anhängen:** WLAN-Firmware, die mit echten Zugangsdaten aus `platformio.local.ini` gebaut wurde.

### Zeiger-Branch `mups18-release` (nach jedem Release)

Die Webseite verlinkt `…/blob/mups18-release/MUPS18.md`. Damit der Link immer zur aktuellen Doku führt, zeigt der
Branch `mups18-release` auf den Commit des neuesten Release-Tags:

```bash
git push -f origin mups18-N^{commit}:refs/heads/mups18-release
git ls-remote origin refs/heads/mups18-release 'refs/tags/mups18-N*'    # beide müssen auf denselben Commit zeigen
```

### Prüfen, dass es veröffentlicht und „Latest“ ist

```bash
gh release view mups18-N --repo m0urs/MeshCore --json isDraft,isPrerelease,publishedAt,url
#   isDraft false, isPrerelease false, publishedAt gesetzt
gh api repos/m0urs/MeshCore/releases/latest --jq .tag_name     # → mups18-N
```

(`gh release view --json isLatest` gibt es nicht.) Danach die Webseite prüfen (Versionsnummer im Text, Links).

---

## 5. Voreinstellungen ändern

Gelten nur für **Neuinstallationen bzw. nach Löschen der Einstellungen**. Bestehende Geräte → per CLI ([Abschnitt 12](#12-geräte-konfigurieren)).

### Wo steht was (Branch `mu/mups18-own`)

| Was | Datei | Stelle |
|---|---|---|
| Radio-Preset (Freq, BW, SF, CR) | `examples/companion_radio/MyMesh.h`, `examples/simple_repeater/MyMesh.cpp`, `examples/simple_room_server/MyMesh.h`, `examples/simple_sensor/SensorMesh.cpp`, `examples/simple_secure_chat/main.cpp` | `#define LORA_FREQ / LORA_BW / LORA_SF / LORA_CR` am Dateianfang |
| Repeater-Werte (af, Delays, Advert, flood.*, multi.acks, path.hash.mode, loop.detect, agc.reset.interval) | `examples/simple_repeater/MyMesh.cpp` | Block `// ---- mups: own defaults …` bis `// ---- end mups defaults ----` im Konstruktor |
| Duty Cycle Companion / Room Server / Sensor | `examples/companion_radio/MyMesh.cpp`, `examples/simple_room_server/MyMesh.cpp`, `examples/simple_sensor/SensorMesh.cpp` | `_prefs.airtime_factor = 9.0;   // mups: …` |
| Companion: CAD vor dem Senden (**neu in 18**) | `examples/companion_radio/MyMesh.cpp` | `_prefs.cad_enabled = 1;   // mups: …` |
| Resend (PR #2670): Anzahl Versuche | `examples/companion_radio/MyMesh.cpp`, `examples/simple_repeater/MyMesh.cpp`, `examples/simple_room_server/MyMesh.cpp`, `examples/simple_sensor/SensorMesh.cpp` | `_prefs.max_resend_attempts = 3;   // mups: …` |
| Paketfilter-Voreinstellungen (Repeater) | `examples/simple_repeater/FilterDefaultsMups.h` | Funktion `mupsFilterDefaults()` |

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
git switch mu/mups18-own
nano examples/simple_repeater/MyMesh.cpp         # Wert im mups-Block ändern
nano MUPS18.md                                   # Tabelle "Changed default settings" anpassen
git commit -am "mups: <was geändert wurde>"
git push origin mu/mups18-own
~/meshcore/build-mups18.sh
```

Prüfen, dass der Wert im Firmware-Stand ankommt:

```bash
git switch mups18
grep -n "flood_max = " examples/simple_repeater/MyMesh.cpp
```

---

## 6. Eigene Code-Änderungen

Immer in `mu/mups18-own`, nie in `mups18`:

```bash
git switch mu/mups18-own
# ändern …
git commit -am "mups: …"
git push origin mu/mups18-own
~/meshcore/build-mups18.sh
```

Hinweis: `mu/mups18-own` allein kompiliert nicht vollständig (z. B. braucht `max.resend` den
Port von #2670). Kompiliert wird immer `mups18`.

Änderung zurücknehmen:

```bash
git switch mu/mups18-own
git log --oneline -5
git revert <commit>                 # sauberer Gegen-Commit
git push origin mu/mups18-own
```

Patch-Datei (von Claude) einspielen:

```bash
git switch mu/mups18-own
git am /mnt/c/Users/micha/Downloads/<datei>.patch.txt
git push origin mu/mups18-own
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

Ist der PR schon in PowerSaving-v18 drin (IoTThinks übernimmt manche PRs selbst), ist nichts zu tun.

### Hinzufügen

```bash
nano ~/meshcore/build-mups18.sh        # PRS=(1349 2834 3260 3536 2586 <NR>)
~/meshcore/build-mups18.sh
```

- Läuft er ohne Konflikt durch → fertig.
- Bei Konflikt → [Abschnitt 10](#10-konflikte-lösen). Die Lösung merkt sich `rerere`, danach geht es automatisch.
- Danach `MUPS18.md` ergänzen (Tabelle "What is included", ggf. "Changed behaviour", Release-Historie).

### Entfernen

```bash
nano ~/meshcore/build-mups18.sh        # Nummer aus PRS=(…) löschen
~/meshcore/build-mups18.sh             # mups18 ist danach ohne den PR – rückstandsfrei
```

`MUPS18.md` anpassen. Bereits gesetzte Tags enthalten den PR weiterhin (gewollt).

### Faustregeln für die Bewertung

- Basiert der PR auf `dev` und fasst Dateien an, die PowerSaving stark umgebaut hat
  (`RadioLibWrappers.*`, Boards, CLI), gibt es Konflikte → ggf. als Port-Branch wie #2670.
- Neue Einstellungen (Prefs) bleiben nach Entfernen des PRs als ungenutzter Wert gespeichert – harmlos.

---

## 8. Portierte Teile aktualisieren

### PR #2670 (Branch `mu/pr-2670-ps18`)

Meldung: `PR #2670 hat sich geändert: git log --oneline <alt>..refs/pr/2670`

```bash
git log --oneline <alt>..refs/pr/2670            # was ist neu?
git switch mu/pr-2670-ps18
git cherry-pick <neuer-commit>                   # einzeln übernehmen, Konflikte lösen
git push origin mu/pr-2670-ps18
nano ~/meshcore/build-mups18.sh                  # PORTED: "2670=<neuer Head-Hash>"
git rev-parse refs/pr/2670                       # → neuer Head-Hash
~/meshcore/build-mups18.sh
```

Nicht übernehmen: Änderungen am Noise-Floor-Median-Schätzer und RX-Desync-Watchdog
(`RadioLibWrappers.*`, `Custom*Wrapper.h`) – kollidieren mit Power Saving.

**Neu in 18:** Der Port ist mit der v1.18.0-Änderung #3573 zusammengeführt. Zusätzlich prüft die Kanalprüfung vor einem
Resend nicht mehr den RSSI, solange der Empfänger in der Schlafphase des RX-Power-Saving ist (Repeater und Room Server).
`get/set max.resend` gibt es auch am Companion. Die Voreinstellung ist 3 Versuche (PR #2670 selbst: 2).

### DMC-Paketfilter (Branch `mu/dmc-filter-ps18`)

Meldung: `dutch/dmc-dev: übernommene Dateien geändert: …`

Der Filter ist **nicht** eine reine Kopie: Eigene Änderungen (`mups`-Kommentare) stecken in `Filter.h`/`Filter.cpp`
(Kanal-Sperre per Schlüssel/MAC statt nur per Hash-Byte, Prüfung auf fehlerhafte Pakete nur bei Länge > 0,
`applyPrefs()` setzt Limiter und Advert-Fenster), dazu die eigenen Dateien `ChannelMAC.h` und `FilterDefaultsMups.h`.
Ein einfaches `git checkout dutch/dmc-dev -- <Dateien>` würde diese Änderungen überschreiben. Deshalb so:

```bash
git log --oneline <alt>..dutch/dmc-dev -- examples/simple_repeater docs/packet_filter_reference.md
git switch mu/dmc-filter-ps18
# je Datei einen 3-Wege-Merge: Basis = letzter portierter Dutch-Stand (<alt>), "ours" = unsere Datei, "theirs" = neuer Dutch-Stand
for f in examples/simple_repeater/{Filter.h,Filter.cpp,FilterStats.h,Limiter.h,AdvertLimiter.h,MessageAge.h,MessageContent.h,PathBlock.h,SenderRules.h} docs/packet_filter_reference.md; do
  git show <alt>:$f > /tmp/base; git show dutch/dmc-dev:$f > /tmp/theirs
  git merge-file $f /tmp/base /tmp/theirs         # Konfliktmarker von Hand auflösen
done
git commit -am "Update DMC filter to dutch/dmc-dev $(git rev-parse --short dutch/dmc-dev)"
git push origin mu/dmc-filter-ps18
nano ~/meshcore/build-mups18.sh                  # TRACKED: neuen Hash eintragen
git rev-parse dutch/dmc-dev                      # → neuer Hash
~/meshcore/build-mups18.sh
```

Prüfen:

- Neue Dateien im Dutch-Filter? Die Liste in `TRACKED` ergänzen (zuletzt kam `MessageContent.h` dazu).
- Haben sich die Einhängepunkte in `examples/simple_repeater/MyMesh.*` geändert (`git log -p … -- examples/simple_repeater/MyMesh.cpp`)?
- Dutch-Tests (gtest) gegen unseren Stand laufen lassen. Beim Update auf `ecfb2d41` waren 130 Tests grün; 2 Tests in
  `test_filter_limits` schlugen nur deshalb fehl, weil unsere Voreinstellung keine Rate-Limits setzt (Absicht, siehe `FilterDefaultsMups.h`).
- Nicht übernommen (bewusst): `dc.gate` / TX-Duty-Messung, die Region-Korrektur `e3d029b2`, Companion-Filter, SNMP/OTA, SenseCAP-GPS-Standard.

---

## 9. Neue PowerSaving-Version

Updates **innerhalb** von `PowerSaving-v18` kommen automatisch mit jedem `build-mups18.sh`
(`BASE` wird bei jedem Lauf neu geholt).

Wechsel auf eine neue Linie (z. B. `PowerSaving-v19`), so wie `mups18` aus `mups17` entstand:

```bash
git config --add remote.iotthings.fetch '+refs/heads/PowerSaving-v19:refs/remotes/iotthings/PowerSaving-v19'
git fetch iotthings
git rev-list --left-right --count upstream/main...iotthings/PowerSaving-v19   # basiert auf main?
```

Dann eine neue Firmware-Linie (`mups19`): Kopie von `build-mups18.sh` mit `BASE=iotthings/PowerSaving-v19`,
`TARGET=mups19`, `MAIN_REF` auf den passenden Release-Commit, Port-Branches `mu/…-ps19` neu erstellen
(Ports der Vorgängerlinie als Vorlage), `mc-build.sh`: Linie in `BUILD_BRANCHES` eintragen (die Version liest er aus dem Code).

Aus den Erfahrungen mit `mups18`:

- Prüfen, **was PowerSaving-vNN schon enthält** und was nicht – dort wurden Funktionen auch wieder entfernt.
  Beispiel: `outpath` (Antwortweg des Repeaters) fehlt in `PowerSaving-v18` (IoTThinks-Commit `eb1139ee` „Removed outpath“);
  das Wiki von EasySkyMesh führt es noch auf (veraltet). In `mups18` kommt `outpath` deshalb aus PR #2586.
- Jede Option, die der offizielle Code inzwischen selbst bietet, ersetzt den eigenen Code (in 18: `af`, `dutycycle`, `cad`,
  `max.resend`, `set pin`, `set wifi.*`). Eigene Duplikate entfernen statt mitschleppen.
- Pull Requests, die inzwischen in `PowerSaving-vNN` oder `main` enthalten sind, aus `PRS` streichen
  (`git merge-base --is-ancestor`, siehe [Abschnitt 7](#7-pr-hinzufügen-oder-entfernen)).
- Eingefrorene PR-Stände gelten nur für ihre Linie: `refs/pr-pin-<TARGET>/<nr>` (die Pins der 17er-Linie gelten für 18 nicht).

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
~/meshcore/build-mups18.sh                  # neu starten – rerere wendet die Lösung künftig selbst an
```

Abbrechen statt lösen:

```bash
git cherry-pick --abort      # bzw.  git merge --abort
```

Falsche Lösung gemerkt? (während der Konflikt ansteht)

```bash
git rerere forget <datei>
```

Bekannte, von `rerere` gemerkte Konflikte in `mups18`: PR #1349 (`ArduinoHelpers.h`: beide Blöcke behalten),
PR #2586 (`simple_repeater/MyMesh.cpp`) und der Branch `mu/pr-2670-ps18` (`companion_radio/MyMesh.cpp`).
(Der PR-#3260-Konflikt der 17er-Linie tritt in 18 nicht mehr auf.)

Merke: `.git/rr-cache` enthält die Lösungen – nur lokal, nicht auf GitHub.

---

## 11. Firmware flashen

| Gerät | Datei | Zweck |
|---|---|---|
| ESP32 (Heltec V3) | `*-v1.18.0.mupsN.bin` | Update (Web-Flasher ohne "merged" oder OTA), Einstellungen bleiben |
| ESP32 (Heltec V3) | `*-merged.bin` | Neuinstallation / Rettung; Web-Flasher löscht dabei den ganzen Flash |
| nRF52 (RAK4631, SenseCAP Solar, T1000-E, Wio Tracker L1) | `*.zip` | Update per DFU (Web-Flasher seriell oder nRF Connect BLE) |
| nRF52 (dieselben) | `*.uf2` | Update per Drag & Drop (Bootloader-Modus, meist Doppel-Reset) |

`.zip` und `.uf2` = gleiche Firmware, nur zwei Wege. Saubere Neuinstallation auf nRF52:
vorher löschen (Web-Flasher "Erase" oder Rescue-CLI `erase`).

Prüfen nach dem Flashen: `ver` → `v1.18.0.mupsN`.

Heltec V3 Companion: Die **BLE-Variante** enthält auch die WLAN-Schnittstelle (WLAN bleibt aus, bis eine SSID gesetzt ist);
die **reine WLAN-Variante** hat kein BLE und Platzhalter `myssid` / `mypwd`. BLE lässt sich nicht dauerhaft abschalten
(nur die Display-Seite, und das wird nicht gespeichert) – deshalb gibt es beide Varianten.

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
set flood.max 15
set flood.max.unscoped 3
set flood.max.advert 3
set multi.acks 1
set path.hash.mode 2
set loop.detect moderate
set agc.reset.interval 12
set max.resend 3
set cad on
```

Prüfen: `ver`, `get dutycycle`, `get max.resend` (zeigt auch die Resend-Quote), `get cad` (zeigt auch den Zähler „channel busy“), `get radio.rxps`.

Radio-Preset: `set radio 869.618,62.5,8,8` (Format siehe `docs/cli_commands.md`), danach Neustart.

### Companion – CLI

**Neu in 18:** Die Befehle `af`, `dutycycle`, `cad`, `max.resend`, `set pin` und die WLAN-Befehle sind **offizieller Code** aus v1.18.0
und funktionieren sowohl in der Rescue-CLI (serielle Konsole) als auch in der Companion-CLI (aus der App; Protokollbefehl
`CMD_RUN_CLI_COMMAND`). Zusätzlich kennt der Companion `powersaving on|off` (Standard: an) und die `radio.rxps`-Befehle.
Voreinstellung Companion: Duty Cycle 10 %, CAD an, `max.resend` 3.

| Befehl | Wirkung |
|---|---|
| `get dutycycle` / `set dutycycle 10` | Duty Cycle (EU: 10) |
| `get af` / `set af <0-9>` | Airtime-Faktor direkt |
| `get cad` / `set cad on\|off` | Kanalprüfung (Hardware-CAD) vor dem Senden |
| `get max.resend` / `set max.resend <0-3>` | Resend-Versuche (Standard 3) |
| `powersaving` / `powersaving on\|off` | Stromsparen am Companion |
| `get radio.rxps` | Einstellungen des RX-Duty-Cycle |
| `set pin <pin>` | BLE-PIN setzen (**`get pin` gibt es nicht mehr**, war eigener Code in mups17) |
| `set wifi.ssid <ssid>`, `set wifi.pwd <pwd>`, danach Neustart | WLAN-Zugangsdaten (nur WLAN-Companion, bzw. BLE-Variante mit WLAN) |
| `set wifi.enabled …`, `set wifi.clear` | WLAN ein/aus bzw. gespeicherte Daten löschen |
| `get wifi.status` / `get wifi.ip` | WLAN-Status / IP |
| `ls`, `cat`, `rm`, `erase`, `rebuild`, `reboot` | Dateisystem / Neustart (nur Rescue-CLI) |

**Weg in die Rescue-CLI** (nur seriell, 115200 Baud, z. B. Console auf flasher.meshcore.io oder ein Terminalprogramm):

1. Gerät neu starten.
2. **Innerhalb von 8 s nach dem Start** die User-/PRG-Taste **lang** drücken. Die Zeit zählt ab dem Start der Oberfläche
   (`UITask::begin`), nicht ab dem Einschalten.
3. Ausgabe: `========= CLI Rescue =========`

Wie lang „lang“ ist, hängt von der Oberfläche ab (aus dem Code): Geräte mit `ui-new` (Display): 1,2 s;
Geräte mit `ui-orig` (z. B. T1000-E): 3 s. Zu spät (> 8 s) löst derselbe lange Druck das **Ausschalten** aus.

**T1000-E (nur eine Taste), aus dem Code, nicht am Gerät getestet:** Hält man die Taste schon beim Start gedrückt (beim
Einschalten nötig), ist sie für die Tastenlogik von Anfang an „gedrückt“ – ein Tastendruck wird erst erkannt, wenn man sie
**loslässt und neu drückt**. Der neue Druck muss 3 s dauern und innerhalb der 8 s enden. Siehe auch
[Fehlerbehebung](#14-fehlerbehebung). Der Tastencode ist in mups18 gleich wie im offiziellen v1.18.0 und in mups17
(einziger Unterschied: Entprellzeit 20 ms statt 50 ms durch PR #3536).

Alternative für den Duty Cycle ohne Rescue-CLI: `meshcore-cli <verbindung> set tuning 0,9000`.
Die Companion-CLI-Befehle in der Tabelle gehen auch ohne Rescue-Modus.

**Anders als bei mups17:** Der eigene Fix für Zeilenenden (CRLF/LF) in der Rescue-CLI, `get pin` und die Befehle
`wifi_ssid`, `wifi_pwd`, `wifi_commit`, `wifi_show`, `wifi_clear` samt Datei `/wifi_config` gibt es **nicht mehr**.
Nach einem Update von mups17 müssen die WLAN-Zugangsdaten neu gesetzt werden.

### Update eines Companions von mups17 auf mups18

Beide Linien speichern die Einstellungen in `/prefs.json` (Schlüssel/Wert). Neue Schlüssel erhalten ihre Standardwerte.
**Getestet:** Ein Companion-Update von mups17 auf mups18 hat funktioniert (2026-10-28). Nach dem Update trotzdem prüfen: `get af`, `powersaving`, `get radio.rxps`, WLAN-Zugangsdaten neu setzen.

### WLAN-Companion beim Bauen (Alternative zur Rescue-CLI)

`~/meshcore/MeshCore/platformio.local.ini` (wird nie committet; `platformio.ini` bindet sie ein) mit vollständigem
`[env:Heltec_v3_companion_radio_wifi]`-Block und eigenen `WIFI_SSID` / `WIFI_PWD` (im Repo stehen die Platzhalter `myssid` / `mypwd`).
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
| `Your branch and 'origin/mups18' have diverged` | normal – `mups18` wird neu gebaut und per Force-Push überschrieben |
| `fatal: cannot switch branch in the middle of an am session` | `git am --abort`, Ursache prüfen (Patch evtl. schon eingespielt: `git log --oneline -5`) |
| `git am` → `patch does not apply` | Patch ist (in älterer Fassung) schon drin oder Basis passt nicht → `git am --abort`, `git log` prüfen, ggf. alten Commit mit `git reset --hard HEAD~1` entfernen |
| Skript: `Abbruch: Es läuft noch ein Merge/Cherry-Pick` | Konflikt zu Ende lösen ([10](#10-konflikte-lösen)) oder `--abort` |
| Skript: `Abbruch: Tag … existiert bereits` | Tag erst löschen ([3](#3-release-tags)) oder nächste Nummer nehmen |
| nRF52-Build: `'CFG_DEBUG' was not declared` | `DISABLE_DEBUG=0` in `mc-build.sh` |
| `mc-build.sh`: `Ausgecheckt ist '…', erwartet 'mups18'` | `git switch mups18` |
| Rescue-CLI-Modus kommt nicht | Taste zu spät (> 8 s nach Start) oder zu kurz gedrückt ([12](#12-geräte-konfigurieren)). T1000-E: Taste muss nach dem Einschalten **losgelassen und neu gedrückt** werden. Am 2026-10-28 ging es beim T1000-E zunächst nicht, später dann ohne Codeänderung (Ursache war offenbar die Bedienung/das Timing) |
| `mc-build.sh` zeigt `dev-<hash>` statt `v1.18.0.mupsN` | `build-mups18.sh mups18-N` wurde nicht (vorher) ausgeführt, oder der Tag zeigt nicht auf `HEAD` ([2](#2-normaler-ablauf), [3](#3-release-tags)) |
| WLAN-Companion verbindet nach dem Update nicht mehr | Zugangsdaten gingen verloren: `set wifi.ssid …`, `set wifi.pwd …`, Neustart |
| Befehl `get pin` / `wifi_ssid` unbekannt | gibt es in mups18 nicht mehr (siehe [12](#12-geräte-konfigurieren)) |
| `strings` nicht gefunden | `sudo apt install binutils`; Alternative: `grep -a` |
| `gh release view --json isLatest` → unknown field | Feld gibt es nicht; `publishedAt` und `gh api repos/m0urs/MeshCore/releases/latest --jq .tag_name` nutzen |
| Heruntergeladene `.sh` lässt sich nicht öffnen | als `.txt` herunterladen, mit `mv … .sh` umbenennen, `chmod +x` |
| Neue Defaults wirken nicht | Gerät hat gespeicherte Einstellungen → per CLI setzen oder Einstellungen löschen |
| Git meldet Fehler im commit-graph | `git -c core.commitGraph=false <befehl>` (beschädigter Eintrag in einer Kopie des Repos) |
