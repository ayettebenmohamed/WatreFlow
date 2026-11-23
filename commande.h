#ifndef COMMANDE_H
#define COMMANDE_H

#include <QString>
#include <QDate>
#include <QSqlQueryModel>


class Commande
{
    int id = 0;
    QString client, produits, adresse, statut, transporteur;
    int quantite = 0;
    QDate dateCommande, datePrevue;
    double montant = 0.0;
    QString m_erreur;

public:
    Commande() {}
    Commande(int id, QString client, QString produits, int quantite, QDate dateCommande,
             QString adresse, QDate datePrevue, QString statut, double montant,
             QString transporteur = QString());

    int getId() const { return id; }
    QString erreur() const { return m_erreur; }

    bool ajouter();
    bool modifier();
    bool supprimer(int id);

    // Lecture avec recherche (client / produit / adresse) et filtre statut.
    static QSqlQueryModel *afficher(const QString &texte = QString(),
                                    const QString &statut = QString(),
                                    bool triDateDesc = false);
    static int prochainId();

    static bool mettreAJourLivraison(int idCommande, const QString &statut, const QString &transporteur);
};

#endif
