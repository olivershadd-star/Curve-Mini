# Curve Mini — reprise depuis Curve Pilot V2.1 R2

Cette base repart de l'archive ANKS_Curve_Pilot_V2_1_VST3_et_Autonome_Sources.zip
fournie par l'utilisateur. Les précédentes tentatives Mini ne servent pas de base.

## Ce qui vient du programme d'origine

Curve.h et MotionLanguage.h sont repris sans modification. Le thème, les couleurs
par ligne, le dessin libre et l'éditeur de points sont ceux de la V2.1. Les outils
copier/coller, annulation, verrouillage, inversion, variation, lissage du dessin,
phase, profondeur, minimum/maximum, longueur et description locale sont conservés.
Les paramètres de lecture originaux gardent leurs identifiants laneN_on/min/max/
bars/offset/smooth/depth. L'annulation ne restaure plus une ancienne destination.

## Interface native du plugin

A gauche, les douze lignes, leurs noms et leurs destinations. Au centre, le
même éditeur de courbes agrandi : dessin, points, formes et copier/coller. Les
réglages de lecture courants restent visibles. Phase, profondeur, lissage,
variation et outils avancés se déplient dans le même panneau, avec défilement.
A droite, destination, commande manuelle et transfert vers un clip. Ce n'est
pas une page web intégrée : l'interface est en C++/JUCE.

## Ce qui est retiré

Chargement et scan des plugins, rack d'instrument/effets, traitement tiers par
sous-blocs de 32 échantillons, compensation dry/wet, buffers de délai, éditeurs
VST enfants et son de test. Mini n'héberge aucun plugin et ne calcule aucun son.
Le produit désactive JUCE_PLUGINHOST_VST3. Seul l'hôte indépendant des tests
active ce module pour vérifier le fichier VST3 livré.

La déclaration audio stéréo d'entrée/sortie et l'entrée MIDI de la V2.1 sont
conservées pour la compatibilité hôte. L'audio et les notes sont effacés ; placer
Mini sur sa piste de contrôle, pas sur le trajet audio d'un instrument.
Le callback publie uniquement le transport par atomiques. Courbes et émissions
CC sont évaluées par un timer du processeur à 100 Hz maximum, hors callback.
Le lissage temporel original est adapté à cette fréquence de contrôle. Fenêtre
fermée, le timer continue. Aucun bénéfice CPU chiffré n'est promis sans mesure.

## Macros Ableton

Choisir une ligne, nommer sa destination et sélectionner Macro / paramètre
Ableton. Créer un port loopMIDI, le sélectionner comme sortie de cette ligne.
Dans Live : entrée du port Remote ON, Track OFF, Sync OFF ; sortie tout OFF.
Connecter dans Mini. Ctrl+M dans Live : sélectionner une macro, puis Assigner
dans Live dans Mini. Seul le CC de cette ligne est envoyé à 64, ENVOI est coupé.
Quitter Ctrl+M, activer la ligne et ENVOI. La courbe suit la lecture de Live.
Le canal par défaut est 16, CC20 à 31, pour distinguer ces adresses des
contrôleurs physiques courants sur canal1. Eviter les collisions d'assignation.
Max for Live n'est pas requis ; instruments et effets restent dans Ableton.

## Machines physiques USB

Choisir Machine MIDI, sortie, canal et CC par ligne. Exemple : ligne1 canal1/CC70,
ligne2 canal1/CC71, ligne3 canal2/CC70, ligne4 canal3/CC73. Les canaux doivent
correspondre aux parties de la machine. Plusieurs machines peuvent être choisies.
Connecter ouvre une seule fois chaque port. Si Windows refuse le partage de la
sortie USB déjà ouverte par Live, passer par loopMIDI et une piste relais Live :
entrée Track ON / Remote OFF, Monitor In, sortie vers la machine au bon canal.
Les pistes de notes existantes restent indépendantes. Deux lignes actives ne
peuvent pas partager le même port/canal/CC : ENVOI s'arrête pour éviter le conflit.

## Potards MIDI externes

Dans Live, activer Remote pour l'entrée du contrôleur physique. Configure sur
Mini : déplacer son curseur manuel, puis quitter Configure. Ctrl+M : sélectionner
le paramètre Ligne N Valeur MIDI, tourner le potard physique, quitter Ctrl+M.
Sa valeur prend la main sur la courbe de cette seule ligne. Reprendre la courbe
rend la main au dessin. Le manuel est absolu 0-127 et fonctionne à l'arrêt,
avec ligne active et ENVOI ON. ENVOI OFF coupe courbes et manuel. Les gestes ne
sont pas redessinés dans la courbe ; ils sont automatisables dans Live.

## Courbe vers clip

Exporter courbe MIDI écrit un tour en SMF sans notes, avec canal/CC,
minimum/maximum, profondeur, phase et lissage (calculé au tempo d'export).
Importer le .mid dans un clip MIDI vide de Live, sélectionner MIDI Ctrl et le CC,
puis régler le canal/sortie de la piste. Désactiver l'émission Mini correspondante.
Pas de collage direct propriétaire dans Live ni de fusion automatique avec des
notes existantes. La durée utilise la dernière signature publiée par Live, ou
4/4 en autonome. CC0/32 et CC120-127 ont des rôles particuliers dans MIDI/Live.
L'import réel dans Live reste à essayer sur le poste utilisateur.

## Sauvegarde et reprise des courbes V2.1

Exporter/Importer session conserve courbes, points, textes, paramètres et
routages. A la restauration, sorties fermées, ENVOI OFF ; reconnecter explicitement.
L'import d'un fichier .curvepilot V2.1 récupère les courbes et leurs réglages,
sans charger les instruments/effets ni conserver leurs correspondances internes.
Les lignes importées sont désactivées : choisir leurs nouvelles destinations.
Les états des précédentes tentatives Mini rejetées ne sont pas la référence.
Le code plugin Cpm1 distingue Mini de Curve Pilot Cvp2 ; éviter les doublons de
Mini installés sous plusieurs noms. Nom du nouveau bundle : Curve Mini.vst3.

## Vérification

Tests locaux des courbes et du langage local : réussis. Le workflow Windows
vérifie le processeur, l'interface native, la sauvegarde, le MIDI exporté et le
chargement du VST3 réel par un hôte indépendant. Un PNG de l'interface C++ est
produit par les tests. Le résultat dans Live et la charge doivent être mesurés
sur le poste utilisateur. Les anciens documents V2.1 sont archivés sous docs/.
