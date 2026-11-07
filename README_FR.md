# Curve Mini Prototype 0.1 — nouvelle base MIDI

Ce projet est distinct des essais Curve Pilot V1/V2/V2.1. Ne pas remplacer le dépôt de ces essais. Nouveau code de contrôle, nouvelle identité VST3 Cpm1, nouveau format de sauvegarde. Les anciennes sessions ne sont pas importées.

## Ce que fait ce prototype

12 lignes indépendantes : activation, six formes, dessin à main levée, minimum, maximum, longueur de 0,25 à 32 mesures. Chaque ligne émet une commande fixe : ligne 1 = CC20, ligne 2 = CC21, jusqu'à ligne 12 = CC31, canal MIDI 1. Le bouton Assigner envoie seulement la commande de sa ligne et arrête toutes les courbes, pour éviter d'apprendre la mauvaise commande.

Aucun synthé ni effet hébergé. Aucun chemin audio traité. La version VST3 apparaît comme un instrument de contrôle silencieux dans Ableton : elle occupe SA piste MIDI séparée. Elle ne se place PAS avant Serum sur la même piste comme un effet MIDI natif Ableton. Elle lit la position et le tempo du morceau ; le MIDI sort directement vers le port Windows choisi. Sa sortie audio reste silencieuse.

## Compilation Windows — méthode habituelle

1. Créer un dépôt distinct, par exemple curve-mini.
2. Dézipper ce paquet. Envoyer le CONTENU du dossier dézippé à la racine du dépôt, avec les dossiers Source et tests. Le fichier CMakeLists.txt doit être directement visible.
3. Si .github/workflows/build.yml n'a pas été envoyé, créer ce fichier dans GitHub et y coller exactement le contenu de WORKFLOW_A_COPIER.yml.
4. Dans Actions, lancer « Curve Mini Prototype 0.1 Windows » sur main.
5. Télécharger l'artefact Curve-Mini-Prototype-0.1-Windows-x64 et le dézipper.
6. Ableton fermé, copier le dossier COMPLET « Curve Mini Prototype 0.1.vst3 » dans C:\Program Files\Common Files\VST3. Ne pas copier seulement le fichier binaire interne.

Le paquet contient aussi une application autonome .exe. Pour le premier essai dans Ableton, utiliser le VST3 : il suit le transport de Live. L'autonome a une horloge propre et NE se synchronise PAS à Ableton dans ce prototype.

## Préparation MIDI (une seule fois)

Sous Windows, créer un port virtuel avec loopMIDI : https://www.tobias-erichsen.de/software/loopmidi.html . Nommer le port « CurveMini Test ». Le laisser ouvert. Ableton référence loopMIDI dans sa documentation officielle : https://help.ableton.com/hc/en-us/articles/209774225-Setting-up-a-virtual-MIDI-bus .

Dans les réglages MIDI d'Ableton, pour l'ENTRÉE CurveMini Test : Remote/Télécommande ON, Track/Piste OFF, Sync OFF. Pour sa SORTIE dans Ableton : tout OFF. Curve ouvre elle-même la sortie Windows ; ne pas créer de renvoi MIDI de Live vers ce port. Ne pas changer les réglages de ton clavier.

Si le port n'apparaît pas, créer le port avant de démarrer Ableton et cliquer Actualiser dans Curve. Il faut ensuite sélectionner CurveMini Test et cliquer Connecter. Un port créé après l'ouverture peut nécessiter de relancer Live.

## Premier test : une seule macro

1. Charger Serum directement dans Ableton sur sa piste habituelle. Jouer un clip MIDI simple et préparer une macro de rack qui contrôle par exemple son filtre.
2. Créer une AUTRE piste MIDI et charger Curve Mini Prototype 0.1. Le synthé et les effets restent sur la première piste.
3. Dans Curve, sélectionner CurveMini Test puis Connecter. Laisser ENVOI OFF.
4. Activer le mode MIDI d'Ableton (Ctrl+M), cliquer sur la macro à contrôler, puis cliquer Assigner sur la ligne 1 de Curve. Live doit apprendre canal 1 / CC20. Sortir du mode MIDI (Ctrl+M).
5. Cocher Active sur la ligne 1, choisir Vague, longueur 4 mesures. Cliquer ENVOI : il passe sur ON. Lancer la lecture d'Ableton. La macro doit bouger.
6. Pour une autre macro : arrêter ENVOI, retourner en mode MIDI, sélectionner la macro suivante, cliquer Assigner sur la ligne 2 (CC21), sortir du mode MIDI, puis réactiver ENVOI.
7. Sauvegarder le set Ableton : Live conserve les correspondances MIDI et Curve conserve les dessins/réglages. À la réouverture, l'envoi reste désarmé : vérifier le port, Connecter, puis ENVOI.

Assigner n'est PAS une détection magique des paramètres de Live : c'est l'équivalent du mouvement d'un bouton de contrôleur physique. Le bouton envoie 64/127, donc la macro sélectionnée peut se déplacer à mi-course.

En mode autonome : choisir le même port, Connecter, régler le BPM, puis ENVOI. Live reçoit les commandes, mais les deux horloges sont indépendantes.

## Limites assumées du prototype

- MIDI CC standard 7 bits : 128 valeurs, pas une automation audio à résolution continue. Certains paramètres sensibles peuvent révéler des marches. Ce test sert d'abord à valider la liaison et la charge.
- Émission au maximum à 100 Hz, uniquement lorsque la valeur quantifiée change. Le planificateur tourne hors du fil audio. Son timing dépend de Windows et peut varier si l'interface est occupée ; pas de promesse de précision à l'échantillon.
- Pas de rack, pas d'effets, pas de traducteur de texte, pas de courbes de tempo, pas de suivi automatique des noms de macros. Ces fonctions ne sont pas nécessaires pour valider ce trajet MIDI.
- Les messages ne sont envoyés que pendant la lecture de Live. Si les informations de transport cessent d'arriver pendant 250 ms, l'émission s'arrête.
- Chaque instance utilise CC20..31 : ne pas lancer deux instances sur le même port pour ce premier essai.
- La latence de commande comprend le timer, le port virtuel et le traitement Remote d'Ableton.

## Validation effectuée

Tests du modèle MIDI exécutés : adresses CC, activation par défaut OFF, conversion 0..127, inversion, phase musicale, détection de transport périmé et bornes des six formes. Vérification de syntaxe C++ de Plugin.cpp et Editor.cpp avec JUCE 8.0.6 : réussie.

Compilation/link Windows, installation VST3, sortie loopMIDI, assignation Remote dans Live Lite, sauvegarde/rechargement réel et consommation avec Serum : PAS encore validés sur Windows. Le workflow fourni compile Windows et exécute les tests du modèle ; il ne remplace pas ces essais de terrain.

## Reprise du 7 novembre 2026

Voir REPRISE.md pour les corrections et les limites de validation de cette reprise.
Le code plugin Cpm1 et le tag XML CURVE_PILOT_MIDI_01 du prototype sont conservés.
