#include "itineraire.h"
#include <QtMath>
#include <algorithm>
#include <limits>
#include <numeric>

namespace
{
struct Lieu { const char *nom; double lat, lon; };

// Coordonnées approximatives (centre de la localité)
const Lieu LIEUX[] = {
    {"Lac 2", 36.8460, 10.2720},       {"Lac 1", 36.8330, 10.2330},
    {"Tunis Centre", 36.8008, 10.1800}, {"Tunis Nord", 36.8450, 10.2000},
    {"Belvédère", 36.8170, 10.1720},    {"Menzah", 36.8400, 10.1700},
    {"Ennasr", 36.8590, 10.1650},       {"Bardo", 36.8092, 10.1406},
    {"Ariana", 36.8625, 10.1956},       {"Soukra", 36.8740, 10.2490},
    {"Raoued", 36.9300, 10.1800},       {"La Marsa", 36.8782, 10.3247},
    {"Marsa", 36.8782, 10.3247},        {"Carthage", 36.8528, 10.3233},
    {"La Goulette", 36.8181, 10.3050},  {"Kram", 36.8333, 10.3167},
    {"Ben Arous", 36.7531, 10.2189},    {"Mégrine", 36.7686, 10.2339},
    {"Ezzahra", 36.7439, 10.3083},      {"Hammam Lif", 36.7300, 10.3400},
    {"Mornag", 36.6800, 10.2900},       {"Manouba", 36.8101, 10.0956},
    {"Nabeul", 36.4561, 10.7376},       {"Bizerte", 37.2744, 9.8739},
    {"Sousse", 35.8256, 10.6084},       {"Monastir", 35.7643, 10.8113},
    {"Sfax", 34.7406, 10.7603},         {"Tunis", 36.8008, 10.1800},
};

QString normaliser(const QString &s)
{
    QString d = s.normalized(QString::NormalizationForm_D).toLower();
    QString r;
    r.reserve(d.size());
    for (const QChar c : d)
        if (c.category() != QChar::Mark_NonSpacing)
            r += c;
    return r;
}

const PointGeo *chercher(const QString &texte)
{
    static QVector<PointGeo> lieux;
    if (lieux.isEmpty()) {
        for (const Lieu &l : LIEUX)
            lieux.append({QString::fromUtf8(l.nom), l.lat, l.lon});
        // les noms les plus longs d'abord : « Lac 2 » avant « Tunis », « La Marsa » avant « Marsa »
        std::stable_sort(lieux.begin(), lieux.end(),
                         [](const PointGeo &a, const PointGeo &b) { return a.nom.size() > b.nom.size(); });
    }
    const QString t = normaliser(texte);
    if (t.isEmpty()) return nullptr;
    for (const PointGeo &p : lieux)
        if (t.contains(normaliser(p.nom)))
            return &p;
    return nullptr;
}
} // namespace

// ------------------------------------------------------------------ Geo
PointGeo Geo::depot()
{
    return {QStringLiteral("Dépôt WaterFlow"), 36.7435, 10.2320};   // zone industrielle Ben Arous
}

QStringList Geo::zones()
{
    return {"Tunis Centre", "Tunis Nord", "Ariana", "La Marsa", "Ben Arous", "Manouba",
            "Nabeul", "Bizerte", "Sousse", "Sfax"};
}

PointGeo Geo::localiser(const QString &adresse, const QString &zone)
{
    if (const PointGeo *p = chercher(adresse)) return *p;
    if (const PointGeo *p = chercher(zone)) return *p;
    return depot();
}

double Geo::distanceKm(const PointGeo &a, const PointGeo &b)
{
    // Haversine (vol d'oiseau) x 1.3 : coefficient moyen de détour routier
    constexpr double R = 6371.0;
    const double dLat = qDegreesToRadians(b.lat - a.lat);
    const double dLon = qDegreesToRadians(b.lon - a.lon);
    const double h = qSin(dLat / 2) * qSin(dLat / 2)
                     + qCos(qDegreesToRadians(a.lat)) * qCos(qDegreesToRadians(b.lat))
                       * qSin(dLon / 2) * qSin(dLon / 2);
    return 2 * R * qAsin(qMin(1.0, qSqrt(h))) * 1.3;
}

// ------------------------------------------------------------------ Itineraire
double Itineraire::longueur(const PointGeo &depart, const QVector<PointGeo> &pts, const QVector<int> &ordre)
{
    double total = 0.0;
    PointGeo courant = depart;
    for (int i : ordre) {
        total += Geo::distanceKm(courant, pts[i]);
        courant = pts[i];
    }
    return total;
}

QVector<int> Itineraire::optimiser(const PointGeo &depart, const QVector<PointGeo> &pts,
                                  const QVector<QDateTime> &echeances, const QDateTime &heureDepart)
{
    const int n = pts.size();
    QVector<int> ordre(n);
    std::iota(ordre.begin(), ordre.end(), 0);
    if (n <= 1) return ordre;

    const bool avecEcheances = echeances.size() == n && heureDepart.isValid();
    // Coût d'un ordre de passage : distance, + pénalité de retard si les échéances sont connues
    auto cout = [&](const QVector<int> &o) {
        double km = 0.0, retardMin = 0.0, minutes = 0.0;
        PointGeo courant = depart;
        for (int i : o) {
            const double d = Geo::distanceKm(courant, pts[i]);
            km += d;
            minutes += d / VITESSE_MOYENNE_KMH * 60.0;
            if (avecEcheances && echeances[i].isValid()) {
                const double ecart = heureDepart.secsTo(echeances[i]) / 60.0;
                if (minutes > ecart) retardMin += minutes - ecart;
            }
            minutes += DUREE_ARRET_MIN;
            courant = pts[i];
        }
        return km + KM_PAR_MINUTE_DE_RETARD * retardMin;
    };

    // Petite tournée : on essaie toutes les permutations -> solution exacte
    if (n <= 8) {
        QVector<int> essai = ordre, meilleur = ordre;
        double best = std::numeric_limits<double>::max();
        do {
            const double c = cout(essai);
            if (c < best - 1e-9) { best = c; meilleur = essai; }
        } while (std::next_permutation(essai.begin(), essai.end()));
        return meilleur;
    }

    // Grande tournée : plus proche voisin...
    QVector<bool> vu(n, false);
    PointGeo courant = depart;
    for (int k = 0; k < n; ++k) {
        int choix = -1;
        double dmin = std::numeric_limits<double>::max();
        for (int i = 0; i < n; ++i) {
            if (vu[i]) continue;
            const double d = Geo::distanceKm(courant, pts[i]);
            if (d < dmin) { dmin = d; choix = i; }
        }
        vu[choix] = true;
        ordre[k] = choix;
        courant = pts[choix];
    }
    // ... puis amélioration 2-opt (inversion de segments tant que le coût baisse)
    double coutActuel = cout(ordre);
    bool ameliore = true;
    while (ameliore) {
        ameliore = false;
        for (int i = 0; i < n - 1; ++i)
            for (int j = i + 1; j < n; ++j) {
                QVector<int> candidat = ordre;
                std::reverse(candidat.begin() + i, candidat.begin() + j + 1);
                const double c = cout(candidat);
                if (c < coutActuel - 1e-9) {
                    ordre = candidat;
                    coutActuel = c;
                    ameliore = true;
                }
            }
    }
    return ordre;
}

void Itineraire::calculerEta(QVector<Etape> &etapes, const PointGeo &depart, const QDateTime &maintenant)
{
    if (etapes.isEmpty()) return;

    // Début de tournée = premier départ prévu
    QDateTime curseur = etapes.first().departPrevu;
    for (const Etape &e : etapes)
        if (e.departPrevu.isValid() && (!curseur.isValid() || e.departPrevu < curseur))
            curseur = e.departPrevu;
    if (!curseur.isValid()) curseur = maintenant;

    PointGeo position = depart;
    for (Etape &e : etapes) {
        e.kmDepuisPrecedent = Geo::distanceKm(position, e.point);

        if (e.statut == "Livrée" && e.livreeLe.isValid()) {
            // Avancement réel : la tournée repart de ce client, à l'heure réelle
            e.eta = e.livreeLe;
            curseur = e.livreeLe.addSecs(DUREE_ARRET_MIN * 60);
        } else if (e.statut == "Échouée") {
            e.eta = QDateTime();                      // arrêt abandonné : pas d'ETA
            continue;                                 // le camion ne s'y est pas arrêté
        } else {
            if (curseur < maintenant) curseur = maintenant;   // on ne remonte pas le temps
            const qint64 trajetSec = qRound64(e.kmDepuisPrecedent / VITESSE_MOYENNE_KMH * 3600.0);
            e.eta = curseur.addSecs(trajetSec);
            curseur = e.eta.addSecs(DUREE_ARRET_MIN * 60);
        }
        e.retardMinutes = (e.eta.isValid() && e.arriveePrevue.isValid())
                              ? int(e.arriveePrevue.secsTo(e.eta) / 60) : 0;
        position = e.point;
    }
}
