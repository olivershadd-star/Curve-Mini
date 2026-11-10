# ANKS Curve Pilot 2 — V2.1 reprise R2 (5 novembre 2026)

Cette livraison remplace la première V2.1 rejetée. Repère visible : **V2.1 R2 / 3 PAGES**. Retélécharger le ZIP avant de remplacer les fichiers GitHub.
Prototype Windows x64 : VST3 + application autonome. Un instrument VST3, huit effets en série, douze courbes. Sources, pas des exécutables déjà compilés.

## Même méthode GitHub que précédemment
1. Garder le ZIP V2 et son VST3 comme sauvegarde.
2. Extraire ce nouveau ZIP dans Windows (Extraire tout).
3. Dans le dépôt du projet, onglet Code, Add file > Upload files : déposer le CONTENU du dossier extrait, notamment CMakeLists.txt, Source et tests. Ne pas téléverser le ZIP seul et ne pas ajouter un dossier parent autour de ces fichiers.
4. Valider avec Commit changes.
5. Ouvrir `.github/workflows/build.yml` : remplacer son texte par celui de WORKFLOW_A_COPIER.yml et valider. Si ce fichier n'existe pas, Actions > set up a workflow yourself : coller le même texte et valider. Un seul workflow build.yml suffit.
6. Actions > Windows VST3 et Autonome V2.1 corrigee > Run workflow. Lancer une nouvelle exécution pour le dernier commit.
7. Après succès, ouvrir le résumé et télécharger l'artefact ANKS-Curve-Pilot-V2-1-R2-Windows-x64.

La racine du dépôt doit contenir CMakeLists.txt, Source/, tests/ et .github/workflows/build.yml. README seul ou workflow seul ne suffit pas.

## Deux sorties de compilation
L'artefact extrait contient :
- VST3/ANKS Curve Pilot 2.vst3 : copier le DOSSIER .vst3 complet dans C:\Program Files\Common Files\VST3, Ableton fermé.
- Standalone/ANKS Curve Pilot 2.exe : lancer depuis Windows, sans Ableton. Garder le dossier extrait complet.
Les cibles CMake restent CurvePilotV2_VST3 et CurvePilotV2_Standalone, même si la version est 2.1.
Cette mise à jour garde l'identité de la V2 : elle la remplace dans Live. La V1 conserve son identité distincte.

## Trois vues
- **Synthe** : synthé au centre, douze indicateurs de courbes à gauche, bande d'effets à droite avec On/Off et Dry/Wet. Cliquer un numéro ouvre ses courbes ; cliquer le nom d'un effet ouvre son édition. Les bandes défilent si l'écran manque de hauteur.
- **Courbes** : douze cartes défilantes à gauche, appareil sélectionné à droite. Dernier potard, recherche de paramètres par appareil, points, dessin libre, copie, collage, lissage et Code sont conservés.
- **Effets** : chaîne audio à gauche, interface de l'effet ou du synthé à droite. Le son parcourt la liste du haut vers le bas. Avant / Après déplace l'effet ; son numéro FX reste son identité stable et ses assignations le suivent. Les emplacements vides ne traitent pas le son.
Les trois boutons Synthe / Courbes / Effets changent de PAGE DE TRAVAIL. Le zoom ne change jamais de page. Le bouton Instrument sélectionne le synthé dans la page actuelle. Page appareil ouvre une page de travail supplémentaire centrée sur l'appareil sélectionné, sans courbes ni cases FX ; Retour aux courbes rejoint la page précédente dans ce contexte. Aucun rechargement du VST pour changer de page. Si l'appareil dépasse même cette page, ses bords droit et bas doivent être accessibles par les deux barres de défilement.
Sous Windows, une fenêtre enfant native dédiée contient la surface du VST et ses barres de défilement : elle limite le dessin natif du VST à cette seule zone. Un conteneur mesure la taille réellement mise à l'échelle pour le défilement. Vérification avec Serum et Resonant Lab sous Windows toujours nécessaire.

## Décrire les mouvements — traducteur local
Sur une ligne : **Decrire / Code**. Le mode par défaut est **Decrire le mouvement (local)**. Décrire les gestes, cliquer Previsualiser, lire l'interprétation, puis Appliquer. Les phrases inconnues sont signalées et ne remplacent pas la courbe.
Exemple : « Ouverture sèche haute, fermeture de trois quarts, va-et-vient rapide très serré, puis réouverture complète et fermeture lente. »
La durée totale vient de Mesures de la ligne ; le départ est son premier point actuel. Pour des durées précises, écrire « sur 2 temps » ou « pendant 1 mesure ». Un temps vaut une noire et une mesure vaut 4 temps dans ce traducteur initial.
Les durées non précisées se partagent le temps restant : une rampe lente reçoit plus de temps qu'une rapide. Pour les oscillations, rapide/lent choisit 8/2 cycles (4 normalement, 16 avec très rapide) ; on peut écrire un nombre explicite de cycles.
Vocabulaire et exemples détaillés : MOUVEMENTS_FR.md. Le code avancé existant reste accessible dans le sélecteur ; le traducteur génère ce même langage déclaratif, pas du Python. Pas de réseau, de modèle téléchargé, de clé API ou de facturation pour traduire. Le texte appliqué est enregistré dans la session de la ligne.

## Autonome
Le menu **Options > Audio/MIDI Settings...** de la fenêtre permet de choisir sortie audio, fréquence d'échantillonnage, tampon et entrées MIDI. Les intitulés de ce menu JUCE peuvent rester en anglais.
Le build standard utilise les pilotes Windows disponibles via JUCE, notamment Windows Audio / WASAPI. ASIO n'est pas inclus par défaut : il exige le SDK Steinberg séparé. La variable CMake optionnelle ANKS_ASIO_SDK_PATH permet de compiler avec un SDK installé ; le workflow standard n'en télécharge pas.
Lecture démarre l'horloge des courbes ; Pause garde sa position ; Stop / début revient au début. Tempo 20–300 BPM, signature 4/4 pour cette base. Les instruments hébergés reçoivent cette position et ce tempo.
Les notes MIDI sont transmises à l'instrument même si le transport est arrêté. Choisir une entrée MIDI dans Options si l'on veut utiliser un clavier physique. Aucun automapping Akai spécifique ajouté.
Lecture ne crée pas de notes : utiliser le clavier du synthé, un contrôleur ou le séquenceur propre à l'instrument. Le séquenceur de Resonant Lab reste du ressort de cet instrument.
La fenêtre autonome possède son cadre Windows et ses commandes système. Ajuster écran utilise le plein écran de cette fenêtre ; Taille normale en ressort. En VST3, le cadre reste géré par Ableton et Ajuster écran demande une taille cliente adaptée.

## Projets communs
Exporter / Importer utilise le même fichier .curvepilot en autonome et en VST3 : états de l'instrument et des effets, courbes, assignations, ordre des effets, Dry/Wet et tempo autonome. Les plugins tiers doivent être installés et autorisés sur le PC de destination.
L'import restaure le rack mais ne lance pas automatiquement le transport autonome.
Importer une session V2 doit préserver ses quatre premiers effets ; les quatre nouveaux emplacements restent libres. À vérifier sur une copie du projet, pas sur l'unique sauvegarde.
Les préférences audio/MIDI sont propres à l'application autonome, pas au fichier de rack.

## Limites de cette base
- Un instrument, pas plusieurs. Huit emplacements FX, pas huit effets chargés d'office.
- Un éditeur VST visible à la fois. L'intégration et le zoom dépendent aussi du plugin tiers ; défilement en secours.
- VST3 stéréo ; pas d'hébergement des appareils natifs Ableton.
- Pas d'enregistrement WAV, pas de séquenceur global, pas d'automation écrite dans les clips Ableton, pas d'intégration IA en ligne.
- Le bypass laisse l'effet calculer pour garder son état et sa compensation de latence ; ce n'est pas un bouton d'économie CPU.
- Le changement d'ordre d'effets peut produire une transition sonore : l'historique du tampon Dry est remis à zéro. Cette première base ne garantit pas un réordonnancement inaudible pendant une prise.
- Sauvegarde du rack par Exporter ; conserver une copie avant mise à jour. La stabilité audio et les changements de taille doivent encore être essayés sous Windows.
