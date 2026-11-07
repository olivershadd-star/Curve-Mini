# Reprise Curve Mini — 7 novembre 2026

Sources importées du ZIP ANKS_Curve_Pilot_Prototype_0_1_Sources_Windows.zip.
Prototype confirmé : instrument de contrôle silencieux, sortie MIDI système,
aucun hébergement audio. Aucun ancien projet remplacé.

## Changements

- Nom visible, produit VST3, documentation et artefact : Curve Mini Prototype 0.1.
- Cible CMake CurvePilotMidi01, code Cpm1 et XML CURVE_PILOT_MIDI_01 conservés.
  Les chemins d'artefacts restent donc CurvePilotMidi01_artefacts ; le nom du
  dossier VST3 devient Curve Mini Prototype 0.1.vst3.
- Restauration : désarmement et fermeture du port précédent. Connecter reste
  nécessaire avant ENVOI, même lorsqu'un état est chargé dans une instance active.
- Transport : aucune émission automatique si Live ne publie pas de position PPQ
  finie. Auparavant, une ancienne position pouvait être réutilisée.
- Accès à armed, selectedPort et standaloneTempo depuis l'éditeur protégés par
  le verrou déjà utilisé pour la restauration, hors callback audio.
- Calcul de phase corrigé pour les mesures de moins d'une noire (ex. 1/8).
- Vérification de l'indice avant l'émission manuelle Assigner.

## Vérification locale

Commande exécutée avec succès :

```sh
g++ -std=c++17 -Wall -Wextra -pedantic tests/midi_tests.cpp -o /tmp/curve-mini-midi-tests
/tmp/curve-mini-midi-tests
```

Cas supplémentaires : mesure 1/8, retour de position de boucle, âge NaN.
CMake indisponible dans cet environnement : aucun build complet effectué.
Aucune nouvelle vérification de compilation JUCE ni intégration MIDI réelle.
Les corrections de port, restauration et transport devront être vérifiées sous
Windows ; les tests de modèle seuls ne les valident pas.

## Examen de fiabilité

Le timer appartient au processeur et continue donc sans éditeur. Le callback
ne prend pas le verrou control et n'appelle pas le pilote MIDI. Le timer refuse
une publication en cours et un transport arrêté ou périmé. Les positions de Live
sont relues à chaque publication, y compris après saut de boucle. Ces constats
proviennent du code ; leur comportement dans Live reste à vérifier.
Un port débranché peut rester représenté par un objet MidiOutput ; sendMessageNow
ne fournit pas d'accusé de réception. Le statut « connecté » ne prouve donc pas
la réception réelle. Recréer le port puis Actualiser/Connecter si nécessaire.

## Suite

Créer un dépôt GitHub distinct, importer ces sources, puis lancer le workflow
manuel Curve Mini Prototype 0.1 Windows. Le workflow compile les tests puis le
VST3 et l'autonome sous Windows x64. Aucun workflow n'a encore été déclenché.

Essai utilisateur après compilation : ligne 1 -> macro 1, puis ligne 2 -> macro 2,
arrêt/reprise, saut de boucle, fenêtre fermée, sauvegarde et réouverture avec
reconnexion explicite. Comparer la charge au même set sans Mini. Voir README_FR.md.
