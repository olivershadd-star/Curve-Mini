# Décision — 6 novembre 2026

Nouvelle branche de conception : Curve Mini Prototype 0.1.

Les essais historiques Curve Pilot V1, V2 et V2.1 sont conservés et gelés. L'essai R4 d'optimisation locale n'est pas livré et ne constitue pas la base du prototype. Aucun ancien dépôt ni paquet n'est supprimé ou écrasé.

But prioritaire : assigner des courbes aux macros d'Ableton, dans Live Lite sans Max for Live. Ableton héberge les instruments et les effets. Curve produit seulement des commandes de contrôle MIDI.

La variante VST3 prototype est un dispositif silencieux sur une piste séparée. Le port MIDI virtuel relie sa sortie Windows à l'entrée Remote de Live. Ce compromis est explicite et doit être testé avant toute extension de conception.

Fil audio : effacement de sa propre sortie silencieuse et publication atomique du transport. Aucun chargement VST, aucun appel à un moteur audio tiers, aucun appel au pilote MIDI, aucun verrou d'interface. Émission MIDI : timer du fil messages, 100 Hz maximum, dédoublonnage des valeurs. Affichage : 15 Hz, seulement les lignes visibles. Ce n'est pas un séquenceur à précision échantillon.

Critères avant évolution : une macro apprend la bonne adresse, deux macros restent indépendantes, arrêt de lecture arrête les commandes, sauvegarde set puis reconnexion retrouvent les courbes et les liens, charge reste faible avec Serum chargé directement. La consommation devra être mesurée sur le PC utilisateur et n'est pas déduite de la seule structure du code.
