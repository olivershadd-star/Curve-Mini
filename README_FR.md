# Curve Mini Prototype 0.2 — courbes et potards MIDI

Outil de contrôle léger pour Ableton Live 12 Lite / Suite et machines MIDI USB.
Windows x64, VST3 et application autonome. Aucun synthé ni effet hébergé,
aucun Max for Live nécessaire. Le VST3 reste un instrument silencieux sur sa
propre piste MIDI ; les instruments, effets et pistes de notes restent dans Live.

## Nouveautés 0.2

Chaque ligne possède une activation, une sortie MIDI (ou la sortie globale),
un canal 1–16 et un CC 0–127. Exemples pour une groovebox à quatre parties :

| Ligne Mini | Sortie | Canal | CC |
| --- | --- | --- | --- |
| 1 | Groovebox USB | 1 | 70 |
| 2 | Groovebox USB | 1 | 71 |
| 3 | Groovebox USB | 2 | 70 |
| 4 | Groovebox USB | 3 | 73 |

Le canal est celui de la partie configurée dans la machine, pas le numéro de
piste Ableton. Vérifier les numéros dans l'implémentation MIDI de la machine.
On peut choisir une autre machine pour chaque ligne. Les sorties sont ouvertes
une seule fois par instance, même si plusieurs lignes utilisent le même port.

Les anciennes sauvegardes Mini 0.1 retrouvent CC20–31, canal 1, courbes et
réglages. Le code plugin Cpm1 et le tag XML CURVE_PILOT_MIDI_01 sont conservés.
Les sessions des anciens Curve Pilot V1/V2/V2.1 hébergeant des instruments ne
sont pas importées. Les deux projets restent distincts.

## Installer le nouveau VST3

Dans GitHub Actions, ouvrir le dernier build réussi, puis télécharger
`Curve-Mini-Prototype-0.2-Windows-x64`. Dézipper. Ableton fermé, remplacer
l'ancien dossier `Curve Mini Prototype 0.1.vst3` par le dossier COMPLET
`Curve Mini Prototype 0.2.vst3` dans `C:\Program Files\Common Files\VST3`.
Éviter de garder les deux versions installées avec le même identifiant plugin.
Conserver une copie de l'ancienne version ailleurs si nécessaire.

Le workflow Windows se lance désormais automatiquement après une modification
du code sur main ; son lancement manuel reste disponible.

## Premier essai : une ligne vers une machine

1. Garder les pistes de notes habituelles dans Live. Ajouter Mini sur une autre
   piste MIDI.
2. Dans Mini, choisir la sortie USB de la machine comme sortie globale. Sur la
   ligne 1, garder « Sortie globale », choisir le canal de la partie et son CC.
3. Cliquer Connecter, activer la ligne 1, puis ENVOI ON.
4. Tourner « Manuel 0-127 ». La ligne passe en MANUEL et envoie cette valeur,
   même lorsque Live est à l'arrêt.
5. Cliquer MANUEL pour revenir à COURBE, choisir une forme ou dessiner, puis
   lancer Live. La courbe commande le même CC.
6. Ajouter la ligne 2 avec un autre CC ou canal et refaire le test.

ENVOI OFF arrête tous les messages automatiques et manuels. Activer une ligne
ne suffit pas : ENVOI doit aussi être ON. Modifier son adresse arrête ENVOI
pour permettre de vérifier la destination. Modifier un port exige aussi de
recliquer Connecter. Deux lignes actives ne peuvent pas partager le même
port/canal/CC ; ENVOI est refusé ou arrêté pour éviter leurs commandes concurrentes.
L'arrêt du transport arrête les courbes ; le mode MANUEL reste volontairement
utilisable à l'arrêt. Assigner Live est une émission ponctuelle distincte.

### Si la sortie USB ne peut pas être ouverte

Certains pilotes Windows ne permettent pas à Mini et Live d'ouvrir la même
sortie simultanément. Ne pas modifier les pistes de notes pour contourner cela.
Créer plutôt un port loopMIDI, par exemple `CurveMini Materiel`, et le choisir
dans Mini. Dans Live, activer **Track/Piste** pour l'entrée de ce port et laisser
**Remote/Télécommande** et **Sync** OFF. Laisser sa sortie dans Live désactivée.

Créer une piste relais MIDI : entrée `CurveMini Materiel` sur le canal de la
partie, Monitor **In**, sortie vers la machine et canal correspondant. Une piste
relais par canal permet de conserver les destinations des différentes parties.
Live ouvre alors la sortie USB et relaie les CC de Mini. Ne pas renvoyer cette
piste vers le port loopMIDI d'entrée. Ce routage matériel diffère du mode macros
ci-dessous ; ne pas confondre Track et Remote.

## Assigner un contrôleur physique aux potards de Mini

Mini expose 12 paramètres VST3 : `Ligne 1 Valeur` à `Ligne 12 Valeur` (0–127).
L'entrée physique est gérée par Live ; Mini n'ouvre pas le port du contrôleur
et ne reçoit pas ses CC bruts sur le bus MIDI du VST3.

1. Dans les réglages MIDI de Live, activer Remote/Télécommande pour l'entrée
   du contrôleur physique.
2. Dans le panneau du plugin Mini dans Live, ouvrir **Configure**, puis bouger
   le potard « Manuel 0-127 » de la ligne voulue. Quitter Configure.
3. Activer l'assignation MIDI de Live (Ctrl+M), sélectionner le paramètre exposé
   `Ligne N Valeur`, tourner le potard physique, puis quitter Ctrl+M.
4. Dans Mini, connecter la destination, activer la ligne et ENVOI. Le potard
   physique commande désormais le CC/canal de cette ligne.

Le CC d'entrée du contrôleur peut être différent du CC de sortie. Exemple :
CC21 de votre contrôleur -> Ligne 1 Valeur de Mini -> CC74/canal 2 de la machine.
Un rack contenant Mini peut aussi mapper ses macros à ces paramètres.

Tourner le potard (souris, contrôleur ou automation de Live) prend la main sur
la courbe de cette ligne. Les autres lignes continuent. Cliquer MANUEL pour
reprendre la courbe. Le changement de valeur prend la main ; renvoyer une valeur
identique n'est pas un nouveau mouvement. La reprise peut provoquer un saut de
valeur : pas de système de rattrapage ou de fondu dans ce prototype.

Le potard manuel donne la valeur CC absolue, de 0 à 127 ; minimum/maximum ne
s'appliquent qu'à la courbe. Le potard ne suit pas visuellement la courbe :
« Valeur » affiche le dernier message envoyé. Les gestes peuvent être enregistrés
comme automation de ce paramètre dans Live, mais ne sont pas redessinés dans Mini.

En autonome, les potards fonctionnent à la souris ; cette version n'a pas
l'assignation MIDI de Live et ne possède pas d'apprentissage d'entrée propre.

## Exporter une courbe dans un clip Live

1. Régler la courbe, minimum/maximum, longueur, canal et CC de la ligne.
2. Cliquer **Exporter MIDI** puis enregistrer le `.mid`.
3. Déposer ce fichier dans un emplacement de clip MIDI vide dans Live.
4. Dans les enveloppes du clip, choisir **MIDI Ctrl**, puis le CC exporté.
   Régler la sortie de la piste et son canal vers la machine.
5. Désactiver la ligne de Mini qui envoyait ce CC pour éviter un double pilotage.

Le fichier contient un tour de la courbe et aucun événement de note. Il ne
fusionne pas automatiquement avec un clip de notes existant. Pour conserver
vos notes, importer d'abord dans un clip séparé, puis utiliser les commandes
d'édition d'enveloppe de Live pour transférer l'enveloppe si nécessaire.

Export SMF0 à 960 ticks/noire, un échantillon tous les 10 ticks (1/96 de noire),
valeurs 7 bits dédoublonnées, fin de piste à la longueur exacte. Aucun changement
de tempo n'est imposé. Le VST3 utilise la dernière signature rythmique publiée
par Live ; à défaut et en autonome, 4/4. Les exports ne contiennent pas les
gestes manuels ni les sauts de lecture. L'import réel dans votre Live reste à tester.
Les CC120–127 sont des commandes de mode de canal ; Live expose surtout les CC
jusqu'à 119 dans ses enveloppes. CC0/32 ont un rôle particulier de sélection de banque.

La documentation Live 12 confirme l'import des contrôleurs en enveloppes :
[Ableton — enveloppes de contrôleurs MIDI](https://www.ableton.com/en/manual/clip-envelopes/#midi-controller-clip-envelopes).
Cela confirme le principe du transfert par fichier, pas encore son essai sur
votre poste ni un copier-coller direct depuis la fenêtre du plugin.

## Conserver le mode macros de VST dans Ableton

Créer un port loopMIDI `CurveMini Macros`. Dans Live, pour son entrée : Remote
ON, Track OFF, Sync OFF ; pour sa sortie : tout OFF. Mini ouvre directement
la sortie système de ce port.

Dans Mini, sélectionner ce port et Connecter. Dans Ctrl+M de Live, sélectionner
une macro cible puis cliquer **Assigner Live** sur la ligne. Un seul CC est
émis à 64/127, ce qui peut placer la macro à mi-course ; ENVOI est coupé. Quitter
Ctrl+M puis réactiver ENVOI. Ne pas utiliser cette opération pour « apprendre »
les paramètres fixes d'une groovebox : y choisir simplement leur canal et CC.

Les correspondances du contrôleur physique et des macros sont conservées dans
le set Live. Les courbes, sorties, adresses, modes et valeurs manuelles sont
conservés dans l'état Mini. Après restauration, ENVOI est OFF et toutes les
sorties sont fermées : vérifier les ports, Connecter puis ENVOI.

## Limites et validation

Émission sur le fil messages, au maximum 100 Hz par ligne, sans renvoi des
valeurs inchangées. L'USB et Live ajoutent leur propre délai. Aucun pilote MIDI,
verrou d'interface ou instrument hébergé dans le callback audio. MIDI 7 bits,
pas de précision audio ni de haute résolution. Le timer continue fenêtre fermée.
Un pilote débranché ne fournit pas nécessairement d'erreur lors de l'envoi :
le statut connecté ne garantit pas la réception. Vérifier et reconnecter.

Le modèle MIDI et l'encodeur SMF sont testés localement. Le workflow Windows
compile les deux formats et exécute aussi les tests du processeur, de ses
paramètres et de la migration des sauvegardes. Les résultats de cette évolution
sont consignés dans REPRISE.md. Seul un essai utilisateur peut confirmer
l'assignation physique, les ports USB/loopMIDI, l'import dans Live, la charge et
la lisibilité à votre mise à l'échelle Windows.
