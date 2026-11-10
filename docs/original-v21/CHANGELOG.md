# V2.1 reprise R2 — 5 novembre 2026

Remplace la V2.1 rejetée. Identité VST Cvp2, noms de produit et cibles de compilation conservés.

- Trois pages principales explicites : Synthe, Courbes, Effets ; navigation indépendante du zoom. Page appareil masque les courbes et les cases d'effets pour travailler sur l'appareil sélectionné.
- Zone d'accueil Windows : peer natif enfant dédié pour contenir les HWND des VST tiers. Défilement mesuré sur un conteneur aux dimensions réellement transformées ; mise à jour sur changements de taille et de zoom.
- Traducteur local français de mouvements : phrases limitées, positions, rampes, durées, oscillations, dents de scie ; interprétation et prévisualisation avant application, erreurs sans modification de courbe. Mode Code avancé conservé.
- Description appliquée conservée dans l'état des lignes et les copies de courbes.
- Tests du traducteur et scénario de régression d'interface avec éditeur surdimensionné. Test natif Windows exécuté par GitHub Actions avant les artefacts.
- Conservé : douze courbes, un instrument, huit effets en série réorganisables, Dry/Wet, versions VST3 et autonome, sauvegardes communes et transport interne.
