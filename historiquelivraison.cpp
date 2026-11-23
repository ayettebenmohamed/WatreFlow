#include "historiquelivraison.h"
#include "session.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <utility>

namespace
{
bool compte(const QString &sql, const QString &nom)
{
    QSqlQuery q;
    q.prepare(sql);
    q.bindValue(":n", nom);
    return q.exec() && q.next() && q.value(0).toInt() > 0;
}

bool executer(const QString &ddl, QString *erreur)
{
    QSqlQuery q;
    if (q.exec(ddl)) return true;
    if (erreur) *erreur = q.lastError().text();
    qDebug() << "HistoriqueLivraison::preparerSchema" << ddl.left(60) << q.lastError().text();
    return false;
}

bool g_disponible = false;
} // namespace

bool HistoriqueLivraison::preparerSchema(QString *erreur)
{
    bool ok = true;
    if (!compte("SELECT COUNT(*) FROM USER_SEQUENCES WHERE SEQUENCE_NAME = :n", "SEQ_HISTORIQUE_LIVRAISON"))
        ok &= executer("CREATE SEQUENCE SEQ_HISTORIQUE_LIVRAISON START WITH 1 INCREMENT BY 1 NOCACHE", erreur);

    if (!compte("SELECT COUNT(*) FROM USER_TABLES WHERE TABLE_NAME = :n", "HISTORIQUE_LIVRAISON"))
        ok &= executer("CREATE TABLE HISTORIQUE_LIVRAISON ("
                       " ID_HISTORIQUE  NUMBER CONSTRAINT PK_HISTORIQUE_LIVRAISON PRIMARY KEY,"
                       " ID_LIVRAISON   NUMBER NOT NULL,"
                       " DATE_ACTION    TIMESTAMP DEFAULT SYSTIMESTAMP NOT NULL,"
                       " UTILISATEUR    VARCHAR2(50 CHAR),"
                       " ACTION         VARCHAR2(30 CHAR) NOT NULL,"
                       " ANCIEN_STATUT  VARCHAR2(20 CHAR),"
                       " NOUVEAU_STATUT VARCHAR2(20 CHAR),"
                       " DETAILS        VARCHAR2(400 CHAR))", erreur);

    if (!compte("SELECT COUNT(*) FROM USER_INDEXES WHERE INDEX_NAME = :n", "IDX_HIST_LIVRAISON"))
        executer("CREATE INDEX IDX_HIST_LIVRAISON ON HISTORIQUE_LIVRAISON (ID_LIVRAISON, DATE_ACTION)", nullptr);

    if (!compte("SELECT COUNT(*) FROM USER_TAB_COLUMNS WHERE TABLE_NAME = 'LIVRAISON' AND COLUMN_NAME = :n",
                "ORDRE_TOURNEE"))
        ok &= executer("ALTER TABLE LIVRAISON ADD (ORDRE_TOURNEE NUMBER)", erreur);

    // Index de performance (exigence « requêtes SQL indexées »)
    if (!compte("SELECT COUNT(*) FROM USER_INDEXES WHERE INDEX_NAME = :n", "IDX_LIVRAISON_EMPLOYE_DATE"))
        executer("CREATE INDEX IDX_LIVRAISON_EMPLOYE_DATE ON LIVRAISON (ID_EMPLOYE, DATE_DEPART_PREVUE)", nullptr);
    if (!compte("SELECT COUNT(*) FROM USER_INDEXES WHERE INDEX_NAME = :n", "IDX_REGROUPE_COMMANDE"))
        executer("CREATE INDEX IDX_REGROUPE_COMMANDE ON REGROUPE (ID_COMMANDE)", nullptr);

    g_disponible = compte("SELECT COUNT(*) FROM USER_TABLES WHERE TABLE_NAME = :n", "HISTORIQUE_LIVRAISON");
    return ok && g_disponible;
}

bool HistoriqueLivraison::disponible()
{
    return g_disponible;
}

bool HistoriqueLivraison::enregistrer(int idLivraison, const QString &action, const QString &ancienStatut,
                                      const QString &nouveauStatut, const QString &details)
{
    if (!g_disponible) return true;          // traçabilité indisponible : on ne bloque pas le métier
    QSqlQuery q;
    q.prepare("INSERT INTO HISTORIQUE_LIVRAISON (ID_HISTORIQUE, ID_LIVRAISON, UTILISATEUR, ACTION, "
              "ANCIEN_STATUT, NOUVEAU_STATUT, DETAILS) "
              "VALUES (SEQ_HISTORIQUE_LIVRAISON.NEXTVAL, :id, :u, :a, :ancien, :nouveau, :d)");
    q.bindValue(":id", idLivraison);
    q.bindValue(":u", Session::utilisateur());
    q.bindValue(":a", action);
    q.bindValue(":ancien", ancienStatut.isEmpty() ? QVariant() : QVariant(ancienStatut));
    q.bindValue(":nouveau", nouveauStatut.isEmpty() ? QVariant() : QVariant(nouveauStatut));
    q.bindValue(":d", details.left(400));
    if (!q.exec()) {
        qDebug() << "HistoriqueLivraison::enregistrer" << q.lastError().text();
        return false;
    }
    return true;
}

QSqlQueryModel *HistoriqueLivraison::afficher(int idLivraison, const QString &texte)
{
    auto *model = new QSqlQueryModel();
    if (!g_disponible) return model;

    QString sql = "SELECT TO_CHAR(DATE_ACTION, 'DD/MM/YYYY HH24:MI:SS'), ID_LIVRAISON, ACTION, "
                  "ANCIEN_STATUT, NOUVEAU_STATUT, UTILISATEUR, DETAILS "
                  "FROM HISTORIQUE_LIVRAISON WHERE 1 = 1";
    const QString t = texte.trimmed().toLower();
    if (idLivraison > 0) sql += " AND ID_LIVRAISON = :id";
    if (!t.isEmpty())
        sql += " AND (LOWER(ACTION) LIKE :t1 OR LOWER(DETAILS) LIKE :t2 OR LOWER(UTILISATEUR) LIKE :t3 "
               "OR TO_CHAR(ID_LIVRAISON) = :t4)";
    sql += " ORDER BY DATE_ACTION DESC, ID_HISTORIQUE DESC";

    QSqlQuery q;
    q.prepare(sql);
    if (idLivraison > 0) q.bindValue(":id", idLivraison);
    if (!t.isEmpty()) {
        const QString motif = "%" + t + "%";
        q.bindValue(":t1", motif); q.bindValue(":t2", motif); q.bindValue(":t3", motif);
        q.bindValue(":t4", t);
    }
    if (!q.exec()) qDebug() << "HistoriqueLivraison::afficher" << q.lastError().text();
    model->setQuery(std::move(q));
    while (model->canFetchMore()) model->fetchMore();

    const QStringList titres = {"Date", "Livraison", "Action", "Ancien statut", "Nouveau statut",
                                "Utilisateur", "Détails"};
    for (int i = 0; i < titres.size(); ++i)
        model->setHeaderData(i, Qt::Horizontal, titres[i]);
    return model;
}
