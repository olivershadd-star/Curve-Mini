# Diagnostic du refus de chargement — 10 novembre 2026

Signalement : Mini apparaît dans le navigateur VST d'Ableton, mais Live refuse
son chargement. Le dossier est correct selon l'utilisateur. Aucun message exact
ni journal du poste utilisateur n'est disponible ; ne pas affirmer avoir reproduit
le refus spécifique de Live à partir d'un test d'un autre hôte.

## Défauts et corrections ciblés

La version précédente déclarait IS_SYNTH TRUE mais NEEDS_MIDI_INPUT FALSE et
acceptsMidi() false : un instrument sans bus d'événements entrant. Le correctif
expose un bus MIDI standard et la catégorie Instrument/Synth, tout en consommant
les événements sans les jouer ni les retransmettre. La sortie audio reste
silencieuse. C'est une correction du contrat d'instrument, dont l'effet dans
Live sur le poste de l'utilisateur reste à confirmer.

La compilation MSVC utilisait par défaut le runtime partagé. Le correctif
sélectionne /MT (/MTd en Debug) pour intégrer le runtime, puis vérifie qu'aucune
dépendance directe MSVCP/VCRUNTIME/CONCRT ne reste dans le module livré. Cela
retire une dépendance possible, sans prouver qu'elle manquait sur le poste.

Des problèmes de bus de plugins MIDI dans Live sont rapportés chez JUCE :
https://forum.juce.com/t/vst3-midi-plugins-wont-load-in-ableton-live/36323
La configuration d'instrument illustrée par le SDK VST3 contient une entrée
d'événements et une sortie audio stéréo :
https://steinbergmedia.github.io/vst3_doc/vstexamples/index.html
Ces références guident la correction, elles ne prouvent pas le diagnostic Live 12.

## Validation ajoutée

Les tests précédents liaient le code partagé du plugin : ils ne vérifiaient pas
la DLL VST3 réellement distribuée. Le nouveau programme vst3_load_tests est un
hôte JUCE indépendant du code Mini. Il charge le bundle livré via la véritable
interface VST3 et vérifie : découverte de factory, création d'instrument, bus
MIDI entrant, paramètres, attachement de la fenêtre à un HWND natif, détachement
et réouverture, traitement silencieux, sauvegarde et recréation avec restauration.
Le test a un délai limite de 90 secondes.

Un script Windows vérifie le bundle, l'architecture PE x64, GetPluginFactory,
InitDll, ExitDll et les dépendances. Le workflow télécharge aussi l'ancien
artefact pour une comparaison de dépendances et de chargement, sans en faire
une condition de disponibilité du correctif.

Les résultats de compilation et de chargement sont à renseigner après le build.
Le résultat dans Ableton n'est pas encore connu.

## Installation

Même identité Cpm1, mêmes paramètres et sauvegardes. Version 0.2.1 ; le nom de
produit et le dossier VST3 restent Curve Mini Prototype 0.2 pour ne pas multiplier
les noms et chemins. La fenêtre indique 0.2.1. Remplacer le dossier complet de
la version précédente, Live fermé.
