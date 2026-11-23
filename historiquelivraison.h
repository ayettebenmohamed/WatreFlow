#pragma once
#include <QString>
#include <QSqlQueryModel>

// Traçabilité des livraisons (exigence non fonctionnelle « Traçabilité » du cahier) :
// chaque création, modification de statut, optimisation de tournée, alerte client
// ou suppression est enregistrée avec sa date et son auteur.
class HistoriqueLivraison
{
public:
    // Crée, si besoin, les objets SQL du module (table HISTORIQUE_LIVRAISON, séquence,
    // colonne LIVRAISON.ORDRE_TOURNEE, index). Idempotent : sans effet s'ils existent déjà.
    static bool preparerSchema(QString *erreur = nullptr);
    static bool disponible();

    // Enregistre une action (à appeler dans la même transaction que la modification).
    static bool enregistrer(int idLivraison, const QString &action,
                            const QString &ancienStatut = QString(),
                            const QString &nouveauStatut = QString(),
                            const QString &details = QString());

    // Lecture : idLivraison = 0 -> toutes les livraisons ; texte = recherche libre
    static QSqlQueryModel *afficher(int idLivraison = 0, const QString &texte = QString());
};
