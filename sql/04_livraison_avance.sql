-- 4) Évolutions du module Livraisons (traçabilité, tournées, performance) — schéma wf.
--    L'application crée ces objets automatiquement au premier lancement
--    (HistoriqueLivraison::preparerSchema) ; ce script sert de documentation
--    ou d'installation manuelle. À exécuter une seule fois.
--      set NLS_LANG=.AL32UTF8
--      sqlplus wf/<mdp>@localhost:1521/XE @04_livraison_avance.sql

-- Traçabilité : qui a fait quoi, quand, sur quelle livraison.
-- Pas de clé étrangère vers LIVRAISON : l'historique d'une livraison supprimée est conservé.
CREATE SEQUENCE SEQ_HISTORIQUE_LIVRAISON START WITH 1 INCREMENT BY 1 NOCACHE;

CREATE TABLE HISTORIQUE_LIVRAISON (
  ID_HISTORIQUE  NUMBER CONSTRAINT PK_HISTORIQUE_LIVRAISON PRIMARY KEY,
  ID_LIVRAISON   NUMBER NOT NULL,
  DATE_ACTION    TIMESTAMP DEFAULT SYSTIMESTAMP NOT NULL,
  UTILISATEUR    VARCHAR2(50 CHAR),
  ACTION         VARCHAR2(30 CHAR) NOT NULL,     -- Création, Changement de statut, Itinéraire optimisé, Alerte client...
  ANCIEN_STATUT  VARCHAR2(20 CHAR),
  NOUVEAU_STATUT VARCHAR2(20 CHAR),
  DETAILS        VARCHAR2(400 CHAR)
);
CREATE INDEX IDX_HIST_LIVRAISON ON HISTORIQUE_LIVRAISON (ID_LIVRAISON, DATE_ACTION);

-- Optimisation des itinéraires : rang de passage dans la tournée du livreur
ALTER TABLE LIVRAISON ADD (ORDRE_TOURNEE NUMBER);

-- Performance (exigence non fonctionnelle « requêtes indexées »)
CREATE INDEX IDX_LIVRAISON_EMPLOYE_DATE ON LIVRAISON (ID_EMPLOYE, DATE_DEPART_PREVUE);
CREATE INDEX IDX_REGROUPE_COMMANDE ON REGROUPE (ID_COMMANDE);

exit
