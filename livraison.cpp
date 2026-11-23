#include "livraison.h"
#include "commande.h"
#include "historiquelivraison.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QRegularExpression>
#include <QDebug>
#include <utility>

const QString Livraison::POSTE_LIVREUR = "Livreur";
const QStringList Livraison::STATUTS = {"Planifiée", "En cours", "Livrée", "Retardée", "Échouée"};

// Colonnes : voir enum ColonneLivraison (livraison.h)
static const QString SELECT_BASE =
    "SELECT L.ID_LIVRAISON, R.ID_COMMANDE, C.CLIENT, "
    "NVL(TRIM(E.PRENOM || ' ' || E.NOM), L.TRANSPORTEUR_CHAUFFEUR), L.ZONE_GEOGRAPHIQUE, "
    "TO_CHAR(L.DATE_DEPART_PREVUE, 'DD/MM/YYYY HH24:MI'), L.STATUT, "
    "TO_CHAR(L.DATE_LIVRAISON_PREVUE, 'DD/MM/YYYY HH24:MI'), "
    "L.VEHICULE, L.ADRESSE_DEPART, L.ADRESSE_LIVRAISON, L.ID_EMPLOYE, "
    "TO_CHAR(L.DATE_LIVRAISON_REELLE, 'DD/MM/YYYY HH24:MI') "
    "FROM LIVRAISON L "
    "LEFT JOIN EMPLOYE E ON E.ID_EMPLOYE = L.ID_EMPLOYE "
    "LEFT JOIN REGROUPE R ON R.ID_LIVRAISON = L.ID_LIVRAISON "
    "LEFT JOIN COMMANDE C ON C.ID_COMMANDE = R.ID_COMMANDE";

// Ordre « métier » des statuts : les urgences d'abord
static const QString ORDRE_STATUT =
    "DECODE(L.STATUT, 'Retardée', 1, 'En cours', 2, 'Planifiée', 3, 'Échouée', 4, 'Livrée', 5, 6)";

static QSqlQueryModel *creerModele(QSqlQuery &&query)
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery(std::move(query));
    while (model->canFetchMore()) model->fetchMore();
    const QStringList titres = {"N°", "Commande", "Client", "Livreur", "Zone", "Départ prévu", "Statut",
                                "Livraison prévue", "Véhicule", "Adresse départ", "Adresse livraison",
                                "ID livreur", "Livrée le"};
    for (int i = 0; i < titres.size(); ++i)
        model->setHeaderData(i, Qt::Horizontal, titres[i]);
    return model;
}

Livraison::Livraison(int id, int idCommande, int idEmploye, const QString &livreur, const QString &vehicule,
                     const QString &zone, const QString &adrDepart, const QString &adrLivraison,
                     const QDateTime &dateDepart, const QDateTime &dateArrivee, const QString &statut)
    : id(id), idCommande(idCommande), idEmploye(idEmploye), livreur(livreur.trimmed()),
      vehicule(vehicule.trimmed().toUpper()), zone(zone), adrDepart(adrDepart.trimmed()),
      adrLivraison(adrLivraison.trimmed()), statut(statut),
      dateDepart(dateDepart), dateArrivee(dateArrivee)
{
}

// Statut de la commande qui découle du statut de sa livraison
QString Livraison::statutCommandePour(const QString &s)
{
    if (s == "Planifiée") return "En préparation";
    if (s == "En cours" || s == "Retardée") return "Expédiée";
    if (s == "Livrée") return "Livrée";
    return "En attente";                                   // Échouée : la commande est à relivrer
}

// ---------- CONTRÔLE DE SAISIE ----------
QString Livraison::verifier(bool creation) const
{
    // Immatriculation tunisienne : « 123 TUN 4567 » ou « RS 123456 »
    static const QRegularExpression reImmat("^(\\d{1,3}\\s?TUN\\s?\\d{1,4}|RS\\s?\\d{1,6})$");

    if (idEmploye <= 0)
        return "Choisissez un livreur (employé au poste « " + POSTE_LIVREUR + " » dans Personnel).";
    if (idCommande <= 0)
        return "Choisissez une commande à livrer.";
    if (vehicule.isEmpty())
        return "Saisissez l'immatriculation du véhicule.";
    if (!reImmat.match(vehicule).hasMatch())
        return "Immatriculation invalide : format attendu « 123 TUN 4567 » ou « RS 123456 ».";
    if (adrDepart.isEmpty() || adrDepart.size() > 200)
        return "Saisissez l'adresse de départ (200 caractères maximum).";
    if (adrLivraison.isEmpty() || adrLivraison.size() > 200)
        return "Saisissez l'adresse de livraison (200 caractères maximum).";
    if (!STATUTS.contains(statut))
        return "Statut de livraison inconnu.";
    if (!dateDepart.isValid() || !dateArrivee.isValid() || dateArrivee <= dateDepart)
        return "L'heure de livraison prévue doit être postérieure au départ.";
    if (dateDepart.secsTo(dateArrivee) > 24 * 3600)
        return "Une livraison ne peut pas durer plus de 24 heures.";
    if (creation && statut == "Planifiée" && dateDepart < QDateTime::currentDateTime().addSecs(-300))
        return "Le départ d'une nouvelle livraison planifiée ne peut pas être dans le passé.";
    if (creation && statut == "Livrée")
        return "Une nouvelle livraison ne peut pas être créée directement au statut « Livrée ».";
    return QString();
}

// Un livreur ne peut pas avoir deux livraisons actives qui se chevauchent
bool Livraison::livreurOccupe()
{
    if (statut == "Livrée" || statut == "Échouée") return false;
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM LIVRAISON WHERE ID_EMPLOYE = :e AND ID_LIVRAISON <> :id "
              "AND STATUT IN ('Planifiée', 'En cours', 'Retardée') "
              "AND DATE_DEPART_PREVUE < :fin AND DATE_LIVRAISON_PREVUE > :debut");
    q.bindValue(":e", idEmploye);
    q.bindValue(":id", id);
    q.bindValue(":fin", dateArrivee);
    q.bindValue(":debut", dateDepart);
    return q.exec() && q.next() && q.value(0).toInt() > 0;
}

// Une commande n'a qu'une seule livraison « vivante » (les livraisons échouées ne comptent pas)
bool Livraison::commandeDejaPlanifiee()
{
    if (statut == "Échouée") return false;
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM REGROUPE R JOIN LIVRAISON L ON L.ID_LIVRAISON = R.ID_LIVRAISON "
              "WHERE R.ID_COMMANDE = :c AND L.ID_LIVRAISON <> :id AND L.STATUT <> 'Échouée'");
    q.bindValue(":c", idCommande);
    q.bindValue(":id", id);
    return q.exec() && q.next() && q.value(0).toInt() > 0;
}

// ---------- CREATE / UPDATE (transaction : LIVRAISON + REGROUPE + COMMANDE + HISTORIQUE) ----------
bool Livraison::enregistrer(bool creation)
{
    erreurSql.clear();
    if (livreurOccupe()) {
        erreurSql = livreur + " a déjà une livraison active sur ce créneau.";
        return false;
    }
    if (commandeDejaPlanifiee()) {
        erreurSql = QString("La commande n° %1 a déjà une livraison en cours.").arg(idCommande);
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.transaction()) {
        erreurSql = "Transaction impossible : " + db.lastError().text();
        return false;
    }
    QSqlQuery q;
    auto echec = [&](const QString &etape) {
        erreurSql = etape + " : " + q.lastError().text();
        qDebug() << "Livraison::enregistrer" << erreurSql;
        db.rollback();
        if (creation) id = 0;
        return false;
    };

    int ancienneCommande = 0;
    QString ancienStatut;
    if (creation) {
        if (!q.exec("SELECT GREATEST(NVL((SELECT MAX(ID_LIVRAISON) FROM LIVRAISON), 0), "
                    "NVL((SELECT MAX(ID_LIVRAISON) FROM REGROUPE), 0)) + 1 FROM DUAL") || !q.next())
            return echec("Génération de l'identifiant");
        id = q.value(0).toInt();
        q.prepare("INSERT INTO LIVRAISON (ID_LIVRAISON, ID_EMPLOYE, TRANSPORTEUR_CHAUFFEUR, VEHICULE, "
                  "ZONE_GEOGRAPHIQUE, ADRESSE_DEPART, ADRESSE_LIVRAISON, DATE_DEPART_PREVUE, "
                  "DATE_LIVRAISON_PREVUE, STATUT, DATE_LIVRAISON_REELLE) "
                  "VALUES (:id, :emp, :livreur, :vehicule, :zone, :adrdep, :adrliv, :ddep, :darr, :statut, "
                  "CASE WHEN :st2 = 'Livrée' THEN SYSTIMESTAMP END)");
    } else {
        q.prepare("SELECT R.ID_COMMANDE, L.STATUT FROM LIVRAISON L "
                  "LEFT JOIN REGROUPE R ON R.ID_LIVRAISON = L.ID_LIVRAISON WHERE L.ID_LIVRAISON = :id");
        q.bindValue(":id", id);
        if (q.exec() && q.next()) {
            ancienneCommande = q.value(0).toInt();
            ancienStatut = q.value(1).toString();
        }

        q.prepare("UPDATE LIVRAISON SET ID_EMPLOYE = :emp, TRANSPORTEUR_CHAUFFEUR = :livreur, "
                  "VEHICULE = :vehicule, ZONE_GEOGRAPHIQUE = :zone, ADRESSE_DEPART = :adrdep, "
                  "ADRESSE_LIVRAISON = :adrliv, DATE_DEPART_PREVUE = :ddep, "
                  "DATE_LIVRAISON_PREVUE = :darr, STATUT = :statut, "
                  "DATE_LIVRAISON_REELLE = CASE WHEN :st2 = 'Livrée' "
                  "THEN NVL(DATE_LIVRAISON_REELLE, SYSTIMESTAMP) END "
                  "WHERE ID_LIVRAISON = :id");
    }
    q.bindValue(":id", id);
    q.bindValue(":emp", idEmploye);
    q.bindValue(":livreur", livreur);
    q.bindValue(":vehicule", vehicule);
    q.bindValue(":zone", zone);
    q.bindValue(":adrdep", adrDepart);
    q.bindValue(":adrliv", adrLivraison);
    q.bindValue(":ddep", dateDepart);
    q.bindValue(":darr", dateArrivee);
    q.bindValue(":statut", statut);
    q.bindValue(":st2", statut);
    if (!q.exec())
        return echec(creation ? "Ajout de la livraison" : "Modification de la livraison");
    if (!creation && q.numRowsAffected() == 0) {
        erreurSql = "Livraison introuvable.";
        db.rollback();
        return false;
    }

    // Lien avec la commande (table d'association REGROUPE)
    if (ancienneCommande != idCommande) {
        if (ancienneCommande > 0) {
            q.prepare("DELETE FROM REGROUPE WHERE ID_LIVRAISON = :id");
            q.bindValue(":id", id);
            if (!q.exec()) return echec("Lien commande");
            if (!Commande::mettreAJourLivraison(ancienneCommande, "En attente", QString())) {
                erreurSql = QString("Mise à jour de l'ancienne commande n° %1 impossible.").arg(ancienneCommande);
                db.rollback();
                return false;
            }
        }
        q.prepare("INSERT INTO REGROUPE (ID_LIVRAISON, ID_COMMANDE) VALUES (:id, :cmd)");
        q.bindValue(":id", id);
        q.bindValue(":cmd", idCommande);
        if (!q.exec()) return echec("Lien commande");
    }

    // La commande suit sa livraison : statut + transporteur assigné (vide si échec)
    if (!Commande::mettreAJourLivraison(idCommande, statutCommandePour(statut),
                                        statut == "Échouée" ? QString() : livreur)) {
        erreurSql = "Mise à jour de la commande impossible.";
        db.rollback();
        return false;
    }

    // Traçabilité
    if (creation) {
        HistoriqueLivraison::enregistrer(id, "Création", QString(), statut,
            QString("Commande n° %1, livreur %2, véhicule %3, départ %4")
                .arg(idCommande).arg(livreur, vehicule, dateDepart.toString("dd/MM/yyyy HH:mm")));
    } else if (ancienStatut != statut) {
        HistoriqueLivraison::enregistrer(id, "Changement de statut", ancienStatut, statut,
            QString("Commande n° %1 -> « %2 »").arg(idCommande).arg(statutCommandePour(statut)));
    } else {
        HistoriqueLivraison::enregistrer(id, "Modification", ancienStatut, statut,
            QString("Livreur %1, véhicule %2, départ %3, arrivée %4")
                .arg(livreur, vehicule, dateDepart.toString("dd/MM/yyyy HH:mm"),
                     dateArrivee.toString("dd/MM/yyyy HH:mm")));
    }

    if (!db.commit()) {
        erreurSql = db.lastError().text();
        db.rollback();
        if (creation) id = 0;
        return false;
    }
    return true;
}

bool Livraison::ajouter()  { return enregistrer(true); }
bool Livraison::modifier() { return enregistrer(false); }

// ---------- DELETE ----------
bool Livraison::supprimer(int idLiv)
{
    erreurSql.clear();
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q;

    int idCmd = 0;
    QString st, client;
    q.prepare("SELECT R.ID_COMMANDE, L.STATUT, C.CLIENT FROM LIVRAISON L "
              "LEFT JOIN REGROUPE R ON R.ID_LIVRAISON = L.ID_LIVRAISON "
              "LEFT JOIN COMMANDE C ON C.ID_COMMANDE = R.ID_COMMANDE WHERE L.ID_LIVRAISON = :id");
    q.bindValue(":id", idLiv);
    if (!q.exec() || !q.next()) {
        erreurSql = "Livraison introuvable.";
        return false;
    }
    idCmd = q.value(0).toInt();
    st = q.value(1).toString();
    client = q.value(2).toString();

    db.transaction();
    auto echec = [&](const QString &message) {
        erreurSql = message;
        db.rollback();
        return false;
    };

    q.prepare("DELETE FROM REGROUPE WHERE ID_LIVRAISON = :id");
    q.bindValue(":id", idLiv);
    if (!q.exec()) return echec("Suppression du lien commande : " + q.lastError().text());

    q.prepare("DELETE FROM LIVRAISON WHERE ID_LIVRAISON = :id");
    q.bindValue(":id", idLiv);
    if (!q.exec() || q.numRowsAffected() == 0)
        return echec("Suppression de la livraison : " + q.lastError().text());

    // Commande non livrée : elle redevient « En attente », sans transporteur
    if (idCmd > 0 && st != "Livrée" && !Commande::mettreAJourLivraison(idCmd, "En attente", QString()))
        return echec(QString("Mise à jour de la commande n° %1 impossible.").arg(idCmd));

    HistoriqueLivraison::enregistrer(idLiv, "Suppression", st, QString(),
                                     QString("Commande n° %1 (%2)").arg(idCmd).arg(client));
    if (!db.commit()) return echec(db.lastError().text());
    return true;
}

// ---------- CHANGEMENT DE STATUT (suivi terrain / alerte ETA) ----------
bool Livraison::changerStatut(int idLiv, const QString &nouveau, const QString &motif, QString *erreur)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q;
    q.prepare("SELECT L.STATUT, R.ID_COMMANDE, L.TRANSPORTEUR_CHAUFFEUR FROM LIVRAISON L "
              "LEFT JOIN REGROUPE R ON R.ID_LIVRAISON = L.ID_LIVRAISON WHERE L.ID_LIVRAISON = :id");
    q.bindValue(":id", idLiv);
    if (!q.exec() || !q.next()) {
        if (erreur) *erreur = "Livraison introuvable.";
        return false;
    }
    const QString ancien = q.value(0).toString();
    const int idCmd = q.value(1).toInt();
    const QString chauffeur = q.value(2).toString();
    if (ancien == nouveau) return true;

    db.transaction();
    q.prepare("UPDATE LIVRAISON SET STATUT = :s, DATE_LIVRAISON_REELLE = CASE WHEN :s2 = 'Livrée' "
              "THEN NVL(DATE_LIVRAISON_REELLE, SYSTIMESTAMP) END WHERE ID_LIVRAISON = :id");
    q.bindValue(":s", nouveau);
    q.bindValue(":s2", nouveau);
    q.bindValue(":id", idLiv);
    bool ok = q.exec();
    if (ok && idCmd > 0)
        ok = Commande::mettreAJourLivraison(idCmd, statutCommandePour(nouveau),
                                            nouveau == "Échouée" ? QString() : chauffeur);
    if (ok)
        HistoriqueLivraison::enregistrer(idLiv, "Changement de statut", ancien, nouveau, motif);
    if (!ok || !db.commit()) {
        if (erreur) *erreur = q.lastError().text();
        db.rollback();
        return false;
    }
    return true;
}

// ---------- RECHERCHE + FILTRES + TRI (métiers basiques 1 et 2) ----------
QSqlQueryModel *Livraison::rechercher(const QString &texte, const QString &statutFiltre,
                                      const QString &zoneFiltre, Tri tri)
{
    const QString t = texte.trimmed();
    const bool filtreStatut = !statutFiltre.isEmpty() && STATUTS.contains(statutFiltre);
    const bool filtreZone = !zoneFiltre.isEmpty() && !zoneFiltre.startsWith("Toutes");

    QStringList conditions;
    if (!t.isEmpty())
        conditions << "(LOWER(E.PRENOM || ' ' || E.NOM) LIKE :t1 OR LOWER(L.TRANSPORTEUR_CHAUFFEUR) LIKE :t2 "
                      "OR LOWER(L.ZONE_GEOGRAPHIQUE) LIKE :t3 OR LOWER(C.CLIENT) LIKE :t4 "
                      "OR LOWER(L.VEHICULE) LIKE :t5 OR TO_CHAR(L.ID_LIVRAISON) = :t6)";
    if (filtreStatut)
        conditions << "L.STATUT = :statut";
    if (filtreZone)
        conditions << "L.ZONE_GEOGRAPHIQUE = :zone";

    QString sql = SELECT_BASE;
    if (!conditions.isEmpty())
        sql += " WHERE " + conditions.join(" AND ");
    switch (tri) {
    case TriDepartRecent: sql += " ORDER BY L.DATE_DEPART_PREVUE DESC, L.ID_LIVRAISON"; break;
    case TriDepartAncien: sql += " ORDER BY L.DATE_DEPART_PREVUE ASC, L.ID_LIVRAISON"; break;
    case TriStatut:       sql += " ORDER BY " + ORDRE_STATUT + ", L.DATE_DEPART_PREVUE"; break;
    case TriLivreur:      sql += " ORDER BY 4, L.DATE_DEPART_PREVUE"; break;
    default:              sql += " ORDER BY L.ID_LIVRAISON"; break;
    }

    QSqlQuery query;
    query.prepare(sql);
    if (!t.isEmpty()) {
        const QString motif = "%" + t.toLower() + "%";
        for (const char *p : {":t1", ":t2", ":t3", ":t4", ":t5"})
            query.bindValue(p, motif);
        query.bindValue(":t6", t);
    }
    if (filtreStatut) query.bindValue(":statut", statutFiltre);
    if (filtreZone) query.bindValue(":zone", zoneFiltre);

    if (!query.exec())
        qDebug() << "Livraison::rechercher :" << query.lastError().text();
    return creerModele(std::move(query));
}

// ---------- STATISTIQUES (métier basique 4) ----------
StatsLivraison Livraison::statistiques()
{
    StatsLivraison s;
    QSqlQuery q;

    QHash<QString, int> parStatut;
    if (q.exec("SELECT STATUT, COUNT(*) FROM LIVRAISON GROUP BY STATUT"))
        while (q.next()) {
            parStatut[q.value(0).toString()] = q.value(1).toInt();
            s.total += q.value(1).toInt();
        }
    for (const QString &st : STATUTS)
        s.parStatut.append({st, parStatut.value(st)});

    if (q.exec("SELECT NVL(TRIM(E.PRENOM || ' ' || E.NOM), L.TRANSPORTEUR_CHAUFFEUR), COUNT(*), "
               "SUM(CASE WHEN L.STATUT = 'Livrée' THEN 1 ELSE 0 END), "
               "SUM(CASE WHEN L.STATUT = 'Retardée' OR (L.STATUT = 'Livrée' "
               "AND L.DATE_LIVRAISON_REELLE > L.DATE_LIVRAISON_PREVUE) THEN 1 ELSE 0 END) "
               "FROM LIVRAISON L LEFT JOIN EMPLOYE E ON E.ID_EMPLOYE = L.ID_EMPLOYE "
               "GROUP BY NVL(TRIM(E.PRENOM || ' ' || E.NOM), L.TRANSPORTEUR_CHAUFFEUR) ORDER BY 2 DESC, 1"))
        while (q.next())
            s.parLivreur.append({q.value(0).toString(), q.value(1).toInt(), q.value(2).toInt(), q.value(3).toInt()});

    // Délais : CAST en DATE -> la différence est un nombre de jours
    if (q.exec(QString("SELECT COUNT(*), "
               "SUM(CASE WHEN L.DATE_LIVRAISON_REELLE <= L.DATE_LIVRAISON_PREVUE THEN 1 ELSE 0 END), "
               "SUM(CASE WHEN (CAST(L.DATE_LIVRAISON_REELLE AS DATE) - CAST(C.DATE_COMMANDE AS DATE)) * 24 <= %1 "
               "THEN 1 ELSE 0 END), "
               "AVG((CAST(L.DATE_LIVRAISON_REELLE AS DATE) - CAST(C.DATE_COMMANDE AS DATE)) * 24), "
               "AVG(CASE WHEN L.DATE_LIVRAISON_REELLE >= L.DATE_DEPART_PREVUE THEN "
               "(CAST(L.DATE_LIVRAISON_REELLE AS DATE) - CAST(L.DATE_DEPART_PREVUE AS DATE)) * 1440 END) "
               "FROM LIVRAISON L LEFT JOIN REGROUPE R ON R.ID_LIVRAISON = L.ID_LIVRAISON "
               "LEFT JOIN COMMANDE C ON C.ID_COMMANDE = R.ID_COMMANDE "
               "WHERE L.STATUT = 'Livrée' AND L.DATE_LIVRAISON_REELLE IS NOT NULL").arg(DELAI_CIBLE_HEURES))
        && q.next()) {
        s.livrees = q.value(0).toInt();
        s.livreesALHeure = q.value(1).toInt();
        s.livreesDansCible = q.value(2).toInt();
        if (!q.value(3).isNull()) s.delaiMoyenHeures = q.value(3).toDouble();
        if (!q.value(4).isNull()) s.dureeMoyenneMinutes = q.value(4).toDouble();
    } else {
        qDebug() << "Livraison::statistiques" << q.lastError().text();
    }
    return s;
}

int Livraison::nombreEnRetard()
{
    QSqlQuery q;
    if (q.exec("SELECT COUNT(*) FROM LIVRAISON WHERE STATUT IN ('Planifiée', 'En cours', 'Retardée') "
               "AND DATE_LIVRAISON_PREVUE < SYSTIMESTAMP") && q.next())
        return q.value(0).toInt();
    return 0;
}

// ---------- TOURNÉE D'UN LIVREUR (métiers innovants) ----------
QVector<Etape> Livraison::tournee(int idEmploye, const QDate &jour)
{
    QVector<Etape> etapes;
    QSqlQuery q;
    q.prepare("SELECT L.ID_LIVRAISON, R.ID_COMMANDE, C.CLIENT, L.ADRESSE_LIVRAISON, L.ZONE_GEOGRAPHIQUE, "
              "L.STATUT, L.DATE_DEPART_PREVUE, L.DATE_LIVRAISON_PREVUE, L.DATE_LIVRAISON_REELLE "
              "FROM LIVRAISON L LEFT JOIN REGROUPE R ON R.ID_LIVRAISON = L.ID_LIVRAISON "
              "LEFT JOIN COMMANDE C ON C.ID_COMMANDE = R.ID_COMMANDE "
              "WHERE L.ID_EMPLOYE = :e AND L.DATE_DEPART_PREVUE >= :debut AND L.DATE_DEPART_PREVUE < :fin "
              "ORDER BY L.ORDRE_TOURNEE NULLS LAST, L.DATE_DEPART_PREVUE, L.ID_LIVRAISON");
    q.bindValue(":e", idEmploye);
    q.bindValue(":debut", QDateTime(jour, QTime(0, 0)));
    q.bindValue(":fin", QDateTime(jour.addDays(1), QTime(0, 0)));
    if (!q.exec()) {
        qDebug() << "Livraison::tournee" << q.lastError().text();
        return etapes;
    }
    while (q.next()) {
        Etape e;
        e.idLivraison = q.value(0).toInt();
        e.idCommande = q.value(1).toInt();
        e.client = q.value(2).toString();
        e.adresse = q.value(3).toString();
        e.zone = q.value(4).toString();
        e.statut = q.value(5).toString();
        e.departPrevu = q.value(6).toDateTime();
        e.arriveePrevue = q.value(7).toDateTime();
        e.livreeLe = q.value(8).toDateTime();
        e.point = Geo::localiser(e.adresse, e.zone);
        etapes.append(e);
    }
    return etapes;
}

bool Livraison::enregistrerItineraire(const QVector<Etape> &ordre, double km, QString *erreur)
{
    QStringList noms = {Geo::depot().nom};
    for (const Etape &e : ordre) noms << e.point.nom;
    const QString trajet = noms.join(" → ") + QString(" (%1 km)").arg(km, 0, 'f', 1);

    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();
    QSqlQuery q;
    for (int i = 0; i < ordre.size(); ++i) {
        q.prepare("UPDATE LIVRAISON SET ORDRE_TOURNEE = :o, ITINERAIRE = :it WHERE ID_LIVRAISON = :id");
        q.bindValue(":o", i + 1);
        q.bindValue(":it", QString("Étape %1/%2 — %3").arg(i + 1).arg(ordre.size()).arg(trajet).left(500));
        q.bindValue(":id", ordre[i].idLivraison);
        if (!q.exec()) {
            if (erreur) *erreur = q.lastError().text();
            db.rollback();
            return false;
        }
        HistoriqueLivraison::enregistrer(ordre[i].idLivraison, "Itinéraire optimisé", QString(), QString(),
                                         QString("Étape %1/%2 : %3").arg(i + 1).arg(ordre.size()).arg(trajet));
    }
    if (!db.commit()) {
        if (erreur) *erreur = db.lastError().text();
        db.rollback();
        return false;
    }
    return true;
}

// ---------- Données venant du module Personnel ----------
QList<ChoixListe> Livraison::listeLivreurs()
{
    QList<ChoixListe> l;
    QSqlQuery q;
    q.prepare("SELECT ID_EMPLOYE, PRENOM || ' ' || NOM, TELEPHONE FROM EMPLOYE "
              "WHERE LOWER(TRIM(POSTE)) = LOWER(:poste) AND STATUT = 'Actif' ORDER BY NOM, PRENOM");
    q.bindValue(":poste", POSTE_LIVREUR);
    if (!q.exec()) qDebug() << "listeLivreurs :" << q.lastError().text();
    while (q.next())
        l.append({q.value(0).toInt(), q.value(1).toString(), q.value(2).toString()});
    return l;
}

// ---------- Données venant du module Commandes ----------
// Commandes ni livrées ni annulées, sans livraison active (une livraison échouée libère
// la commande pour une nouvelle tentative), + celle de la livraison en cours d'édition.
QList<ChoixListe> Livraison::listeCommandes(int idCommandeEnCours)
{
    QList<ChoixListe> l;
    QSqlQuery q;
    q.prepare("SELECT C.ID_COMMANDE, C.CLIENT, C.ADRESSE_LIVRAISON FROM COMMANDE C "
              "WHERE (C.STATUT NOT IN ('Livrée', 'Annulée') "
              "       AND NOT EXISTS (SELECT 1 FROM REGROUPE R JOIN LIVRAISON L "
              "                       ON L.ID_LIVRAISON = R.ID_LIVRAISON "
              "                       WHERE R.ID_COMMANDE = C.ID_COMMANDE AND L.STATUT <> 'Échouée')) "
              "   OR C.ID_COMMANDE = :enCours "
              "ORDER BY C.ID_COMMANDE");
    q.bindValue(":enCours", idCommandeEnCours);
    if (!q.exec()) qDebug() << "listeCommandes :" << q.lastError().text();
    while (q.next())
        l.append({q.value(0).toInt(),
                  QString("n° %1 — %2").arg(q.value(0).toString(), q.value(1).toString()),
                  q.value(2).toString()});
    return l;
}
