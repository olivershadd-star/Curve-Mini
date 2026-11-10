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
InitDll, ExitDll et les dépendances. Une comparaison ponctuelle avec l’ancien artefact a également été tentée
dans le build de diagnostic. Elle a échoué car son jeton ne disposait pas de
l’accès Actions nécessaire (Resource not accessible by integration). Aucun
résultat de chargement de cet ancien binaire n’en est donc déduit. Ces étapes
ponctuelles ont ensuite été retirées du workflow permanent.

## Résultat Windows établi

Build réussi sur le commit 3aa200628613d146e2e0ae726c87961fabc3e312 :
https://github.com/olivershadd-star/Curve-Mini/actions/runs/42870573329

- midi_model : réussi.
- plugin_state : réussi.
- vst3_load : réussi (0,70 seconde), module réellement chargé, fenêtre native
  attachée/détachée trois fois, traitement silencieux et rechargement de l'état.
- Inspection du module x64, des exports VST3 et des dépendances : réussie ;
  aucun MSVCP/VCRUNTIME/CONCRT externe.
- Artefact publié : Curve-Mini-Prototype-0.2.1-Windows-x64, ID 13789662496.

Le résultat dans Ableton sur le poste utilisateur n'est pas encore connu.
La comparaison des sources de Midi Editor et Resonant Lab a ensuite confirmé
qu'ils déclaraient tous deux NEEDS_MIDI_INPUT TRUE et acceptsMidi() true,
contrairement à Mini 0.2 avant correction. Cette différence est établie ; le
refus spécifique de Live n'a pas été reproduit sur le runner Windows.

Le nettoyage ultérieur du workflow retire uniquement les étapes comparatives
ponctuelles ; il ne change pas le code ni le VST3 testé. Aucun rebuild n'est
nécessaire pour ce nettoyage (commit marqué [skip ci]).

## Installation

Même identité Cpm1, mêmes paramètres et sauvegardes. Version 0.2.1 ; le nom de
produit et le dossier VST3 restent Curve Mini Prototype 0.2 pour ne pas multiplier
les noms et chemins. La fenêtre indique 0.2.1. Remplacer le dossier complet de
la version précédente, Live fermé.
