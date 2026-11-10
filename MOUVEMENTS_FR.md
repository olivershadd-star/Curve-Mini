# Décrire le mouvement du bouton

Assistant local à vocabulaire limité : il traduit les gestes d'un potentiomètre. Il n'interprète pas une ambiance musicale. Écrire « ouvre », « ferme », « maintiens », « oscille », « dents de scie », puis enchaîner avec « puis », « ensuite » ou une virgule. Les accents sont acceptés dans les formes courantes documentées.

## Exemples directement utilisables
- Ouvre rapidement jusqu'à 80 %, ferme jusqu'à 25 %, oscille rapidement et légèrement, puis ouvre complètement et ferme lentement.
- Ouvre jusqu'à 80 % pendant 2 temps puis ferme sur 2 temps. Choisir 1 mesure pour la ligne.
- Ouvre à 50 %, oscille autour de 50 % amplitude 10 % pendant 2 temps, puis ferme.
- Dents de scie jusqu'à 90 % 8 cycles. Le départ doit être différent de 90 %.
- Maintiens pendant 1 temps, puis ouvre progressivement jusqu'à 80 % et ferme lentement.

## Vocabulaire initial
- Direction : ouvre, ouvrir, ouverture, monte, monter, réouvre, réouverture ; ferme, fermer, fermeture, descend, descends, descendre.
- Positions : pourcentages 0 à 100 ; haut/haute = 80 %, bas/basse = 20 %, moitié = 50 %, maximum = 100 %, minimum = 0 %.
- Ouvrir complètement = 100 % ; fermer complètement = 0 %.
- « Ferme de trois quarts » réduit l'ouverture actuelle de 75 % ; depuis 80 %, on arrive donc à 20 %. « Ferme jusqu'à 25 % » vise directement 25 %.
- Rampe : rapide/rapidement, lent/lente/lentement, sec/sèche/brutal/immédiat, courbé/courbée/progressif/doux/douce, linéaire/droit/droite.
- Oscillations : oscille / oscillations / va-et-vient, rapide/lent, serré/petit/léger (amplitude 5 points), large/ample (25 points), amplitude ou profondeur chiffrée, cycles chiffrés. Amplitude normale : 10 points.
- Osciller autour de 0 ou 100 % ne laisse pas de place à une oscillation symétrique ; le traducteur demande un centre intérieur. Il réduit l'amplitude si nécessaire pour rester entre 0 et 100 % et indique la valeur retenue.
- Dents de scie : « dents de scie jusqu'à 90 % 8 cycles » monte du départ vers 90 % puis retombe à chaque cycle. « Ouvre cisaillé » propose des dents de scie. La forme est ascendante ou descendante selon les deux valeurs.
- Temps : « pendant 2 temps », « sur 1 mesure », « pendant 0,5 mesure ». Nombres écrits courants acceptés : zéro à dix, douze, seize.
- Maintien : maintiens, maintenir, maintien, reste.

Le langage n'est pas du français libre. Des termes comme « spatial », « plus chaud », « va sur Mars », ou des formes non encore prises en charge sont refusés avec une explication. Pour un résultat précis, donner positions, durées et nombre de cycles. Aucune phrase n'est appliquée sans Previsualiser puis Appliquer.

Sans durées, le programme répartit celle de la ligne selon les mouvements et explique ses choix ; s'ils ont tous une durée et qu'il reste du temps, il ajoute un maintien final. Si les durées dépassent la ligne ou ne laissent rien à un mouvement non chiffré, il demande une correction.

Cette première version construit une proposition complète. Elle ne comprend pas encore des retouches conversationnelles du type « garde le début et accélère juste la fin ». On peut modifier la description complète, éditer les points, ou passer en Code avancé.
