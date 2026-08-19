# Star Catcher

Ein kleines Nintendo-DS-Homebrew-Spiel, gebaut mit **devkitARM** + **libnds**.

Der Spieler steuert ein Raumschiff am unteren Rand des oberen Bildschirms mit
dem Steuerkreuz, fängt herabfallende Sterne (+10 Punkte) ein und kann rote
Meteoriten mit Projektilen zersprengen (+5 Punkte, mit kleiner
Explosionsanimation) oder ihnen ausweichen — bei Kontakt kostet das ein Leben.
Hat man genug Sterne für das aktuelle Level eingesammelt, erscheint ein
**Portal**; fliegt der Spieler hindurch, geht es ins nächste (schwerere)
Level. Bei 0 Leben und einem Top-5-würdigen Punktestand tippt man seine
**Initialen** über eine Touch-Tastatur auf dem unteren Bildschirm ein, danach
zeigt der **obere** Bildschirm die **Bestenliste**.

Das **untere** Bildschirm-HUD ist kein reiner Text-Screen mehr: ein von Hand
gezeichneter Verlaufs-/Panel-Hintergrund (eigener Bitmap-Background-Layer)
liegt hinter dem Text, dazu zeigen echte Herz-Sprites die Leben an statt
einer nackten Zahl. Die Spielgrafik (Schiff, Stern, Meteor, Portal,
Projektil, Explosion) läuft als OAM-Bitmap-Sprites auf dem **oberen**
Bildschirm während des Spiels; nach Game Over übernimmt dort stattdessen ein
zweiter Text-Layer für Highscore-Vorschau und Bestenliste.

Schiff, Stern, Meteor und Portal kommen aus einem handgezeichneten
32x32-Pixelart-Spritesheet (`graphics/spritesheet.png`, 3 Frames pro Reihe)
und werden über `grit` eingebunden. Jeder Typ hat eine kleine Animation:
Sterne rotieren im Fall und zeigen beim Einsammeln kurz einen Funkeleffekt,
Meteoriten wechseln zwischen zwei Gesteinsvarianten und zeigen beim Treffer
erst einen Riss, dann eine Explosion, das Portal rotiert durch 3
Wirbel-Frames, und das Schiff hat Idle-/Blinklicht-/Explosions-Frames (bei
0 Leben explodiert es kurz auf dem oberen Bildschirm, bevor es zum
Game-Over-Screen geht). Nur das Projektil ist weiterhin prozedural in C
gezeichnet (kein Frame dafür im Spritesheet).

Im Hintergrund läuft durchgehend Musik (`nitrofs/music.raw`, aus
`assets/Sternenfall.mp3` konvertiert), die per `maxmod`-Streaming über
NitroFS von der Karte gelesen wird (nicht als Datei im ARM9-Programm
eingebettet — siehe technischer Hintergrund unten, warum).

Beim Start zeigt das Spiel außerdem ein kurzes Intro-Video (`assets/intro.mp4`,
5 Sekunden) auf dem oberen Bildschirm, bevor der Titelbildschirm erscheint —
mit START/A/B überspringbar. Der DS hat keine Videohardware, deshalb läuft
das technisch als "Diashow": 121 Einzelbilder bei 256x192, 256 Farben, aus
NitroFS gestreamt (siehe technischer Hintergrund unten).

Jedes **5. Level** (5, 10, 15, ...) ersetzt einen **Boss-Kampf** das normale
Stern-/Meteoriten-Level: Der Boss hält seine Position nahe der Bildschirm-
oberkante und schießt Projektile gezielt in Richtung des Spielers, während
eine Lebensanzeige über ihm anzeigt, wie viele Treffer er noch aushält. Der
Spieler schießt mit `A` wie gewohnt zurück (+20 Punkte pro Treffer). Ist der
Boss besiegt (dritter Frame: zerstört, +100 Bonus-Punkte), geht es direkt ins
nächste normale Level — kein Portal nötig.

## Projektstruktur

```text
NDSGame/
    source/
        main.c         # Zustandsmaschine (Titel/Spiel/Level-Ende/Game Over/
                        # Initialen-Eingabe/Bestenliste) + Game-Loop,
                        # Schuss- & Portal-Logik
        player.c        # Spieler-Bewegung & Zeichnen
        entities.c      # Pool aus Sternen/Meteoriten/Portal: Spawnen,
                        # Kollision, Explosions-Animation
        projectiles.c   # Pool aus Spieler-Projektilen
        boss.c          # Boss-Gegner (jedes 5. Level): Position, Lebens-
                        # punkte, gezielte Projektile, Lebensanzeige
        sprites.c       # Laedt Schiff/Stern/Meteor/Portal/Boss-Frames aus
                        # dem Spritesheet, zeichnet nur noch Projektil-Bolt,
                        # Boss-Geschoss und Lebensanzeige-Pip prozedural
        hud.c           # Bottom-Screen-HUD während des Spiels (Bitmap-BG +
                        # Konsole + Herz-Sprites)
        keyboard.c      # Touch-Tastatur auf dem Bottom Screen zur
                        # Initialen-Eingabe
        topscreen.c     # Text-Layer auf dem Top Screen für Highscore-
                        # Vorschau & Bestenliste
        leaderboard.c   # Top-5-Bestenliste inkl. Speichern via libfat
        music.c         # Startet NitroFS + maxmod-Stream fuer die
                        # Hintergrundmusik (siehe technischer Hintergrund)
        intro.c         # Spielt das Boot-Intro (Bilderfolge aus NitroFS)
                        # auf dem oberen Bildschirm ab, ueberspringbar
    include/
        game.h
        player.h
        entities.h
        projectiles.h
        boss.h
        sprites.h
        hud.h
        keyboard.h
        topscreen.h
        leaderboard.h
        music.h
        intro.h
    graphics/
        spritesheet.png  # 96x160 Pixelart, 5 Reihen (Stern/Meteor/Portal/
                        # Schiff/Boss) x 3 Frames, weisser Hintergrund =
                        # transparent
        spritesheet.grit # grit-Flags fuer die Konvertierung (16bpp Bitmap,
                        # Alpha-Bit statt Palette)
    nitrofs/
        music.raw        # 16kHz/mono/16bit rohes PCM, ca. 5.7MB - wird zur
                        # Laufzeit von der Karte gestreamt, nicht ins
                        # ARM9-Programm eingebettet
        intro.raw        # 121 Frames x 256x192, 8bpp indiziert, ca. 5.7MB -
                        # ebenfalls gestreamt statt eingebettet
        intro.pal        # die 256-Farben-Palette dazu (512 Bytes, RGB555)
    assets/
        icon.png         # Original-Boxart (Referenz, nicht Teil des Builds)
        icon_final.bmp   # 32x32, 8bpp indiziertes BMP - das echte Cartridge-
                        # Icon (siehe Hinweis unten, warum BMP statt PNG)
        Sternenfall.mp3  # Original-Musikdatei (Referenz, nicht Teil des
                        # Builds - nitrofs/music.raw ist die konvertierte
                        # Version, die tatsaechlich verwendet wird)
        intro.mp4        # Original-Introvideo (Referenz, nicht Teil des
                        # Builds - nitrofs/intro.raw+.pal ist die
                        # konvertierte Version, die tatsaechlich verwendet
                        # wird)
        spritesheet_reference.png # Vollaufloesung des finalen Spritesheets
        build_spritesheet*.ps1    # PowerShell-Skripte, mit denen das
                        # Spritesheet aus der Originalvorlage bearbeitet wurde
        build_intro.ps1  # Skript, das aus den per ffmpeg extrahierten
                        # Intro-Frames intro.raw/intro.pal erzeugt
    Makefile
    StarCatcher.nds     # entsteht beim Bauen (nicht eingecheckt, ca. 12MB
                        # wegen Musik + Intro-Video)
```

Schiff/Stern/Meteor/Portal kommen aus `graphics/spritesheet.png` (siehe
oben), nur das Projektil wird noch **prozedural in C erzeugt** (ein simpler
Lichtbalken, dafür lohnt sich kein eigener Spritesheet-Frame). Der
`grit`-Workflow dafür ist ganz normal über `GRAPHICS`/`.grit`-Dateien in der
Makefile eingebunden (siehe Kommentare dort) — eigene Sprites hinzufügen
funktioniert also genauso.

**Cartridge-Icon (wichtiger Hinweis):** Das Icon (`assets/icon_final.bmp`)
wird bewusst **nicht** über `grit` erzeugt, sondern als fertiges 32x32,
8bpp-indiziertes BMP direkt an `ndstool -b` übergeben. Grund: Auf dieser
devkitPro-Version kommt `grit`s eigenes Icon-`.grf`-Containerformat beim
Einbetten über `ndstool` beschädigt heraus (durch Extrahieren und
Byte-für-Byte-Dekodieren des eingebetteten Icons direkt aus der `.nds`
verifiziert — Palette und Pixel wurden vertauscht/verworfen). Ein direktes,
bereits indiziertes BMP über `-b` funktioniert dagegen zuverlässig. Falls du
das Icon änderst: als indiziertes `.bmp` (≤256 Farben) exportieren, nicht als
`.png` über den normalen `GRAPHICS`-Pfad — die Makefile-Logik dafür steht in
den Kommentaren rund um `ICON`/`GAME_ICON`.

## Bauen

Voraussetzung: [devkitPro](https://devkitpro.org/wiki/Getting_Started) mit
devkitARM ist installiert (bei dir bereits vorhanden unter `C:\devkitPro`).

**Wichtig unter Windows/PowerShell:** `DEVKITARM`/`DEVKITPRO` müssen mit
Schrägstrichen gesetzt werden (nicht `C:\devkitPro`, sondern `C:/devkitPro`),
da die Build-Regeln intern über eine MSYS-Shell laufen, die Backslashes als
Escape-Zeichen interpretiert.

```powershell
$env:DEVKITARM = "C:/devkitPro/devkitARM"
$env:DEVKITPRO = "C:/devkitPro"
$env:PATH = "C:\devkitPro\devkitARM\bin;C:\devkitPro\tools\bin;" + $env:PATH
make
```

Ergebnis: `StarCatcher.nds` im Projektordner.

`make clean` entfernt alle Build-Artefakte.

## Testen

- **Emulator:** [melonDS](https://melonds.kuribo64.net/) oder
  [DeSmuME](https://desmume.org/) — beide waren auf diesem Rechner nicht
  gefunden, müssten separat installiert werden. Einfach `StarCatcher.nds`
  öffnen. Die Touch-Tastatur lässt sich per Mausklick auf den unteren
  Bildschirmbereich bedienen.
- **Echte Hardware:** `StarCatcher.nds` auf eine Homebrew-kompatible
  Flashkarte (z. B. R4) kopieren. Die Bestenliste wird dann als
  `/StarCatcher.sav` im Root der Flashkarte/SD gespeichert.

**Hinweis:** HUD (unten) und Bestenliste/Initialen-Eingabe (oben) teilen sich
jeweils eine eigene VRAM-Bank zwischen einem selbstgezeichneten Bitmap-
Hintergrund bzw. den Sprites und einer Text-Konsole (siehe Kommentare in
`hud.c` und `topscreen.c` zu `tileBase`/`mapBase`/VRAM-Bänken). Das ist
rechnerisch geprüft, aber nicht auf echter Hardware/im Emulator visuell
verifiziert — falls beim Testen etwas am Layout nicht stimmt (Position von
Text/Rahmen/Tasten), sag Bescheid, das lässt sich leicht nachjustieren.

**Hinweis zur Musik:** `StarCatcher.nds` ist jetzt ca. 12MB groß (Musik +
Intro-Video dominieren das). Das Streaming selbst (`music.c`) folgt exakt dem
offiziellen devkitPro-Beispiel für `maxmod`-Streaming, konnte hier aber nicht
tatsächlich abgespielt/gehört werden (kein Emulator installiert). Falls die
Musik stottert oder gar nicht startet: `MUSIC_BUFFER_SAMPLES` in `music.c`
erhöhen (mehr Zeitpuffer gegen langsame Kartenzugriffe) bzw. prüfen ob
`nitroFSInit()` überhaupt erfolgreich ist (manche Emulator-Konfigurationen
ohne virtuelle SD/DLDI könnten hier Probleme machen, ähnlich wie beim
Leaderboard-Speichern).

**Hinweis zum Intro:** Dieselbe Einschränkung gilt fürs Intro-Video
(`intro.c`) — Frame-Timing/NitroFS-Zugriff nicht in einem Emulator
gegengetestet. Ein bekanntes, in Kauf genommenes Risiko: Jeder Frame wird
per `fread()` direkt in den sichtbaren VRAM-Puffer geschrieben (kein
Doppelpuffer), daher ist bei jedem der ca. 24 Frame-Wechsel über die 5
Sekunden ein kurzes Reißen/Flackern im Bild theoretisch möglich, falls das
Karten-Lesen länger dauert als die verbleibende Anzeigezeit. Für ein
5-Sekunden-Intro als vertretbar eingeschätzt; bei sichtbaren Artefakten
lässt sich das mit einem Vorab-Lesen des nächsten Frames in einen RAM-Puffer
(`intro.c`) beheben.

## Steuerung

| Eingabe            | Aktion                                          |
|---------------------|--------------------------------------------------|
| Steuerkreuz          | Schiff links/rechts bewegen                      |
| A                    | Schießen (Projektil nach oben)                   |
| START                | Bestätigen / Spiel starten / weiter              |
| SELECT               | Bestenliste ansehen (im Titelbildschirm)         |
| Touchscreen (unten)  | Buchstaben auf der Tastatur antippen             |
| B (in der Tastatur)  | Letzten Buchstaben löschen                       |
| START/A/B (im Intro) | Intro-Video überspringen                         |

## Spielablauf

- **Level:** Jedes Level braucht `8 + (Level-1) * 2` eingesammelte Sterne,
  bevor ein Portal erscheint (fällt wie Sterne/Meteoriten von oben herab).
  Durchfliegen des Portals erhöht den Levelzähler, Punktestand bleibt
  erhalten, Schwierigkeit (Fallgeschwindigkeit/Spawnrate) steigt weiter mit
  dem Punktestand.
- **Meteoriten:** Rot, unregelmäßig geformt. Mit `A` abschießen zersprengt
  sie (3-stufige Explosionsanimation: Blitz → Feuerring → verblassender
  Rauchring) und gibt +5 Punkte. Berührung mit dem Schiff kostet 1 Leben
  (der Meteor explodiert dabei ebenfalls).
- **Boss (jedes 5. Level):** Ersetzt das normale Level komplett — es spawnen
  keine Sterne/Meteoriten, kein Portal. Der Boss bleibt fix nahe der
  Bildschirmoberkante stehen und feuert alle ~1,2 Sekunden ein Projektil
  gezielt Richtung Spieler (Treffer kostet 1 Leben). Die Lebensanzeige über
  ihm (8 Segmente) zeigt, wie oft er noch getroffen werden muss; jeder
  Volltreffer mit `A` gibt +20 Punkte, das Besiegen zusätzlich +100 Punkte
  und zeigt kurz den dritten (zerstörten) Frame, bevor es direkt ins nächste
  normale Level geht.
- **Game Over → Initialen → Bestenliste:** Reicht der Punktestand für die
  Top 5, zeigt der obere Bildschirm eine Live-Vorschau der eingegebenen
  Initialen, während unten eine QWERTY-Touch-Tastatur zum Eintippen (3
  Buchstaben, danach `[OK]` antippen oder START drücken) erscheint.
  Anschließend zeigt der obere Bildschirm die aktualisierte Bestenliste mit
  einem `->`-Marker auf dem neuen Eintrag. Reicht der Punktestand nicht,
  geht es direkt zur Bestenliste.

## Technischer Hintergrund

- **ARM9** enthält die komplette Spiellogik (dieses Projekt hat keinen
  eigenen ARM7-Code mehr nötig — die installierte devkitPro-Version bindet
  automatisch einen fertigen ARM7-Kern aus `calico` beim Linken ein, siehe
  `$(DEVKITARM)/ds_rules`). Das ist neuer als die klassischen
  "combined ARM7+ARM9"-Tutorials, die man online oft zuerst findet.
- **Video (oben):** `MODE_5_2D` (nicht `MODE_0_2D` — wird für die
  Bitmap-Ebene des Intros gebraucht, siehe unten). VRAM-Bank A trägt die
  Sprite-Grafiken (Schiff/Sterne/Meteoriten/Portal/Projektile/Explosionen,
  alle direct-color Bitmap-Sprites, `SpriteMapping_Bmp_1D_128`). VRAM-Bank B
  trägt zwei Layer: BG0, einen unabhängigen Text-Layer für die
  Highscore-/Bestenliste-Anzeige (bleibt während des Spiels leer, sodass er
  die Sprites nicht stört), und BG2, eine 256-Farben-Bitmap-Ebene fürs
  Intro-Video (siehe unten) — platziert bei `mapBase 4` (64KB-Marke), damit
  sie nicht mit dem Text-Layer kollidiert (gleiche Technik wie beim
  Bottom-Screen-HUD, siehe dort).
- **Video (unten, HUD):** `MODE_5_2D` mit zwei Layern auf VRAM-Bank C: BG3
  als 256-Farben-Bitmap-Hintergrund (Verlauf + Kopfleiste, Priorität 3 = ganz
  hinten) und BG0 als Text-Konsole (Priorität 0 = vorne). Da Text-Kacheln mit
  Palettenindex 0 transparent sind, scheint der bunte Bitmap-Hintergrund
  überall dort durch, wo kein Buchstabe steht. Dieselbe Konsole zeigt während
  `STATE_ENTER_INITIALS` die Tastatur statt des normalen HUDs (`keyboard.c`
  übernimmt das Zeichnen). Die Herz-Icons für Leben laufen über eine eigene,
  unabhängige Sprite-Engine (`oamSub`) auf VRAM-Bank D.
- **Tastatur:** QWERTY-Layout aus reinem Konsolentext; jeder Buchstabe hat
  eine 16x16-Pixel-Trefferzone, die per `touchRead()` gegen die
  Touch-Koordinate geprüft wird (`keyboard.c`).
- **Leaderboard:** `libfat` (`fatInitDefault()`) speichert Top 5 (inkl.
  Initialen) als `/StarCatcher.sav`. Schlägt das Mounten fehl (kein
  Flashcard/SD, manche Emulator-Konfigurationen), bleibt die Bestenliste
  einfach nur für die laufende Sitzung im RAM bestehen — kein Absturz.
- **Hintergrundmusik:** Der komplette ARM9-Programmcode (inkl. allem, was
  als `.rodata`/eingebettetes Binary im Programm liegt) wird beim Boot
  vollständig ins 4MB-Haupt-RAM des DS kopiert — dort hätte ein 3-minütiger
  Song unkomprimiert (~5.7MB) gar nicht reingepasst. Deshalb liegt
  `music.raw` stattdessen in **NitroFS** (`nitrofs/`-Verzeichnis, siehe
  `NITRO`-Variable in der Makefile) und wird per `maxmod`-Streaming
  (`mmStreamOpen`, automatischer Hintergrund-Thread) häppchenweise direkt
  von der Karte gelesen (`music.c`) — nie vollständig im RAM. Format: 16kHz,
  mono, 16-bit PCM (aus der Original-MP3 konvertiert), das hält den
  Speicherbedarf für den Streaming-Puffer klein und die Lesegeschwindigkeit
  unkritisch. Loopt nahtlos über die ganze Spielsitzung.
- **Intro-Video:** Der DS hat keine Videodecoder-Hardware (kein H.264 o.ä.),
  daher gibt's kein "echtes" Video. Stattdessen: `assets/intro.mp4` wurde per
  `ffmpeg` (`palettegen`/`paletteuse`) auf eine gemeinsame 256-Farben-Palette
  reduziert und in 121 Einzelbilder (256x192, 8bpp, 24fps) zerlegt
  (`assets/build_intro.ps1`), alle zu einer Datei `nitrofs/intro.raw`
  aneinandergehängt. Aus demselben RAM-Grund wie bei der Musik läuft auch das
  über NitroFS-Streaming, nicht eingebettet: `intro.c` liest pro Frame
  49152 Bytes direkt in den VRAM-Bitmap-Puffer (BG2, s.o.) und synchronisiert
  die Anzeige über einen VBlank-Zähler auf die Ziel-Framerate.
- **Boss-Zielführung ohne Trigonometrie:** `boss.c` berechnet beim Abfeuern
  eines Schusses einmalig `dx = spielerX - bossX`, `dy = spielerY - bossY`
  und teilt beide durch eine feste Anzahl Frames (`BOSS_BULLET_TRAVEL_FRAMES`,
  50). Das ergibt eine geradlinige, auf den Spieler gezielte Flugbahn ganz
  ohne `sqrt`/Winkelfunktionen — für ein einfaches 2D-Spiel völlig
  ausreichend und auf der ARM9-CPU (keine Hardware-FPU) günstiger.
- **Rotierendes 3D-Schiffsmodell (oberer Bildschirm):** Der DS hat genau
  eine 3D-fähige Engine ("Engine A"), und die hängt fest an dem Bildschirm,
  dem sie per `lcdMainOnBottom()` gerade zugewiesen ist — sie kann nicht
  parallel zur bestehenden 2D-Sprite-Logik (Schiff, Sterne, Meteoriten,
  Boss, ...) unabhängig in Echtzeit rendern. Statt dessen wird das
  gelieferte Modell (`assets/Starfighter/Starfighter.obj`, ursprünglich
  ~442k Dreiecke) einmalig **offline** in Blender (headless, per
  `blender_render.py`) auf 1400 Dreiecke reduziert (`Decimate`-Modifier),
  zentriert/normiert und über 48 Frames um die eigene Hochachse (Z) rotiert
  gerendert (96x96px, orthografische Kamera, `EEVEE_NEXT`). Die 48 PNGs
  werden dann — exakt wie beim Intro-Video — per `ffmpeg`
  (`palettegen`/`paletteuse`) auf eine gemeinsame 128-Farben-Palette
  reduziert (`assets/build_starfighter.ps1`) und zu
  `nitrofs/starfighter.raw` + `nitrofs/starfighter.pal` zusammengefasst.
  Zur Laufzeit (`starfighter.c`) besitzt die Animation eine **eigene,
  dedizierte Bitmap-Ebene** (BG2) auf dem oberen Bildschirm, in derselben
  VRAM-Bank/demselben Offset, das schon das Boot-Intro benutzt und nach dem
  Abspielen per `bgHide()` wieder freigibt (`intro.c`) — `starfighter_init()`
  ruft nach `intro_play()` einfach erneut `bgInit()` für BG2 auf und
  beansprucht die Ebene neu. Weil sie dediziert ist (nicht wie beim HUD mit
  einem Verlaufshintergrund geteilt), reicht ein einfaches `bgShow()`/
  `bgHide()` zum Ein-/Ausblenden, statt Pixel manuell zurückzusetzen. Die
  Palette liegt in einem reservierten Bereich von `BG_PALETTE` (Index
  100-227) — kollidiert mit nichts, da Gameplay-Sprites direktfarbige
  Bitmap-Sprites sind (kein Palettenindex) und die Bestenlisten-Konsole
  (`topscreen.c`) nur Bank 0 (Index 0-15) nutzt. Priorität 2 sorgt dafür,
  dass alle Gameplay-Sprites (Priorität 0-1) davor gezeichnet werden, das
  Modell also im Hintergrund sitzt statt Spielgeschehen zu verdecken.

  Zwei Anzeige-Modi (`StarfighterMode` in `starfighter.h`), je nach
  `GameState` in `main.c`s Game Loop gewählt: **Hintergrund** — klein
  (96x96, Originalgröße), oben rechts, während `STATE_PLAYING`/
  `STATE_SHIP_EXPLODING` — dezente Deko im laufenden Spiel, wird von den
  Gameplay-Sprites überdeckt. **Showcase** — dasselbe Frame per simplem
  Nearest-Neighbor auf 2x hochskaliert (192x192, füllt die Bildschirmhöhe
  exakt aus) und zentriert, während `STATE_TITLE` (direkt nach dem
  Intro-Video) — hier ist der obere Bildschirm sonst leer (die
  Titel-/Steuerungstexte liegen auf dem *unteren* Bildschirm, siehe
  `hud.c`s `drawTitleStatic()`), das Modell ist also der einzige Blickfang
  und bekommt entsprechend Raum. Bei allen anderen Zuständen (Bestenliste,
  Tastatur, Level-Ende, Game Over) bleibt sie ausgeblendet
  (`STARFIGHTER_HIDDEN`). Da sich die beiden Modi in ihrer Bildschirmfläche
  nur teilweise überlappen, löscht `starfighter_setMode()` bei jedem
  Moduswechsel die komplette BG2-Fläche einmal auf die Hintergrundfarbe
  (Index 0), um Bildreste vom jeweils anderen Modus zu vermeiden.

  (Ursprünglich war die Animation auf dem *unteren* Touchscreen als Teil der
  HUD-Bitmap-Ebene (BG3) geplant und implementiert — das Zeichnen dorthin
  wurde per Diagnose-Build nachweislich bestätigt (unkritischer Testfüllung
  + VRAM-Rücklesen), aber nicht das war gewünscht: der Nutzer wollte das
  Modell auf dem *oberen* Bildschirm. Daher der Wechsel auf eine eigene BG2-
  Ebene wie oben beschrieben.)
- **OAM-Budget:** Spieler (1) + Entities (10) + Projektile (6) + Boss (1) +
  Lebensanzeige (8) + Boss-Geschosse (8) = 34 Sprites gleichzeitig, weit
  unter dem Hardware-Limit von 128.
- **Game Loop:** klassisch `scanKeys()` → Update (Spieler/Schüsse/Entities
  oder Boss+Geschosse je nach Levelart/Level/Tastatur) → HUD/Top-Screen
  zeichnen → `oamSet(...)` je Sprite → `swiWaitForVBlank()` →
  `oamUpdate(&oamMain)`.

## Mögliche nächste Schritte

1. Eigene Pixelart-Verfeinerungen am Spritesheet (z. B. weitere
   Animationsframes).
2. Hintergrund-Tilemap fürs Spielfeld (z. B. mit
   [Tiled](https://www.mapeditor.org/)) statt Flächenfarbe.
3. Kurze Soundeffekte (Schuss, Explosion, Stern eingesammelt) über `maxmod`
   als geladene Samples (`AUDIO`-Verzeichnis in der Makefile ist dafür
   vorbereitet) — ergänzend zur Hintergrundmusik, die schon per Streaming
   läuft.
4. Lautstärkeregler fürs Menü (`mmSetModuleVolume` wirkt nur auf
   Modul-/Jingle-Wiedergabe, nicht auf Streams — bräuchte ggf. eine
   Vorverstärkung/Skalierung direkt in `musicStreamCallback`).
5. D-Pad-Alternative zur Touch-Tastatur für reine Tastatursteuerung ohne
   Stylus/Maus.
6. Aufbau als kleine ECS-Struktur, sobald mehr Objekttypen dazukommen.
7. Die "STERNE X/Y"-Anzeige im HUD (`hud.c`) bleibt während eines
   Boss-Kampfs unverändert stehen (0/Bedarf), da dort ja keine Sterne
   gesammelt werden — kosmetisch unpassend, aber harmlos. Könnte durch eine
   Boss-HP-Anzeige im HUD ersetzt werden, wenn gewünscht.
