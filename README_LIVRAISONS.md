# Module 5 — Logistique de Livraison (Ayette Ben Mohamed)

## Correspondance avec le cahier de spécification

| Exigence du cahier | Où la montrer dans l'application |
|---|---|
| **CRUD** avec contrôle de saisie | Onglet *Planification* : Nouveau / Planifier / Enregistrer / Supprimer (confirmation). Immatriculation vérifiée par `QRegularExpression` (`123 TUN 4567`) |
| **Basique 1** : trier par date ou statut | Liste « Tri » : n°, départ récent/ancien, statut (urgences d'abord), livreur |
| **Basique 2** : rechercher / filtrer (transporteur, zone, statut) | Barre de recherche (mise à jour immédiate) + filtres Statut et Zone |
| **Basique 3** : export PDF | « Bordereau de livraison (PDF) » (une livraison) et « Exporter la liste (PDF) » (filtres appliqués) |
| **Basique 4** : statistiques en graphiques | Onglet *Statistiques* : délai moyen, durée moyenne, % livrées en moins de 48 h (valeur de référence du cahier), ponctualité, anneau par statut, barres par transporteur (Qt Charts) |
| **Innovant 1** : optimisation des itinéraires | Onglet *Tournées & ETA* → « Optimiser l'itinéraire » : ordre de passage exact (toutes les permutations jusqu'à 8 arrêts, sinon plus proche voisin + 2-opt), en tenant compte des heures promises. « Valider la tournée » enregistre l'ordre |
| **Innovant 2** : ETA dynamique + alerte client | ETA recalculée chaque minute selon l'avancement réel (arrêts livrés, heure actuelle). Les retards sont signalés en orange ; « Alerter les clients en retard » passe la livraison à « Retardée » et prépare les messages |
| Traçabilité (exigence non fonctionnelle) | Onglet *Traçabilité* : date, auteur, action, ancien/nouveau statut de chaque modification |
| Intégration des modules | Livreurs = employés actifs au poste « Livreur » (Personnel). Commandes = commandes à livrer (Commandes). Le statut et le transporteur de la commande suivent automatiquement la livraison |

## Architecture (MVC)
- **Modèle** : `livraison.cpp` (SQL du module), `historiquelivraison.cpp` (traçabilité), `itineraire.cpp` (algorithmes, sans SQL ni interface).
- **Vue** : `mainwindow.ui` (fichier `.ui` **unique** : tableau de bord + Commandes + Livraisons + Personnel ; widgets du module préfixés `liv`), `cartetournee.cpp` (schéma de la tournée), `style.h` (charte graphique du cahier).
- **Contrôleur** : `livraisons.cpp` (slots, aucune requête SQL).

## Préparer la démonstration
1. Lancer l'application une fois : elle crée automatiquement `HISTORIQUE_LIVRAISON`, la colonne `ORDRE_TOURNEE` et les index (voir `sql/04_livraison_avance.sql`).
2. **Le jour même**, charger la tournée de démo (les horaires sont calculés à partir de l'heure d'exécution du script) :
   `sqlplus wf/<mdp>@localhost:1521/XE @sql/05_demo_tournee.sql`
3. Dans *Livraisons > Tournées & ETA*, choisir « Sami TRABELSI » et la date du jour, puis cliquer sur **Optimiser** et **Alerter les clients en retard**.

Test automatique (37 vérifications) : `tests/test_integration.pro`.
