-- 5) Données de DÉMONSTRATION du module Livraisons (validation des métiers avancés).
--    Crée une tournée « aujourd'hui » pour le livreur Sami TRABELSI : 5 arrêts dans le Grand Tunis,
--    horaires calculés à partir de l'heure d'exécution (un arrêt déjà livré, un en cours,
--    un arrêt qui sera en retard -> alerte ETA).
--    Identifiants réservés 901 à 905 : le script peut être relancé, il remplace ses propres données.
--
--    À exécuter le jour de la démonstration (schéma wf) :
--      set NLS_LANG=.AL32UTF8
--      sqlplus wf/<mdp>@localhost:1521/XE @05_demo_tournee.sql
--    Dans l'application : Livraisons > Tournées & ETA > livreur « Sami TRABELSI » > jour = aujourd'hui.
--
--    Pour supprimer les données de démonstration : exécuter uniquement le bloc « Nettoyage » ci-dessous.

-- Nettoyage ------------------------------------------------------------------
DELETE FROM REGROUPE  WHERE ID_LIVRAISON BETWEEN 901 AND 905 OR ID_COMMANDE BETWEEN 901 AND 905;
DELETE FROM LIVRAISON WHERE ID_LIVRAISON BETWEEN 901 AND 905;
DELETE FROM CONTIENT  WHERE ID_COMMANDE BETWEEN 901 AND 905;
DELETE FROM COMMANDE  WHERE ID_COMMANDE BETWEEN 901 AND 905;
BEGIN
  EXECUTE IMMEDIATE 'DELETE FROM HISTORIQUE_LIVRAISON WHERE ID_LIVRAISON BETWEEN 901 AND 905';
EXCEPTION WHEN OTHERS THEN NULL;   -- table créée au premier lancement de l'application
END;
/

-- Commandes --------------------------------------------------------------------
INSERT INTO COMMANDE VALUES (901, 'Hôtel Les Berges du Lac', SYSTIMESTAMP - 1, 'Bonbonne 19L', 30, 'Tunis, Lac 2, rue du Lac Biwa',
                             SYSTIMESTAMP, 'Livrée', 'Sami TRABELSI', 390);
INSERT INTO COMMANDE VALUES (902, 'Pharmacie Ennasr', SYSTIMESTAMP - 1, 'Pack eau 0.5L x12', 80, 'Ariana, avenue Hédi Nouira',
                             SYSTIMESTAMP, 'Expédiée', 'Sami TRABELSI', 560);
INSERT INTO COMMANDE VALUES (903, 'Café Sidi Bou', SYSTIMESTAMP - 1, 'Pack eau 1.5L x6', 45, 'La Marsa, corniche',
                             SYSTIMESTAMP, 'En préparation', 'Sami TRABELSI', 315);
INSERT INTO COMMANDE VALUES (904, 'Superette Mégrine', SYSTIMESTAMP - 1, 'Pack eau 1.5L x6', 120, 'Mégrine, rue de Marseille',
                             SYSTIMESTAMP, 'En préparation', 'Sami TRABELSI', 840);
INSERT INTO COMMANDE VALUES (905, 'Clinique Ezzahra', SYSTIMESTAMP - 1, 'Bonbonne 19L', 20, 'Ezzahra, boulevard de l''environnement',
                             SYSTIMESTAMP, 'En préparation', 'Sami TRABELSI', 260);

-- Livraisons (même départ pour toute la tournée, ordre non optimisé) ----------
INSERT INTO LIVRAISON (ID_LIVRAISON, ID_EMPLOYE, TRANSPORTEUR_CHAUFFEUR, VEHICULE, ZONE_GEOGRAPHIQUE, ADRESSE_DEPART,
                       ADRESSE_LIVRAISON, DATE_DEPART_PREVUE, DATE_LIVRAISON_PREVUE, DATE_LIVRAISON_REELLE, STATUT)
SELECT 901, ID_EMPLOYE, 'Sami TRABELSI', '145 TUN 8910', 'Tunis Nord', 'Dépôt WaterFlow', 'Tunis, Lac 2, rue du Lac Biwa',
       SYSTIMESTAMP - INTERVAL '90' MINUTE, SYSTIMESTAMP - INTERVAL '60' MINUTE, SYSTIMESTAMP - INTERVAL '55' MINUTE, 'Livrée'
FROM EMPLOYE WHERE UPPER(NOM) = 'TRABELSI' AND ROWNUM = 1;

INSERT INTO LIVRAISON (ID_LIVRAISON, ID_EMPLOYE, TRANSPORTEUR_CHAUFFEUR, VEHICULE, ZONE_GEOGRAPHIQUE, ADRESSE_DEPART,
                       ADRESSE_LIVRAISON, DATE_DEPART_PREVUE, DATE_LIVRAISON_PREVUE, STATUT)
SELECT 902, ID_EMPLOYE, 'Sami TRABELSI', '145 TUN 8910', 'Ariana', 'Dépôt WaterFlow', 'Ariana, avenue Hédi Nouira',
       SYSTIMESTAMP - INTERVAL '90' MINUTE, SYSTIMESTAMP + INTERVAL '25' MINUTE, 'En cours'
FROM EMPLOYE WHERE UPPER(NOM) = 'TRABELSI' AND ROWNUM = 1;

INSERT INTO LIVRAISON (ID_LIVRAISON, ID_EMPLOYE, TRANSPORTEUR_CHAUFFEUR, VEHICULE, ZONE_GEOGRAPHIQUE, ADRESSE_DEPART,
                       ADRESSE_LIVRAISON, DATE_DEPART_PREVUE, DATE_LIVRAISON_PREVUE, STATUT)
SELECT 903, ID_EMPLOYE, 'Sami TRABELSI', '145 TUN 8910', 'La Marsa', 'Dépôt WaterFlow', 'La Marsa, corniche',
       SYSTIMESTAMP - INTERVAL '90' MINUTE, SYSTIMESTAMP + INTERVAL '30' MINUTE, 'Planifiée'
FROM EMPLOYE WHERE UPPER(NOM) = 'TRABELSI' AND ROWNUM = 1;

INSERT INTO LIVRAISON (ID_LIVRAISON, ID_EMPLOYE, TRANSPORTEUR_CHAUFFEUR, VEHICULE, ZONE_GEOGRAPHIQUE, ADRESSE_DEPART,
                       ADRESSE_LIVRAISON, DATE_DEPART_PREVUE, DATE_LIVRAISON_PREVUE, STATUT)
SELECT 904, ID_EMPLOYE, 'Sami TRABELSI', '145 TUN 8910', 'Ben Arous', 'Dépôt WaterFlow', 'Mégrine, rue de Marseille',
       SYSTIMESTAMP - INTERVAL '90' MINUTE, SYSTIMESTAMP + INTERVAL '180' MINUTE, 'Planifiée'
FROM EMPLOYE WHERE UPPER(NOM) = 'TRABELSI' AND ROWNUM = 1;

INSERT INTO LIVRAISON (ID_LIVRAISON, ID_EMPLOYE, TRANSPORTEUR_CHAUFFEUR, VEHICULE, ZONE_GEOGRAPHIQUE, ADRESSE_DEPART,
                       ADRESSE_LIVRAISON, DATE_DEPART_PREVUE, DATE_LIVRAISON_PREVUE, STATUT)
SELECT 905, ID_EMPLOYE, 'Sami TRABELSI', '145 TUN 8910', 'Ben Arous', 'Dépôt WaterFlow', 'Ezzahra, boulevard de l''environnement',
       SYSTIMESTAMP - INTERVAL '90' MINUTE, SYSTIMESTAMP + INTERVAL '240' MINUTE, 'Planifiée'
FROM EMPLOYE WHERE UPPER(NOM) = 'TRABELSI' AND ROWNUM = 1;

INSERT INTO REGROUPE VALUES (901, 901);
INSERT INTO REGROUPE VALUES (902, 902);
INSERT INTO REGROUPE VALUES (903, 903);
INSERT INTO REGROUPE VALUES (904, 904);
INSERT INTO REGROUPE VALUES (905, 905);

-- Historique (traçabilité) de la tournée de démonstration
BEGIN
  FOR h IN (SELECT 901 id, 'Création' a, NULL ancien, 'Planifiée' nouveau, 'Commande n° 901, livreur Sami TRABELSI' d, 180 m FROM DUAL UNION ALL
            SELECT 902, 'Création', NULL, 'Planifiée', 'Commande n° 902, livreur Sami TRABELSI', 179 FROM DUAL UNION ALL
            SELECT 903, 'Création', NULL, 'Planifiée', 'Commande n° 903, livreur Sami TRABELSI', 178 FROM DUAL UNION ALL
            SELECT 901, 'Changement de statut', 'Planifiée', 'En cours', 'Départ du dépôt', 90 FROM DUAL UNION ALL
            SELECT 901, 'Changement de statut', 'En cours', 'Livrée', 'Commande n° 901 -> « Livrée »', 55 FROM DUAL UNION ALL
            SELECT 902, 'Changement de statut', 'Planifiée', 'En cours', 'Commande n° 902 -> « Expédiée »', 50 FROM DUAL) LOOP
    EXECUTE IMMEDIATE 'INSERT INTO HISTORIQUE_LIVRAISON (ID_HISTORIQUE, ID_LIVRAISON, DATE_ACTION, UTILISATEUR, ACTION, '
                   || 'ANCIEN_STATUT, NOUVEAU_STATUT, DETAILS) VALUES (SEQ_HISTORIQUE_LIVRAISON.NEXTVAL, :1, '
                   || 'SYSTIMESTAMP - NUMTODSINTERVAL(:2, ''MINUTE''), ''admin'', :3, :4, :5, :6)'
      USING h.id, h.m, h.a, h.ancien, h.nouveau, h.d;
  END LOOP;
EXCEPTION WHEN OTHERS THEN NULL;   -- table absente : lancer d'abord l'application (ou 04_livraison_avance.sql)
END;
/

COMMIT;
exit
