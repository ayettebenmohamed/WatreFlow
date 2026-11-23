#pragma once
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QList>
#include <QVector>
#include <QSqlQueryModel>
#include "itineraire.h"

// Élément d'une liste déroulante alimentée par un autre module (livreur, commande)
struct ChoixListe {
    int id;
    QString libelle;
    QString extra;
};

// Statistiques du module (métier basique n° 4)
struct StatsLivraison {
    struct ParLivreur { QString nom; int total = 0; int livrees = 0; int retards = 0; };

    int total = 0;
    QList<QPair<QString, int>> parStatut;     // dans l'ordre de Livraison::STATUTS
    QList<ParLivreur> parLivreur;
    int livrees = 0;
    int livreesALHeure = 0;                   // réelle <= prévue
    int livreesDansCible = 0;                 // commande -> livraison <= délai cible (48 h)
    double delaiMoyenHeures = -1;             // commande -> livraison réelle (moyenne)
    double dureeMoyenneMinutes = -1;          // départ prévu -> livraison réelle (moyenne)
};

// Colonnes renvoyées par Livraison::rechercher()
enum ColonneLivraison {
    COL_ID, COL_COMMANDE, COL_CLIENT, COL_LIVREUR, COL_ZONE, COL_DEPART, COL_STATUT,
    COL_ARRIVEE, COL_VEHICULE, COL_ADR_DEPART, COL_ADR_LIVRAISON, COL_ID_LIVREUR, COL_REELLE,
    NB_COLONNES_LIVRAISON
};

class Livraison
{
    int id = 0;
    int idCommande = 0;
    int idEmploye = 0;
    QString livreur;
    QString vehicule, zone, adrDepart, adrLivraison, statut;
    QDateTime dateDepart, dateArrivee;
    QString erreurSql;

public:
    enum Tri { TriNumero, TriDepartRecent, TriDepartAncien, TriStatut, TriLivreur };

    static const QString POSTE_LIVREUR;
    static const QStringList STATUTS;          // Planifiée, En cours, Livrée, Retardée, Échouée
    static constexpr int DELAI_CIBLE_HEURES = 48;   // valeur de référence du cahier

    Livraison() {}
    Livraison(int id, int idCommande, int idEmploye, const QString &livreur, const QString &vehicule,
              const QString &zone, const QString &adrDepart, const QString &adrLivraison,
              const QDateTime &dateDepart, const QDateTime &dateArrivee, const QString &statut);

    QString getErreurSql() const       { return erreurSql; }
    int getId() const                  { return id; }
    int getIdCommande() const          { return idCommande; }
    int getIdEmploye() const           { return idEmploye; }
    QString getStatut() const          { return statut; }

    // Contrôle de saisie (règles métier) : chaîne vide si tout est correct
    QString verifier(bool creation = false) const;

    // CRUD
    bool ajouter();
    bool modifier();
    bool supprimer(int id);
    static QSqlQueryModel *rechercher(const QString &texte, const QString &statut,
                                      const QString &zone = QString(), Tri tri = TriNumero);

    // Changement de statut seul (suivi terrain / alerte ETA), commande synchronisée
    static bool changerStatut(int idLivraison, const QString &nouveauStatut, const QString &motif,
                              QString *erreur = nullptr);

    // Métier basique n° 4 : statistiques
    static StatsLivraison statistiques();

    // Métiers innovants : tournée d'un livreur pour un jour, et enregistrement de l'ordre optimisé
    static QVector<Etape> tournee(int idEmploye, const QDate &jour);
    static bool enregistrerItineraire(const QVector<Etape> &ordre, double km, QString *erreur = nullptr);
    static int nombreEnRetard();               // livraisons actives dont l'heure prévue est dépassée

    // Données des deux autres modules
    static QList<ChoixListe> listeLivreurs();                       // Personnel : livreurs actifs
    static QList<ChoixListe> listeCommandes(int idCommandeEnCours); // Commandes à livrer (+ celle en cours)
    static QString statutCommandePour(const QString &statutLivraison);

private:
    bool livreurOccupe();                     // chevauchement avec une autre livraison active
    bool commandeDejaPlanifiee();             // la commande a déjà une autre livraison active
    bool enregistrer(bool creation);          // INSERT ou UPDATE dans une transaction
};
