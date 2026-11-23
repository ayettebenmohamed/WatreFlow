#pragma once
#include <QObject>
#include <QElapsedTimer>
#include <QVector>
#include <QPointF>
#include <QColor>

class QWidget;
class QTimer;
class QPainter;

// Fond animé sur le thème du projet (logistique de livraison d'eau), aux couleurs de la charte :
//   - camions-citernes WaterFlow qui roulent sur une route (roues qui tournent, phares) ;
//   - carte : itinéraires en pointillés qui défilent, colis qui circulent d'un client à l'autre,
//     repères de livraison qui « pulsent » aux couleurs des statuts (planifiée, en cours, livrée, retard) ;
//   - gouttes d'eau qui flottent.
// S'installe comme filtre d'événements derrière un widget existant (aucun changement de mise en page).
//   Clair   : fond de page (carte discrète, sans route)
//   Bandeau : en-tête bleu de la page Livraisons (carte + route + camions)
//   Sombre  : barre latérale / écran de connexion (carte + route + camions)
// Pause automatique quand le widget est caché ; désactivable (préférence mémorisée).
class AnimationLivraison : public QObject
{
    Q_OBJECT
public:
    enum Theme { Clair, Bandeau, Sombre };
    AnimationLivraison(QWidget *cible, Theme theme);

    static bool active();
    static void setActive(bool actif);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    struct Goutte { double x, vitesse, taille, phase, depart; };
    struct Repere { QPointF pos; int statut; double phase; };
    struct Camion { double vitesse, echelle, decalage; int voie; bool versDroite; bool citerne; };

    void dessiner(QPainter &p);
    void dessinerCarte(QPainter &p, double w, double h, double t);
    void dessinerRoute(QPainter &p, double w, double h, double t);
    void dessinerCamion(QPainter &p, double x, double yRoute, double s, bool versDroite, bool citerne,
                        double t) const;
    void tic();

    QWidget *m_cible;
    Theme m_theme;
    QTimer *m_timer;
    QElapsedTimer m_horloge;
    QVector<Goutte> m_gouttes;
    QVector<Repere> m_reperes;
    QVector<Camion> m_camions;
};
