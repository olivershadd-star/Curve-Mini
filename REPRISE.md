# Curve Mini — évolution du 10 novembre 2026

## Périmètre réalisé

- CC 0–127, canal 1–16, sortie globale ou distincte par ligne. Ports mutualisés.
- Protection contre deux lignes actives sur la même adresse complète.
- 12 paramètres hôte stables lane1_value à lane12_value, valeurs entières 0–127.
  Assignation MIDI et automation assurées par Live, sans Max for Live.
- Le mouvement d'un paramètre prend la main sur sa seule ligne. Retour explicite
  à COURBE. Manuel à l'arrêt, courbes en lecture uniquement, ENVOI OFF coupe les deux.
- Export d'un tour de dessin en SMF0, sans notes ni tempo, longueur exacte,
  signature rythmique publiée par Live, minimum/maximum et inversion appliqués.
- Migration des sauvegardes 0.1 : CC20–31/canal1, courbes inchangées. Code Cpm1,
  cible CurvePilotMidi01 et XML CURVE_PILOT_MIDI_01 conservés.
- Version produit 0.2.0, nom visible et artefact 0.2 ; chemins CMake cohérents.
- Build Windows automatique après changement du code sur main, manuel toujours possible.

## Validation

Le test C++17 local du modèle et des exports réussit. Il relit les fichiers MIDI
avec un décodeur indépendant, vérifie six formes, inversion, valeurs, adresses,
signature 7/8, durée exacte des boucles constantes et courtes, absence de notes,
conflits par port/canal/CC et rejets des données invalides.

Tests d'intégration ajoutés au workflow Windows : paramètres réels APVTS,
prise de main par ligne, sauvegarde avant consommation du mouvement, restauration
des modes et valeurs, réouverture de l'éditeur, anciens états et entrées hors
bornes, transport manquant / saut de boucle, relecture du fichier MIDI par JUCE.
La compilation et ces tests Windows sont à confirmer par le build de ce commit.

Aucun essai utilisateur de cette évolution dans Live ni sur groovebox n'a encore
été fait. L'import des CC est documenté par Ableton, mais doit être essayé avec
ce fichier. Le partage d'une sortie USB dépend du pilote ; le README décrit le
relais loopMIDI/Live si l'ouverture directe n'est pas possible.

## Premier parcours utilisateur

Installer 0.2 en remplacement de 0.1. Une ligne CC70/canal1 -> machine, tester
potard souris à l'arrêt puis courbe en lecture, puis assignation du contrôleur
physique via Configure/Ctrl+M. Ajouter une seconde adresse. Tester fenêtre fermée,
ENVOI OFF, arrêt/reprise, réouverture avec reconnexion. Exporter une ligne, importer
sur un clip vide, choisir MIDI Ctrl et son CC. Comparer la charge au set sans Mini.

Le détail de la reprise initiale est dans docs/REPRISE_2026-11-07.md.
