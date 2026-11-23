#include "cartetournee.h"
#include "theme.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace
{
QColor couleurArret(const Etape &e)
{
    if (e.statut == "Livrée") return QColor("#2ECC71");
    if (e.statut == "Échouée") return QColor("#E74C3C");
    if (e.retardMinutes > Itineraire::TOLERANCE_RETARD_MIN || e.statut == "Retardée") return QColor("#F39C12");
    if (e.statut == "En cours") return QColor("#00B4D8");
    return Theme::sombre() ? QColor("#3D8BD9") : QColor("#0A4D8C");
}
} // namespace

CarteTournee::CarteTournee(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(320, 260);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void CarteTournee::setTournee(const PointGeo &depot, const QVector<Etape> &etapes)
{
    m_depot = depot;
    m_etapes = etapes;
    update();
}

void CarteTournee::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF cadre = rect().adjusted(1, 1, -1, -1);
    const bool nuit = Theme::sombre();
    const QColor texte = nuit ? QColor("#E3F1F8") : QColor("#14324A");
    const QColor titre = nuit ? QColor("#5CC8EE") : QColor("#0A4D8C");
    p.setPen(QPen(nuit ? QColor("#24405A") : QColor("#D6E9F2"), 1));
    p.setBrush(nuit ? QColor(19, 38, 58, 215) : QColor(244, 250, 253, 215));   // légèrement translucide : le fond animé transparaît
    p.drawRoundedRect(cadre, 12, 12);

    if (m_etapes.isEmpty()) {
        p.setPen(nuit ? QColor("#8FB0C6") : QColor("#6B8193"));
        p.drawText(cadre, Qt::AlignCenter, "Choisissez un livreur et une date\npuis cliquez sur « Calculer la tournée »");
        return;
    }

    // Emprise géographique -> projection équirectangulaire adaptée au widget
    QVector<PointGeo> tous = {m_depot};
    for (const Etape &e : m_etapes) tous << e.point;
    double minLat = 90, maxLat = -90, minLon = 180, maxLon = -180;
    for (const PointGeo &g : tous) {
        minLat = qMin(minLat, g.lat); maxLat = qMax(maxLat, g.lat);
        minLon = qMin(minLon, g.lon); maxLon = qMax(maxLon, g.lon);
    }
    const double kx = qCos(qDegreesToRadians((minLat + maxLat) / 2));
    const double w = qMax((maxLon - minLon) * kx, 0.01), h = qMax(maxLat - minLat, 0.01);
    const QRectF zone = cadre.adjusted(110, 56, -110, -56);
    const double echelle = qMin(zone.width() / w, zone.height() / h);
    const QPointF centre = zone.center();
    auto proj = [&](const PointGeo &g) {
        return QPointF(centre.x() + ((g.lon - (minLon + maxLon) / 2) * kx) * echelle,
                       centre.y() - (g.lat - (minLat + maxLat) / 2) * echelle);
    };

    // Trajet
    QPainterPath chemin(proj(m_depot));
    for (const Etape &e : m_etapes)
        if (e.statut != "Échouée") chemin.lineTo(proj(e.point));
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor("#00B4D8"), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPath(chemin);

    // Dépôt
    const QPointF d = proj(m_depot);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#0A4D8C"));
    p.drawRoundedRect(QRectF(d.x() - 11, d.y() - 11, 22, 22), 5, 5);
    p.setPen(Qt::white);
    QFont f = font();
    f.setBold(true);
    f.setPointSize(8);
    p.setFont(f);
    p.drawText(QRectF(d.x() - 11, d.y() - 11, 22, 22), Qt::AlignCenter, "D");
    p.setPen(titre);
    p.drawText(QRectF(d.x() + 14, d.y() - 8, 140, 16), Qt::AlignLeft | Qt::AlignVCenter, m_depot.nom);

    // Arrêts numérotés (plusieurs arrêts au même endroit : léger décalage)
    QVector<QPointF> places;
    for (int i = 0; i < m_etapes.size(); ++i) {
        const Etape &e = m_etapes[i];
        QPointF c = proj(e.point);
        int doublons = 0;
        for (const QPointF &q : places) if (QLineF(q, proj(e.point)).length() < 4) ++doublons;
        places << proj(e.point);
        c += QPointF(doublons * 16, -doublons * 10);

        p.setPen(QPen(Qt::white, 2));
        p.setBrush(couleurArret(e));
        p.drawEllipse(c, 12, 12);
        p.setPen(Qt::white);
        p.drawText(QRectF(c.x() - 12, c.y() - 12, 24, 24), Qt::AlignCenter, QString::number(i + 1));
        p.setPen(texte);
        p.drawText(QRectF(c.x() - 80, c.y() - 30, 160, 16), Qt::AlignCenter,
                   e.point.nom + (e.client.isEmpty() ? QString() : " · " + e.client));
    }

    // Légende
    const QList<QPair<QString, QColor>> legende = {
        {"Planifiée", QColor("#0A4D8C")}, {"En cours", QColor("#00B4D8")},
        {"Retard", QColor("#F39C12")},    {"Livrée", QColor("#2ECC71")}, {"Échouée", QColor("#E74C3C")}};
    double x = cadre.left() + 12;
    f.setBold(false);
    p.setFont(f);
    for (const auto &l : legende) {
        p.setPen(Qt::NoPen);
        p.setBrush(l.second);
        p.drawEllipse(QPointF(x + 5, cadre.top() + 16), 5, 5);
        p.setPen(nuit ? QColor("#8FB0C6") : QColor("#4B6275"));
        p.drawText(QPointF(x + 14, cadre.top() + 20), l.first);
        x += 22 + p.fontMetrics().horizontalAdvance(l.first);
    }
}
