# Feuille de test dans Live — Curve Mini 0.6 (environ 10 minutes)

Cocher OK / KO et noter ce qui se passe. Si un point est KO, s'arrêter là et le décrire :
les points suivants en dépendent souvent.

## Préparation
1. Lancer le workflow GitHub (Actions > « Curve Mini Precision MIDI Windows » > Run workflow, ou push sur la branche).
2. Télécharger l'artefact, copier `Curve Mini.vst3` dans `C:\Program Files\Common Files\VST3`, rescanner dans Live.
3. loopMIDI : créer un port (ex. « Curve Mini »). Live > Préférences > MIDI : entrée du port
   Remote ON, Track OFF, Sync OFF ; sortie du port tout OFF.

## Tests
| # | À faire | Résultat attendu | OK/KO |
|---|---------|------------------|-------|
| 1 | Charger Curve Mini sur une piste MIDI | La fenêtre s'ouvre, 8 onglets, aucun plantage ; éditeur affiché « ANKS Labs » dans le navigateur | |
| 2 | Configure dans l'en-tête du plugin Live | Les paramètres « Lane N … » apparaissent, jusqu'à la ligne 8 (64 au total) | |
| 3 | Ligne 1 : destination « Sortie MIDI vers Ableton », Active, ENVOI ON ; 2e piste avec Serum, MIDI From = piste Curve Mini ; enregistrer 4 mesures dans un clip | Une enveloppe CC20 (canal 16) qui suit la courbe apparaît dans le clip | |
| 4 | Chemin loopMIDI : ligne 1 sortie loopMIDI, Connecter, Assigner ; Ctrl+M dans Live sur une macro de Serum ; quitter Ctrl+M ; Active + ENVOI ON | La macro suit la courbe pendant la lecture | |
| 5 | Pendant la lecture : déplacer la fenêtre, ouvrir « Session », redimensionner | Le mouvement de la macro reste régulier (pas de saccades) — c'est le but du thread d'envoi | |
| 6 | Lignes 5 à 8 : CC 24 à 27, Active ; assigner chacune à une macro différente | Les 8 macros bougent indépendamment, sans conflit de CC | |
| 7 | Verrouiller la ligne 1, tirer/modifier les autres ; tourner un potard MIDI relié à « Ligne N Valeur MIDI » | La ligne verrouillée garde sa forme ; le potard prend la main puis « Reprendre la courbe » la rend | |
| 8 | Exporter courbe MIDI, importer le .mid dans un clip vide, MIDI Ctrl = CC choisi | La courbe apparaît en enveloppe de clip, la boucle se raccorde sans saut | |
| 9 | Enregistrer le set, le rouvrir | Les 8 lignes sont restaurées ; ENVOI OFF ; reconnecter puis ENVOI ON refonctionne | |
| 10 | 8 lignes actives, ENVOI ON, observer le compteur de charge de Live | Noter le pourcentage (plugin seul, sans autre piste) | |

## À rapporter
- Les numéros KO et le message affiché en bas du plugin.
- Le pourcentage de charge du test 10.
- Tout comportement étrange (CC en retard, saut de valeur au premier envoi, macro qui ne répond pas).
