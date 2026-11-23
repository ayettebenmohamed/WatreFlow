# Scénario de test — Module Logistique de Livraison (WaterFlow)

Durée : **environ 15 minutes**. Suivre les étapes dans l'ordre : chaque étape prépare la suivante.
Cocher ✅ quand le résultat attendu est obtenu.

---

## 0. Préparation (5 min avant la validation)

| # | Action | Résultat attendu | ✅ |
|---|---|---|---|
| 0.1 | Vérifier que le service **OracleServiceXE** est démarré | — | ☐ |
| 0.2 | Recharger la tournée de démo **le jour même** : `sqlplus wf/azerty@localhost:1521/XE @sql/05_demo_tournee.sql` | « Commit complete. » | ☐ |
| 0.3 | Qt Creator : **Compiler → Exécuter qmake**, puis **Compiler** et **Exécuter** | L'application démarre sans erreur | ☐ |

---

## 1. Connexion et interface

| # | Action | Résultat attendu | ✅ |
|---|---|---|---|
| 1.1 | Observer l'écran de connexion | Panneau bleu animé : itinéraire en pointillés, colis et repères de livraison, camions-citernes WaterFlow sur la route | ☐ |
| 1.2 | Cliquer « Se connecter » avec les champs vides | Message « Veuillez remplir les deux champs » | ☐ |
| 1.3 | Saisir `admin` / un mauvais mot de passe | « Identifiant ou mot de passe incorrect » | ☐ |
| 1.4 | Saisir `admin` / `admin123` | Fenêtre principale ; en bas de la barre latérale : « Connecté : admin » | ☐ |
| 1.5 | Décocher puis recocher **« Fond animé »** (barre latérale) | L'animation s'arrête puis reprend (choix mémorisé au prochain lancement) | ☐ |
| 1.6 | Ouvrir **Livraisons** | Bandeau d'en-tête animé (tournée + camions), horloge en français, alerte orange si des livraisons sont en retard, 5 indicateurs | ☐ |
| 1.7 | Cliquer **🌙 Mode sombre** (barre latérale) | Toute l'application passe en bleu nuit : tableaux, cartes, graphiques, carte de tournée ; le bouton devient **☀ Mode jour**. Choix mémorisé au prochain lancement | ☐ |
| 1.8 | Cliquer **⏻ Se déconnecter** → **Annuler**, puis **Se déconnecter** (ou Ctrl+Maj+Q) | Annuler : rien ne change. Confirmer : retour à l'écran de connexion ; se reconnecter rouvre l'application | ☐ |

---

## 2. Intégration avec les autres modules

| # | Action | Résultat attendu | ✅ |
|---|---|---|---|
| 2.1 | **Personnel** : ajouter `TEST` / `Livreur`, poste **Livreur**, statut **Actif** | Employé ajouté | ☐ |
| 2.2 | **Commandes** : ajouter client `Café Démo`, produit `Pack eau 1.5L x6`, quantité `20`, adresse `Ariana, centre`, montant `150` | Commande ajoutée, statut « En attente » | ☐ |
| 2.3 | Retour dans **Livraisons** | « Livreur » contient **Livreur TEST** ; « Commande » contient **Café Démo** | ☐ |
| 2.4 | Choisir la commande Café Démo | L'adresse « Ariana, centre » est proposée automatiquement, avec l'estimation « ≈ x km · y min » | ☐ |

> Ce que montre cette étape : la liste des livreurs vient de Personnel (postes « Livreur » actifs uniquement) et les commandes à livrer viennent de Commandes.

---

## 3. CRUD et contrôle de saisie (onglet Planification)

| # | Action | Résultat attendu | ✅ |
|---|---|---|---|
| 3.1 | Véhicule `ABC`, puis **Planifier** | « Immatriculation invalide : format attendu 123 TUN 4567 » | ☐ |
| 3.2 | Véhicule `200 TUN 1234`, arrivée prévue **avant** le départ, puis **Planifier** | « L'heure de livraison prévue doit être postérieure au départ » | ☐ |
| 3.3 | Départ prévu **hier**, puis **Planifier** | « Le départ … ne peut pas être dans le passé » | ☐ |
| 3.4 | Départ demain 09:00, arrivée demain 11:00, puis **Planifier** | « Livraison n° X planifiée — commande passée En préparation » ; nouvelle ligne avec la pastille **Planifiée** | ☐ |
| 3.5 | **Commandes** : la commande Café Démo | Statut « En préparation », transporteur « Livreur TEST » (mis à jour automatiquement) | ☐ |
| 3.6 | **Commandes** : changer son statut en « Livrée », puis **Modifier** | Refusé : « Le statut de cette commande est géré par la livraison n° X » | ☐ |
| 3.7 | **Livraisons** : **Nouveau**, même livreur TEST, autre commande, même créneau (demain 09:30–10:30), puis **Planifier** | Refusé : « Livreur TEST a déjà une livraison active sur ce créneau » | ☐ |
| 3.8 | Cliquer sur la livraison Café Démo, statut **En cours**, puis **Enregistrer** | « Modification enregistrée » ; commande → « Expédiée » | ☐ |
| 3.9 | Statut **Échouée**, puis **Enregistrer** | Commande → « En attente » ; la commande réapparaît dans la liste « Commande » (**re-planification possible**) | ☐ |
| 3.10 | Statut **Livrée**, puis **Enregistrer** | Commande → « Livrée » ; date réelle de livraison enregistrée | ☐ |

---

## 4. Métiers basiques

| # | Action | Résultat attendu | ✅ |
|---|---|---|---|
| 4.1 | **Recherche** : taper `sami` | Seules les livraisons de Sami TRABELSI restent (filtrage immédiat) | ☐ |
| 4.2 | Effacer, puis **Statut = Planifiée** | Seules les livraisons planifiées | ☐ |
| 4.3 | **Zone = Ariana** | Filtre combiné statut + zone | ☐ |
| 4.4 | Remettre les filtres ; **Tri : départ (plus récent)**, puis **Tri : statut (urgences d'abord)** | Ordre modifié ; Retardée / En cours en premier | ☐ |
| 4.5 | Sélectionner une livraison → **Bordereau de livraison (PDF)** | PDF A4 : en-tête WaterFlow, tableau des informations, distance estimée, cadres de signature | ☐ |
| 4.6 | **Exporter la liste (PDF)** avec un filtre actif | PDF paysage avec les filtres rappelés et les statuts en couleur | ☐ |
| 4.7 | Onglet **Statistiques** | 4 indicateurs (délai moyen, durée moyenne, % < 48 h, ponctualité) + anneau par statut + barres par transporteur | ☐ |

---

## 5. Métiers innovants (onglet Tournées & ETA) — le point fort de la démo

| # | Action | Résultat attendu | ✅ |
|---|---|---|---|
| 5.1 | Livreur **Sami TRABELSI**, jour **aujourd'hui** | 5 arrêts ; tableau (km, arrivée prévue, ETA, écart) + schéma de la tournée | ☐ |
| 5.2 | Lire l'arrêt n° 1 | « Livrée », ETA = heure **réelle** (l'ETA tient compte de l'avancement réel) | ☐ |
| 5.3 | Repérer l'arrêt en orange | Écart « +xx min », compteur « ⚠ arrêt(s) en retard » | ☐ |
| 5.4 | **Optimiser l'itinéraire** | Nouvel ordre ; arrêts livrés / en cours conservés ; résumé « itinéraire optimisé » ; carte redessinée | ☐ |
| 5.5 | Attendre 1 minute | « dernier calcul à HH:mm » se met à jour : **recalcul automatique** | ☐ |
| 5.6 | **Valider la tournée** | « L'ordre de passage (5 arrêts, x km) est enregistré » | ☐ |
| 5.7 | **Alerter les clients en retard** | Fenêtre avec le message prêt à envoyer pour chaque client concerné | ☐ |
| 5.8 | **Confirmer et copier les messages** | Livraison(s) passée(s) « Retardée » ; messages copiés dans le presse-papiers (les coller dans le Bloc-notes pour le montrer) | ☐ |

> À expliquer au jury :
> - **Itinéraire** : distance routière estimée (haversine × 1,3). Pour 8 arrêts ou moins, toutes les permutations sont essayées, donc la solution est **exacte**. Au-delà : plus proche voisin + amélioration 2-opt. Le calcul pénalise aussi les retards par rapport aux heures promises.
> - **ETA** : vitesse moyenne 35 km/h + 10 min par arrêt. On repart de l'heure réelle du dernier arrêt livré et on ne remonte jamais avant l'heure actuelle. Une alerte est levée au-delà de 15 min de retard.

---

## 6. Traçabilité

| # | Action | Résultat attendu | ✅ |
|---|---|---|---|
| 6.1 | Onglet **Traçabilité** | Toutes les actions des étapes 3 et 5 : date, n°, action, ancien → nouveau statut, utilisateur **admin** | ☐ |
| 6.2 | Rechercher `Alerte` | Seulement les alertes clients | ☐ |
| 6.3 | Planification : sélectionner une livraison, puis cocher « Livraison sélectionnée uniquement » | Historique de cette livraison uniquement | ☐ |

---

## 7. Suppression et nettoyage

| # | Action | Résultat attendu | ✅ |
|---|---|---|---|
| 7.1 | Supprimer la livraison Café Démo → **Annuler** | Rien n'est supprimé | ☐ |
| 7.2 | Supprimer → **Supprimer** | Ligne supprimée ; « Suppression » visible dans Traçabilité | ☐ |
| 7.3 | **Personnel** : supprimer un livreur qui a des livraisons (ex. Sami) | Message clair : « lié à N livraison(s)… passez-le plutôt au statut Suspendu » | ☐ |
| 7.4 | Supprimer la commande Café Démo puis l'employé TEST | Suppressions acceptées | ☐ |

---

## 8. Corrections de bugs à montrer si on te le demande

| Bug corrigé | Comment le montrer |
|---|---|
| Statuts de Livraisons tous verts (conflit entre deux classes `StatutDelegate`) | Pastilles de couleurs différentes : bleu / cyan / vert / orange / rouge |
| Une livraison échouée bloquait sa commande | Étape 3.9 |
| Commandes : « Vider » puis « Modifier » écrasait l'ancienne commande | Commandes : cliquer une ligne → **Vider** → **Modifier** → « Veuillez sélectionner une commande » |
| Chiffre d'affaires comptant les commandes annulées | Commandes : le total n'inclut plus la commande « Annulée » |
| Suppression d'un employé lié : erreur Oracle brute | Étape 7.3 |

---

## Questions probables du jury (réponses courtes)

- **Où est le SQL ?** Uniquement dans le modèle : `livraison.cpp` et `historiquelivraison.cpp`. `livraisons.cpp` (contrôleur) n'en contient aucun. `itineraire.cpp` est de la logique pure, testable sans base.
- **Pourquoi une transaction ?** Créer ou modifier une livraison touche LIVRAISON + REGROUPE + COMMANDE + HISTORIQUE : soit tout réussit, soit rien (`rollback`).
- **Requêtes préparées ?** Oui partout (`prepare` / `bindValue`), donc pas d'injection SQL.
- **Comment les tables ont-elles été créées ?** Au premier lancement, l'application crée `HISTORIQUE_LIVRAISON`, la colonne `ORDRE_TOURNEE` et des index (`sql/04_livraison_avance.sql`).
- **Tests ?** `tests/test_integration.pro` : 37 vérifications automatiques (CRUD, règles métier, itinéraire, ETA, traçabilité).
