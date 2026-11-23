#include "animationlivraison.h"
#include "theme.h"
#include <QWidget>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QRandomGenerator>
#include <QSettings>
#include <QPointer>
#include <QList>
#include <QtMath>
#include <cmath>

namespace
{
// Charte graphique du cahier de spécification
const QColor BLEU("#0A4D8C"), BLEU_NUIT("#073763"), CYAN("#00B4D8"), BLEU_CLAIR("#E6F4FA"),
             VERT("#2ECC71"), ORANGE("#F39C12"), ROUGE("#E74C3C");

QColor alpha(QColor c, int a) { c.setAlpha(a); return c; }

QList<QPointer<AnimationLivraison>> &toutes()
{
    static QList<QPointer<AnimationLivraison>> liste;
    return liste;
}
bool g_lu = false, g_actif = true;

double hasard() { return QRandomGenerator::global()->bounded(10000) / 10000.0; }
} // namespace

// ------------------------------------------------------------------ préférence
bool AnimationLivraison::active()
{
    if (!g_lu) {
        g_actif = QSettings("WaterFlow", "Interface").value("animation", true).toBool();
        g_lu = true;
    }
    return g_actif;
}

void AnimationLivraison::setActive(bool actif)
{
    g_actif = actif;
    g_lu = true;
    QSettings("WaterFlow", "Interface").setValue("animation", actif);
    for (const auto &a : toutes())
        if (a) a->m_cible->update();
}

// ------------------------------------------------------------------ construction
AnimationLivraison::AnimationLivraison(QWidget *cible, Theme theme)
    : QObject(cible), m_cible(cible), m_theme(theme), m_timer(new QTimer(this))
{
    toutes().append(this);

    // Gouttes d'eau
    const int nGouttes = theme == Clair ? 16 : theme == Bandeau ? 10 : 12;
    for (int i = 0; i < nGouttes; ++i)
        m_gouttes.append({hasard(), 0.02 + hasard() * 0.04, 3.0 + hasard() * 5.0, hasard() * 6.283, hasard()});

    // Repères de livraison (positions relatives) : la tournée part du dépôt (à gauche)
    // positions relatives à la zone de carte (voir zoneCarte)
    const QList<QPointF> pos = theme == Clair
        ? QList<QPointF>{{0.10, 0.18}, {0.30, 0.10}, {0.48, 0.30}, {0.70, 0.14}, {0.90, 0.30},
                         {0.82, 0.62}, {0.55, 0.72}, {0.25, 0.60}}
        : QList<QPointF>{{0.18, 0.25}, {0.36, 0.80}, {0.54, 0.20}, {0.72, 0.78}, {0.90, 0.30}};
    for (int i = 0; i < pos.size(); ++i)
        m_reperes.append({pos[i], i % 4, hasard()});

    // Camions (route : Bandeau et Sombre seulement)
    if (theme != Clair) {
        m_camions = {{70, 1.00, 0.00, 1, true, true},
                     {55, 0.90, 0.55, 1, true, false},
                     {45, 0.80, 0.25, 0, false, true}};
    }

    m_cible->installEventFilter(this);
    m_horloge.start();
    connect(m_timer, &QTimer::timeout, this, &AnimationLivraison::tic);
    m_timer->start(40);                                        // 25 images par seconde
}

void AnimationLivraison::tic()
{
    if (!active() || !m_cible->isVisible() || m_cible->window()->isMinimized()) return;
    m_cible->update();
}

bool AnimationLivraison::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == m_cible && ev->type() == QEvent::Paint) {
        QPainter p(m_cible);
        p.setClipRegion(static_cast<QPaintEvent *>(ev)->region());
        dessiner(p);
        return true;     // on remplace le fond : les widgets enfants se dessinent par-dessus
    }
    return QObject::eventFilter(obj, ev);
}

// ------------------------------------------------------------------ dessin
void AnimationLivraison::dessiner(QPainter &p)
{
    const QRectF r = m_cible->rect();
    const double w = r.width(), h = r.height();
    const double t = active() ? m_horloge.elapsed() / 1000.0 : 0.0;
    p.setRenderHint(QPainter::Antialiasing);
    if (m_theme == Bandeau) {
        QPainterPath arrondi;
        arrondi.addRoundedRect(r, 14, 14);
        p.setClipPath(arrondi, Qt::IntersectClip);
    }

    // 1. Dégradé de fond aux couleurs de la charte
    QLinearGradient fond(0, 0, w, h);
    if (m_theme == Clair && ::Theme::sombre()) {          // page en mode sombre
        fond.setColorAt(0, QColor("#0B1724"));
        fond.setColorAt(1, QColor("#0F2C45"));
    } else if (m_theme == Clair) {
        fond.setColorAt(0, QColor("#F7FCFE"));
        fond.setColorAt(1, BLEU_CLAIR);
    } else if (m_theme == Bandeau) {
        fond.setColorAt(0, BLEU);
        fond.setColorAt(0.65, QColor("#0B6FA8"));
        fond.setColorAt(1, CYAN);
    } else {
        fond = QLinearGradient(0, 0, 0, h);
        fond.setColorAt(0, QColor("#0B5596"));
        fond.setColorAt(1, BLEU_NUIT);
    }
    p.fillRect(r, fond);

    // 2. Halo lumineux qui dérive (profondeur)
    {
        const QPointF c(w * (0.65 + 0.18 * qSin(t * 0.25)), h * (0.35 + 0.15 * qCos(t * 0.31)));
        QRadialGradient g(c, qMax(w, h) * 0.55);
        g.setColorAt(0, m_theme == Clair ? alpha(CYAN, 40) : alpha(QColor(Qt::white), 30));
        g.setColorAt(1, alpha(CYAN, 0));
        p.fillRect(r, g);
    }

    // 3. Carte : grille, itinéraires, colis, repères
    dessinerCarte(p, w, h, t);

    // 4. Gouttes d'eau qui flottent vers le haut
    p.setPen(Qt::NoPen);
    for (const Goutte &g : m_gouttes) {
        const double y = (1.0 - std::fmod(g.depart + t * g.vitesse, 1.15)) * h;
        const double x = g.x * w + 14 * qSin(t * 0.8 + g.phase);
        const double s = g.taille;
        QPainterPath goutte;
        goutte.moveTo(x, y - s * 1.9);
        goutte.cubicTo(x + s * 0.4, y - s * 1.1, x + s, y - s * 0.4, x + s, y + s * 0.1);
        goutte.cubicTo(x + s, y + s * 0.75, x + s * 0.55, y + s * 1.1, x, y + s * 1.1);
        goutte.cubicTo(x - s * 0.55, y + s * 1.1, x - s, y + s * 0.75, x - s, y + s * 0.1);
        goutte.cubicTo(x - s, y - s * 0.4, x - s * 0.4, y - s * 1.1, x, y - s * 1.9);
        QLinearGradient lg(x, y - s * 2, x, y + s);
        lg.setColorAt(0, m_theme == Clair ? alpha(CYAN, 90) : alpha(QColor(Qt::white), 120));
        lg.setColorAt(1, m_theme == Clair ? alpha(BLEU, 50) : alpha(CYAN, 70));
        p.setBrush(lg);
        p.drawPath(goutte);
    }

    // 5. Route et camions-citernes
    if (m_theme != Clair) dessinerRoute(p, w, h, t);
}

void AnimationLivraison::dessinerCarte(QPainter &p, double w, double h, double t)
{
    const bool pageClaire = m_theme == Clair;             // fond de page (pas de route)
    const bool clair = pageClaire && !::Theme::sombre();    // couleurs « jour »

    // Grille de carte discrète
    if (pageClaire) {
        p.setPen(QPen(clair ? alpha(BLEU, 12) : alpha(CYAN, 14), 1));
        for (double x = 0; x < w; x += 56) p.drawLine(QPointF(x, 0), QPointF(x, h));
        for (double y = 0; y < h; y += 56) p.drawLine(QPointF(0, y), QPointF(w, y));
    }

    // Zone de la carte : on évite les textes (titre, horloge, menu, slogan)
    QRectF zone(0, 0, w, h);
    if (m_theme == Bandeau)    zone = QRectF(w * 0.42, 14, w * 0.38, h - 52);
    else if (m_theme == Sombre) zone = h > 700 ? QRectF(w * 0.06, h * 0.33, w * 0.88, h * 0.40)    // barre latérale
                                               : QRectF(w * 0.06, h * 0.75, w * 0.88, h * 0.13);   // connexion

    // Itinéraire : dépôt -> repères, courbes lissées
    QVector<QPointF> pts;
    pts << (pageClaire ? QPointF(w * 0.04, h * 0.40) : QPointF(zone.left(), zone.center().y()));
    for (const Repere &rp : m_reperes)
        pts << QPointF(zone.left() + rp.pos.x() * zone.width(), zone.top() + rp.pos.y() * zone.height());

    QPainterPath chemin(pts.first());
    for (int i = 1; i < pts.size(); ++i) {
        const QPointF a = pts[i - 1], b = pts[i];
        const double dx = (b.x() - a.x()) * 0.5;
        chemin.cubicTo(QPointF(a.x() + dx, a.y()), QPointF(b.x() - dx, b.y()), b);
    }
    // trait de fond + pointillés qui défilent (flux de livraison)
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(clair ? alpha(CYAN, 22) : alpha(QColor(Qt::white), 35), clair ? 6 : 5,
                  Qt::SolidLine, Qt::RoundCap));
    p.drawPath(chemin);
    QPen tirets(clair ? alpha(BLEU, 45) : alpha(QColor(Qt::white), 150), 2, Qt::CustomDashLine, Qt::RoundCap);
    tirets.setDashPattern({4, 5});
    tirets.setDashOffset(-t * 12);
    p.setPen(tirets);
    p.drawPath(chemin);

    // Colis qui circulent sur l'itinéraire
    for (int k = 0; k < 3; ++k) {
        const double f = std::fmod(t * 0.035 + k / 3.0, 1.0);
        const QPointF c = chemin.pointAtPercent(f);
        const double ang = -chemin.angleAtPercent(f);
        p.save();
        p.translate(c);
        p.rotate(ang);
        p.setPen(Qt::NoPen);
        p.setBrush(alpha(QColor(Qt::black), 30));
        p.drawRoundedRect(QRectF(-5, -3, 12, 10), 2, 2);
        p.setBrush(clair ? alpha(CYAN, 150) : QColor(Qt::white));
        p.drawRoundedRect(QRectF(-6, -5, 12, 10), 2, 2);
        p.setPen(QPen(clair ? QColor(Qt::white) : CYAN, 1.4));
        p.drawLine(QPointF(0, -5), QPointF(0, 5));
        p.restore();
    }

    // Dépôt (usine WaterFlow)
    {
        const QPointF d = pts.first();
        p.setPen(Qt::NoPen);
        p.setBrush(clair ? alpha(BLEU, 160) : QColor(Qt::white));
        p.drawRoundedRect(QRectF(d.x() - 9, d.y() - 9, 18, 18), 4, 4);
        p.setBrush(clair ? QColor(Qt::white) : BLEU);
        QPainterPath g;                                   // petite goutte dans le dépôt
        g.moveTo(d.x(), d.y() - 6);
        g.cubicTo(d.x() + 4, d.y() - 1, d.x() + 4, d.y() + 5, d.x(), d.y() + 5);
        g.cubicTo(d.x() - 4, d.y() + 5, d.x() - 4, d.y() - 1, d.x(), d.y() - 6);
        p.drawPath(g);
    }

    // Repères de livraison qui pulsent (couleurs des statuts)
    const QList<QColor> statuts = {clair ? BLEU : QColor(Qt::white), CYAN, VERT, ORANGE};
    for (int i = 0; i < m_reperes.size(); ++i) {
        const QPointF c = pts[i + 1];
        const QColor col = statuts[m_reperes[i].statut];
        const double cycle = std::fmod(t / 2.4 + m_reperes[i].phase, 1.0);
        // onde
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(alpha(col, int((clair ? 90 : 150) * (1 - cycle))), 2));
        p.drawEllipse(c, 5 + 20 * cycle, (5 + 20 * cycle) * 0.55);
        // marqueur (épingle)
        const double s = clair ? 9 : 8;
        QPainterPath epingle;
        epingle.moveTo(c);
        epingle.cubicTo(c + QPointF(-s * 0.3, -s * 0.9), c + QPointF(-s, -s * 1.3), c + QPointF(-s, -s * 1.9));
        epingle.arcTo(QRectF(c.x() - s, c.y() - s * 2.9, s * 2, s * 2), 180, -180);
        epingle.cubicTo(c + QPointF(s, -s * 1.3), c + QPointF(s * 0.3, -s * 0.9), c);
        p.setPen(Qt::NoPen);
        p.setBrush(alpha(QColor(Qt::black), 35));
        p.drawEllipse(c, s * 0.6, s * 0.25);
        p.setBrush(clair ? alpha(col, 120) : col);
        p.drawPath(epingle);
        p.setBrush(m_theme == Clair ? QColor(Qt::white) : (col == QColor(Qt::white) ? BLEU : QColor(Qt::white)));
        p.drawEllipse(c + QPointF(0, -s * 1.9), s * 0.38, s * 0.38);
    }
}

void AnimationLivraison::dessinerRoute(QPainter &p, double w, double h, double t)
{
    // Position de la route : bas du bandeau / de l'écran de connexion ; plus haut dans la barre latérale
    const double yRoute = m_theme == Bandeau ? h - 13 : (h > 700 ? h - 290 : h - 16);
    const double echelle = m_theme == Bandeau ? 0.42 : (h > 700 ? 0.33 : 0.48);

    // Chaussée + ligne centrale
    p.setPen(Qt::NoPen);
    p.setBrush(alpha(BLEU_NUIT, m_theme == Bandeau ? 110 : 140));
    p.drawRect(QRectF(0, yRoute - 13, w, 26));
    p.setBrush(alpha(CYAN, 120));
    p.drawRect(QRectF(0, yRoute - 13, w, 1.5));
    p.drawRect(QRectF(0, yRoute + 11.5, w, 1.5));
    QPen ligne(alpha(QColor(Qt::white), 150), 1.5, Qt::CustomDashLine);
    ligne.setDashPattern({8, 8});
    p.setPen(ligne);
    p.drawLine(QPointF(0, yRoute), QPointF(w, yRoute));

    // Camions : voie du fond (vers la gauche) puis voie de devant (vers la droite)
    for (int voie = 0; voie < 2; ++voie)
        for (const Camion &c : m_camions) {
            if (c.voie != voie) continue;
            const double s = echelle * c.echelle;
            const double longueur = 115 * s;
            const double parcours = w + longueur * 2 + 60;
            const double d = std::fmod(c.decalage * parcours + t * c.vitesse, parcours);
            const double y = voie == 0 ? yRoute - 2 : yRoute + 11;
            const double x = c.versDroite ? d - longueur - 30 : w + longueur + 30 - d;
            dessinerCamion(p, x, y, s, c.versDroite, c.citerne, t * c.vitesse);
        }
}

void AnimationLivraison::dessinerCamion(QPainter &p, double x, double yRoute, double s, bool versDroite,
                                        bool citerne, double distance) const
{
    p.save();
    p.translate(x, yRoute);
    p.scale(versDroite ? s : -s, s);
    p.setPen(Qt::NoPen);

    // Ombre
    p.setBrush(alpha(QColor(Qt::black), 45));
    p.drawEllipse(QPointF(56, 1), 60, 4);

    // Faisceau des phares
    QLinearGradient faisceau(112, -20, 190, -20);
    faisceau.setColorAt(0, QColor(255, 240, 180, 90));
    faisceau.setColorAt(1, QColor(255, 240, 180, 0));
    p.setBrush(faisceau);
    p.drawPolygon(QPolygonF({QPointF(112, -22), QPointF(190, -34), QPointF(190, -6), QPointF(112, -17)}));

    // Châssis
    p.setBrush(BLEU_NUIT);
    p.drawRoundedRect(QRectF(0, -16, 114, 8), 2, 2);

    // Citerne (ou caisse) aux couleurs WaterFlow
    if (citerne) {
        QLinearGradient g(0, -48, 0, -16);
        g.setColorAt(0, QColor("#4FD6EE"));
        g.setColorAt(1, CYAN.darker(115));
        p.setBrush(g);
        p.drawRoundedRect(QRectF(2, -48, 76, 32), 15, 15);
        p.setBrush(alpha(QColor(Qt::white), 70));
        p.drawRoundedRect(QRectF(8, -44, 64, 5), 2.5, 2.5);
    } else {
        p.setBrush(QColor("#F4FAFD"));
        p.drawRoundedRect(QRectF(2, -52, 76, 36), 4, 4);
        p.setBrush(CYAN);
        p.drawRect(QRectF(2, -24, 76, 4));
    }
    // Logo goutte
    p.setBrush(citerne ? QColor(Qt::white) : BLEU);
    QPainterPath goutte;
    goutte.moveTo(24, -42);
    goutte.cubicTo(30, -34, 31, -28, 24, -25);
    goutte.cubicTo(17, -28, 18, -34, 24, -42);
    p.drawPath(goutte);
    if (s >= 0.3) {
        QFont f("Segoe UI", 9, QFont::Bold);
        p.setFont(f);
        p.setPen(citerne ? QColor(Qt::white) : BLEU);
        if (!versDroite) {                                   // texte lisible même camion retourné
            p.save();
            p.translate(56, -33);
            p.scale(-1, 1);
            p.drawText(QRectF(-24, -8, 48, 16), Qt::AlignCenter, "WaterFlow");
            p.restore();
        } else {
            p.drawText(QRectF(32, -41, 48, 16), Qt::AlignCenter, "WaterFlow");
        }
        p.setPen(Qt::NoPen);
    }

    // Cabine
    QPainterPath cabine;
    cabine.moveTo(80, -16);
    cabine.lineTo(80, -42);
    cabine.quadTo(80, -46, 85, -46);
    cabine.lineTo(100, -46);
    cabine.quadTo(104, -46, 106, -42);
    cabine.lineTo(113, -28);
    cabine.lineTo(114, -16);
    cabine.closeSubpath();
    p.setBrush(m_theme == Clair ? BLEU : QColor(Qt::white));
    p.drawPath(cabine);
    p.setBrush(m_theme == Clair ? QColor("#BFEFFA") : BLEU.lighter(140));
    p.drawPolygon(QPolygonF({QPointF(90, -42), QPointF(101, -42), QPointF(108, -30), QPointF(90, -30)}));
    p.setBrush(QColor("#FFE08A"));
    p.drawEllipse(QPointF(111, -21), 2.6, 2.6);
    p.setBrush(ROUGE);
    p.drawRect(QRectF(0, -18, 3, 4));                         // feu arrière

    // Roues qui tournent
    const double angle = distance / (8 * s) * 57.3;
    for (double rx : {22.0, 62.0, 98.0}) {
        p.setBrush(QColor("#14324A"));
        p.drawEllipse(QPointF(rx, -7), 8.5, 8.5);
        p.setBrush(QColor("#D6E9F2"));
        p.drawEllipse(QPointF(rx, -7), 4, 4);
        p.save();
        p.translate(rx, -7);
        p.rotate(angle);
        p.setPen(QPen(QColor("#14324A"), 1.4));
        p.drawLine(QPointF(-4, 0), QPointF(4, 0));
        p.drawLine(QPointF(0, -4), QPointF(0, 4));
        p.restore();
        p.setPen(Qt::NoPen);
    }
    p.restore();
}
