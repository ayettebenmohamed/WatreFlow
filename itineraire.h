#pragma once
#include <QString>
#include <QStringList>
#include <QVector>
#include <QDateTime>

// ---------------------------------------------------------------------------
// Métiers innovants du module Livraisons (logique pure, sans SQL ni interface) :
//   1. Optimisation des itinéraires : ordre de passage le plus court entre
//      plusieurs points de livraison (exact jusqu'à 8 arrêts, sinon
//      plus proche voisin + amélioration 2-opt).
//   2. Estimation dynamique de l'heure d'arrivée (ETA) : recalcul selon
//      l'avancement réel de la tournée (arrêts déjà livrés, heure actuelle).
// ---------------------------------------------------------------------------

struct PointGeo
{
    QString nom;
    double lat = 0.0;
    double lon = 0.0;
};

// Référentiel géographique (Grand Tunis + principales villes desservies)
namespace Geo
{
    PointGeo depot();                                             // Dépôt WaterFlow
    QStringList zones();                                          // zones proposées dans le formulaire
    PointGeo localiser(const QString &adresse, const QString &zone);
    double distanceKm(const PointGeo &a, const PointGeo &b);      // distance routière estimée
}

// Un arrêt de la tournée d'un livreur
struct Etape
{
    int idLivraison = 0;
    int idCommande = 0;
    QString client;
    QString adresse;
    QString zone;
    QString statut;
    PointGeo point;
    QDateTime departPrevu;
    QDateTime arriveePrevue;
    QDateTime livreeLe;          // date réelle (si livrée)

    // Résultats du calcul
    double kmDepuisPrecedent = 0.0;
    QDateTime eta;               // heure d'arrivée estimée (ou réelle si livrée)
    int retardMinutes = 0;       // > 0 : en retard sur l'heure prévue
    bool terminee() const { return statut == "Livrée" || statut == "Échouée"; }
};

class Itineraire
{
public:
    static constexpr double VITESSE_MOYENNE_KMH = 35.0;  // circulation urbaine
    static constexpr int DUREE_ARRET_MIN = 10;           // déchargement chez le client
    static constexpr int TOLERANCE_RETARD_MIN = 15;      // au-delà : alerte

    static constexpr double KM_PAR_MINUTE_DE_RETARD = 0.5;   // pondération distance / ponctualité

    // Ordre de visite optimal des points (indices dans pts), départ du dépôt, sans retour.
    // Sans échéances : trajet le plus court. Avec échéances (heure promise au client) et heure
    // de départ : coût = km + 0.5 km par minute de retard -> trajet court ET ponctuel.
    static QVector<int> optimiser(const PointGeo &depart, const QVector<PointGeo> &pts,
                                  const QVector<QDateTime> &echeances = {},
                                  const QDateTime &heureDepart = QDateTime());

    // Longueur d'un parcours (km)
    static double longueur(const PointGeo &depart, const QVector<PointGeo> &pts, const QVector<int> &ordre);

    // Recalcule eta / retard / km de chaque étape (dans l'ordre donné).
    static void calculerEta(QVector<Etape> &etapes, const PointGeo &depart, const QDateTime &maintenant);
};
