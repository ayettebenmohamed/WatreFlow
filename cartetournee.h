#pragma once
#include <QWidget>
#include <QVector>
#include "itineraire.h"

// Vue : schéma de la tournée (dépôt + arrêts numérotés reliés dans l'ordre de passage)
class CarteTournee : public QWidget
{
    Q_OBJECT
public:
    explicit CarteTournee(QWidget *parent = nullptr);
    void setTournee(const PointGeo &depot, const QVector<Etape> &etapes);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    PointGeo m_depot;
    QVector<Etape> m_etapes;
};
