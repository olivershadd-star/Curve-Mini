# Langage de courbes — version 0.1

Mots-cles en anglais, nombres avec point decimal, valeurs 0..100 %. Les commentaires commencent par #. Pas de code executable. Une instruction par ligne, sensible a la casse.

En-tete :
```
duration: 8 bars
start: 20%
```
La duree totale doit aller de 0.25 a 32 mesures, par pas de 0.25. start et duration doivent preceder les segments. Par defaut : 4 mesures et 20 %.

Instructions exactes :
```
ramp to 70% over 2 bars smooth
hold over 1 bars
wave around 70% depth 10% cycles 2 over 2 bars sine
saw from 15% to 60% cycles 3 over 2 bars linear
```
- ramp : part de la valeur precedente et rejoint la cible. Dernier mot obligatoire : linear, smooth, in ou out.
- hold : garde la valeur precedente.
- wave : sinus autour du centre, nombre entier de cycles (1..64). La valeur precedente doit etre egale au centre ; ajouter une rampe avant si necessaire. Le sinus termine sur son centre.
- saw : dents de scie depuis from jusqu'a to avec retours brusques. La valeur precedente doit etre egale a from. Le segment termine sur from, pas sur to.
- depth : amplitude de part et d'autre du centre ; la vague doit rester entre 0 et 100 %.
- bars reste toujours au pluriel, meme pour une seule mesure.
- La somme de toutes les durees doit etre exactement egale a duration.
- Les durees fractionnaires sont admises, par exemple 0.5 bars.
- Pas de boucle, expression mathematique, variable, fichier ou import.

Exemple complet valide :
```
duration: 8 bars
start: 20%
ramp to 70% over 2 bars smooth
wave around 70% depth 10% cycles 2 over 2 bars sine
ramp to 15% over 1 bars linear
saw from 15% to 60% cycles 3 over 2 bars linear
ramp to 20% over 1 bars out
```

La lecture boucle la courbe. Si fin != debut, le retour est abrupt ; utiliser le lissage de la ligne ou terminer sur la valeur initiale pour un raccord doux. Min/Max et profondeur s'appliquent ensuite : ce texte definit la geometrie normalisee, pas une frequence en Hz ou un niveau en dB.
