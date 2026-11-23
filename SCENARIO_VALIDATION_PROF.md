# Scénario de validation — Module 5 « Logistique de Livraison »
**Ayette Ben Mohamed — Équipe Vencord — WaterFlow (Smart Water Factory)**

> **Durée visée : 12 à 15 minutes.** Fil rouge de la démo : *« une journée de livraison d'eau chez WaterFlow »*.
> Chaque acte indique : 🎯 l'exigence du cahier couverte · 🗣 ce que tu dis · 🖱 ce que tu fais · 👀 ce que la prof doit voir.

---

## ✅ La veille / 10 minutes avant

| | À faire |
|---|---|
| ☐ | Service **OracleServiceXE** démarré |
| ☐ | Qt Creator : **Compiler → Exécuter qmake**, puis **Compiler** (aucune erreur) |
| ☐ | **Le jour même, juste avant de passer** : `sqlplus wf/azerty@localhost:1521/XE @sql/05_demo_tournee.sql` (les horaires de la tournée sont calculés à partir de l'heure d'exécution du script) |
| ☐ | Application lancée **en mode jour**, fenêtre agrandie, sur l'écran de connexion |
| ☐ | Bloc-notes ouvert (pour coller les messages d'alerte) |
| ☐ | Dossier « Documents » vide de vieux PDF (pour retrouver vite les PDF générés) |

Données que tu vas saisir pendant la démo (à garder sous les yeux) :
- **Livreur** : `HAMDI` / `Yassine`, poste **Livreur**, département **Logistique**, shift **Matin**, statut **Actif**, tél. `22 456 789`
- **Commande** : client `Hôtel Africa`, produit `Bonbonne 19L`, quantité `40`, adresse `Tunis, Lac 1`, montant `520`
- **Véhicule** : `210 TUN 5821`

---

## Acte 0 — Introduction (30 s)
🗣 *« Mon module est la Logistique de Livraison. Une livraison relie une commande du module Commandes à un livreur du module Personnel. J'ai réalisé le CRUD, les 4 métiers basiques et les 2 métiers innovants du cahier : l'optimisation des itinéraires et l'estimation dynamique de l'heure d'arrivée, avec alerte client. Le tout est organisé en MVC sur Oracle. »*

🖱 Rien à cliquer. Si la prof veut l'architecture tout de suite, voir « Montrer le code » plus bas.

---

## Acte 1 — Connexion, sécurité et interface (1 min)
🎯 Exigences non fonctionnelles : sécurité (authentification), ergonomie, charte graphique.

| 🖱 Action | 👀 Résultat attendu |
|---|---|
| Montrer l'écran de connexion | Panneau animé sur le thème de la livraison : itinéraire, colis, repères, camions-citernes WaterFlow |
| Cliquer « Se connecter » avec les champs vides | « Veuillez remplir les deux champs. » |
| `admin` / `admin123` | Fenêtre principale, « Connecté : admin » |
| Montrer le **Tableau de bord** (page d'accueil) | Indicateurs des 3 modules (Personnel, Commandes, Livraisons) + liste des alertes ; boutons « Ouvrir … » vers chaque module |
| Cliquer **🌙 Mode sombre**, puis **☀ Mode jour** | Toute l'interface change de thème (choix mémorisé) |

🗣 *« Les mots de passe sont hachés en SHA-256 (QCryptographicHash). Les couleurs sont celles de la charte du cahier : bleu profond #0A4D8C, cyan #00B4D8, vert, orange et rouge pour les statuts. J'ai ajouté un mode sombre et un fond animé sur le thème de la livraison. »*

---

## Acte 2 — Intégration avec les autres modules (2 min)
🎯 Relations entre modules : Personnel → Livraisons ← Commandes.

| 🖱 Action | 👀 Résultat attendu |
|---|---|
| **Personnel** → ajouter **Yassine HAMDI**, poste **Livreur**, statut **Actif** | Employé ajouté dans la liste |
| **Commandes** → ajouter la commande **Hôtel Africa** | Commande au statut « En attente » |
| **Livraisons** → onglet *Planification* → ouvrir la liste **Livreur** | **Yassine HAMDI (22 456 789)** est proposé. Karim MEJRI (en congé) n'y est pas |
| Ouvrir la liste **Commande**, choisir **Hôtel Africa** | L'adresse « Tunis, Lac 1 » est remplie automatiquement, avec l'estimation « ≈ x km · y min de trajet » |

🗣 *« Les livreurs viennent du module Personnel : seuls les employés actifs au poste Livreur sont proposés. Les commandes viennent du module Commandes : seulement celles qui restent à livrer. »*

---

## Acte 3 — CRUD et contrôle de saisie (2 min 30)
🎯 Créer / Lire / Modifier / Supprimer avec contrôle de saisie (QRegularExpression), confirmation avant suppression, règles métier.

| 🖱 Action | 👀 Résultat attendu |
|---|---|
| Véhicule `ABC` → **Planifier** | ❌ « Immatriculation invalide : format attendu 123 TUN 4567 » |
| Véhicule `210 TUN 5821`, arrivée prévue **avant** le départ → **Planifier** | ❌ « L'heure de livraison prévue doit être postérieure au départ » |
| Départ **demain 09:00**, arrivée **demain 11:00** → **Planifier** | ✅ « Livraison n° X planifiée — la commande est passée En préparation » ; nouvelle ligne avec la pastille bleue **Planifiée** |
| **Commandes** → regarder Hôtel Africa | Statut **En préparation**, transporteur **Yassine HAMDI** (mis à jour automatiquement) |
| Toujours dans Commandes : changer son statut en « Livrée » → **Modifier** | ❌ « Le statut de cette commande est géré par la livraison n° X » |
| **Livraisons** → **Nouveau** → même livreur Yassine, une autre commande, **demain 09:30 → 10:30** → **Planifier** | ❌ « Yassine HAMDI a déjà une livraison active sur ce créneau » |
| Cliquer la livraison Hôtel Africa → statut **En cours** → **Enregistrer** | ✅ Pastille cyan ; commande → **Expédiée** |
| Statut **Échouée** → **Enregistrer** | ✅ Commande → **En attente**, et elle redevient disponible pour une nouvelle livraison |
| Statut **Livrée** → **Enregistrer** | ✅ Commande → **Livrée** ; la date réelle de livraison est enregistrée |

🗣 *« L'enregistrement d'une livraison est une transaction : la livraison, le lien REGROUPE, la commande et l'historique sont mis à jour ensemble ; en cas d'erreur, tout est annulé (rollback). »*

*(La suppression est montrée à l'acte 7.)*

---

## Acte 4 — Les 4 métiers basiques (2 min 30)

| 🎯 Métier basique | 🖱 Action | 👀 Résultat attendu |
|---|---|---|
| **1. Trier par date ou statut** | Liste **Tri** : « départ (plus récent) », puis « statut (urgences d'abord) » | L'ordre change ; Retardée / En cours remontent en haut |
| **2. Rechercher / filtrer** (transporteur, zone, statut) | Taper `sami` · puis **Statut = Planifiée** · puis **Zone = Ariana** | Filtrage immédiat, filtres combinables |
| **3. Export PDF** | Sélectionner une livraison → **Bordereau de livraison (PDF)** · puis **Exporter la liste (PDF)** | Bordereau A4 (en-tête WaterFlow, détails, distance, signatures) · liste en paysage avec les filtres appliqués |
| **4. Statistiques en graphiques** | Onglet **Statistiques** | Délai moyen commande → livraison, durée moyenne, % livrées en moins de 48 h (valeur de référence du cahier), ponctualité, anneau par statut, barres par transporteur (Qt Charts) |

🗣 *« Les 48 heures sont le délai de livraison cible indiqué dans le tableau des valeurs de référence du cahier. »*

---

## Acte 5 — Métier innovant 1 : optimisation des itinéraires (1 min 30)
🎯 « Calcul du trajet le plus court/rapide entre plusieurs points de livraison pour réduire les distances parcourues. »

| 🖱 Action | 👀 Résultat attendu |
|---|---|
| Onglet **Tournées & ETA** → livreur **Sami TRABELSI**, jour **aujourd'hui** | 5 arrêts : km entre arrêts, heure prévue, ETA, écart + carte de la tournée depuis le dépôt |
| **Optimiser l'itinéraire** | Nouvel ordre de passage ; les arrêts déjà livrés ou en cours ne bougent pas ; carte redessinée ; résumé « itinéraire optimisé » |
| **Valider la tournée** | « L'ordre de passage (5 arrêts, x km) est enregistré » |

🗣 *« Les distances sont calculées avec la formule de haversine, multipliée par 1,3 pour approcher la distance routière. Jusqu'à 8 arrêts, j'essaie toutes les permutations : la solution est exacte. Au-delà, j'utilise l'heuristique du plus proche voisin améliorée par 2-opt. L'algorithme minimise la distance et pénalise aussi les retards par rapport aux heures promises aux clients : le trajet est à la fois court et ponctuel. »*

---

## Acte 6 — Métier innovant 2 : ETA dynamique + alerte client (2 min)
🎯 « Recalcul automatique de l'heure prévue selon l'avancement réel de la tournée, avec alerte au client en cas de retard. »

| 🖱 Action | 👀 Résultat attendu |
|---|---|
| Montrer la ligne 1 | ETA = heure **réelle** (« réelle ») : le calcul part de ce qui s'est vraiment passé |
| Montrer la ligne orange | « +xx min » de retard ; compteur « ⚠ arrêt(s) en retard » ; « dernier calcul à HH:mm » (recalcul automatique chaque minute) |
| **Avancement réel** : onglet *Planification* → livraison **902** → statut **Livrée** → **Enregistrer** → revenir à *Tournées & ETA* → **Calculer la tournée** | La 902 passe en heure réelle et les ETA suivantes sont recalculées à partir de ce point |
| **Alerter les clients en retard** | Fenêtre avec un message prêt à envoyer par client (retard + nouvelle heure) |
| **Confirmer et copier les messages** → Ctrl+V dans le Bloc-notes | Livraison(s) → **Retardée** (orange) ; messages collés ; bandeau d'alerte en haut de la page |

🗣 *« Hypothèses : 35 km/h de moyenne en ville et 10 minutes par arrêt. On repart de l'heure réelle du dernier arrêt livré, et jamais d'avant l'heure actuelle. Au-delà de 15 minutes de retard, le client est alerté. »*

---

## Acte 7 — Traçabilité, suppression, déconnexion (1 min)
🎯 Exigence non fonctionnelle « Traçabilité » (date + auteur des modifications importantes) ; confirmation avant suppression.

| 🖱 Action | 👀 Résultat attendu |
|---|---|
| Onglet **Traçabilité** | Toutes les actions de la démo : création, changements de statut, itinéraire optimisé, alerte client, bordereau PDF. Avec la date, l'utilisateur **admin**, l'ancien → le nouveau statut |
| Rechercher `Alerte` | Seulement les alertes clients |
| *Planification* → livraison Hôtel Africa → **Supprimer** → **Annuler**, puis **Supprimer** | Annuler : rien. Confirmer : ligne supprimée, et la suppression reste visible dans la Traçabilité |
| **Personnel** → supprimer **Sami TRABELSI** | ❌ Message clair : « lié à N livraison(s) … passez-le plutôt au statut Suspendu » (au lieu de l'erreur Oracle brute) |
| **⏻ Se déconnecter** → confirmer | Retour à l'écran de connexion |

---

## Conclusion (30 s)
🗣 *« Pour résumer : le CRUD complet avec contrôle de saisie, les 4 métiers basiques, les 2 métiers innovants, l'intégration avec Personnel et Commandes, la traçabilité, et un test automatique de 37 vérifications. »*

### Couverture du cahier
| Exigence du cahier (Module 5) | Acte |
|---|---|
| CRUD avec contrôle de saisie | 3 |
| Trier par date ou statut | 4 |
| Rechercher / filtrer par transporteur, zone, statut | 4 |
| Exporter bordereau / liste en PDF | 4 |
| Statistiques : délai moyen, nombre par statut et par transporteur, en graphiques | 4 |
| Innovant : optimisation des itinéraires | 5 |
| Innovant : ETA dynamique + alerte client | 6 |
| Relations entre modules | 2, 3 |
| Sécurité (login, mot de passe haché, requêtes préparées) | 1, Q&R |
| Fiabilité (contrôle de saisie, confirmation, erreurs explicites) | 3, 7 |
| Traçabilité (date + auteur) | 7 |
| Ergonomie / charte graphique | 1 |

---

## Si la prof demande de « montrer le code »

| Ce qu'elle demande | Fichier : fonction |
|---|---|
| Architecture MVC | Modèle : `livraison.cpp`, `historiquelivraison.cpp`, `itineraire.cpp` · Vue : `mainwindow.ui` (un seul `.ui` pour tout le dashboard, page Livraisons = widget promu), `cartetournee.cpp` · Contrôleur : `livraisons.cpp` (aucune requête SQL) |
| Contrôle de saisie | [livraison.cpp:64](livraison.cpp:64) `Livraison::verifier` (QRegularExpression de l'immatriculation) |
| Transaction | [livraison.cpp:122](livraison.cpp:122) `Livraison::enregistrer` (`transaction` / `commit` / `rollback`) |
| Créneau du livreur déjà pris | [livraison.cpp:95](livraison.cpp:95) `Livraison::livreurOccupe` |
| Synchronisation avec la commande | [livraison.cpp:55](livraison.cpp:55) `statutCommandePour` · verrou dans [commande.cpp:54](commande.cpp:54) `Commande::modifier` |
| Tri / recherche / filtres | [livraison.cpp:334](livraison.cpp:334) `Livraison::rechercher` (requête préparée) |
| Statistiques | [livraison.cpp:379](livraison.cpp:379) `Livraison::statistiques` · graphiques [livraisons.cpp:626](livraisons.cpp:626) |
| PDF | [livraisons.cpp:928](livraisons.cpp:928) `exporterBordereau` · [livraisons.cpp:969](livraisons.cpp:969) `exporterListe` |
| Optimisation des itinéraires | [itineraire.cpp:102](itineraire.cpp:102) `Itineraire::optimiser` (permutations exactes / plus proche voisin + 2-opt) |
| ETA dynamique | [itineraire.cpp:175](itineraire.cpp:175) `Itineraire::calculerEta` · recalcul chaque minute [livraisons.cpp:897](livraisons.cpp:897) `tic` |
| Alerte client | [livraisons.cpp:843](livraisons.cpp:843) `alerterClients` |
| Traçabilité | [historiquelivraison.cpp:70](historiquelivraison.cpp:70) `HistoriqueLivraison::enregistrer` |
| Livreurs de Personnel / commandes de Commandes | [livraison.cpp:500](livraison.cpp:500) `listeLivreurs` · [livraison.cpp:516](livraison.cpp:516) `listeCommandes` |

---

## Questions probables et réponses courtes

- **Pourquoi la table REGROUPE ?** C'est l'association LIVRAISON ↔ COMMANDE du MCD (une livraison regroupe des commandes). Elle permet aussi de replanifier une commande après une livraison échouée.
- **Comment évitez-vous l'injection SQL ?** Toutes les requêtes utilisent `prepare()` + `bindValue()`.
- **Que se passe-t-il si une étape échoue au milieu ?** `rollback()` : la base reste cohérente, rien n'est enregistré à moitié.
- **Pourquoi un livreur en congé n'apparaît pas ?** Filtre `POSTE = 'Livreur' AND STATUT = 'Actif'` sur les données du module Personnel.
- **L'optimisation est-elle vraiment optimale ?** Oui jusqu'à 8 arrêts (toutes les permutations, 8! = 40 320 essais). Au-delà, c'est une heuristique (plus proche voisin + 2-opt), qui donne de très bonnes solutions rapidement.
- **D'où viennent les coordonnées ?** D'un référentiel des localités du Grand Tunis et des grandes villes desservies (`Geo::localiser`) : la localité est reconnue dans l'adresse, sinon la zone sert de repli.
- **Comment est testée l'application ?** `tests/test_integration.pro` : 37 vérifications automatiques sur la vraie base (CRUD, règles métier, itinéraire, ETA, traçabilité), résultat « 0 échec ».
- **Lien avec les ODD ?** ODD 11 (fiabilité de la livraison d'eau aux citoyens et entreprises) et ODD 9 (digitalisation de la logistique).

---

## Plan B (si quelque chose coince)

| Problème | Solution immédiate |
|---|---|
| « Connexion Oracle impossible » au lancement | Démarrer le service **OracleServiceXE** (services.msc), puis relancer |
| Tournée de Sami vide | Vérifier que le jour choisi est **aujourd'hui** ; relancer `05_demo_tournee.sql` |
| Bouton « Alerter les clients » grisé | Aucun retard de plus de 15 min : dans *Planification*, mettre l'arrivée prévue de la 904 à *maintenant + 5 min*, enregistrer, recalculer la tournée |
| Yassine absent de la liste des livreurs | Vérifier dans Personnel : poste **Livreur** et statut **Actif**, puis recliquer sur *Livraisons* |
| L'interface rame | Décocher **Fond animé** dans la barre latérale |
| Plus de temps | Lancer le test automatique `tests/test_integration.pro` et montrer « 0 échec(s) » |
