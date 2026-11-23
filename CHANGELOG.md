# Reprise V2.1 — Curve Mini 0.3

Nouvelle base à partir de l'archive originale, sans réutilisation du produit Mini
rejeté. Moteur de courbes, langage local et éditeur V2.1 conservés. Hébergement
audio retiré ; destinations macros Ableton / MIDI matériel ajoutées. Interface
native structurée, contrôle externe via paramètres hôte, export MIDI de la ligne.
Compilation et chargement Windows à vérifier par le workflow de cette branche.

# Quatre courbes — Live Lite — 0.4

Périmètre demandé le 11 novembre : quatre courbes, une fenêtre extérieure
compacte qui laisse de la place à droite. Navigation par quatre aperçus, édition,
destinations et traducteur dans une seule fenêtre. Plus de dialogue Decrire/Code.
Ajustement en unités logiques et placement prudent de la fenêtre de plugin seule.
Paramètres stables des quatre premières lignes ; 32 paramètres hôte au total.

# 0.6 — huit lignes et envoi MIDI stabilisé

- Huit lignes au lieu de quatre (les huit macros d'un synthé comme Serum) : 64 paramètres hôte,
  identifiants des lignes 1 à 4 inchangés, lignes 5 à 8 en canal 16 / CC 24 à 27, inactives.
  Une session à quatre lignes se recharge (test ajouté). Onglets étroits : numéro + aperçu.
- Éditeur de plugin « ANKS Labs » (au lieu de « ANKS ») pour le regroupement dans Ableton.

- Envoi MIDI vers les ports (loopMIDI / matériel) déplacé du thread d'interface vers
  un thread haute résolution dédié (`juce::HighResolutionTimer`, 10 ms). Les messages
  sont préparés sous verrou puis envoyés après l'avoir relâché.
- Formule courbe -> CC et lissage factorisés dans `Source/LaneMath.h` (sans JUCE), utilisés
  par `processBlock`, l'émetteur et l'export MIDI. Nouveau test `lane_math_tests`.
- `processBlock` ne construit plus de `String` pour lire les paramètres (pointeurs mis en cache).
- `Curve::at()` : correction d'une lecture hors tableau (`y[2049]`) pour une phase qui
  retombe exactement sur 1.0 (ex. -1e-18) ; phases non finies protégées. Test ajouté.
- `sending` devient atomique ; `isManual()` prend le verrou ; sorties MIDI en `shared_ptr`.
- Vérifié sous Linux (JUCE 8.0.6, sans ALSA/GL, écran virtuel) : compilation du plugin et
  de l'éditeur, tests des courbes, du langage, de LaneMath et du processeur (dont l'éditeur
  à 8 onglets et l'image de l'interface). Non vérifié : MSVC, VST3 chargé par Live, CC réels.
- Voir FEUILLE_TEST_LIVE_FR.md pour la vérification dans Live (environ 10 minutes).
