#pragma once
#include <QWidget>
#include <QSqlQueryModel>
#include <QVector>
#include "livraison.h"

namespace Ui { class MainWindow; }
class QChartView;
class QTimer;
class CarteTournee;

// Vue + contrôleur du module « Logistique de Livraison ».
// AUCUNE requête SQL ici : tout passe par le modèle (Livraison, HistoriqueLivraison)
// et la logique métier pure (Itineraire).
class Livraisons : public QWidget
{
    Q_OBJECT
public:
    // Page « Livraisons » du mainwindow.ui unique (widget promu) : les widgets de la page sont
    // créés par Ui::MainWindow ; initialiser() les relie une fois setupUi() terminé.
    explicit Livraisons(QWidget *parent = nullptr);
    ~Livraisons();
    void initialiser(Ui::MainWindow *ui);

    // Recharge livreurs (Personnel), commandes (Commandes), tableau, indicateurs.
    // Appelée par la fenêtre principale à chaque ouverture de la page.
    void rechargerListes();

    // Ré-applique les couleurs dessinées par le code (graphiques, carte) après un changement de thème
    void appliquerTheme();

private slots:
    // CRUD
    void ajouter();
    void modifier();
    void supprimer();
    void nouveau();
    void actualiser();
    void remplirFormulaire(const QModelIndex &index);
    void commandeChoisie(int index);
    void majEstimation();
    // Métiers basiques
    void exporterBordereau();
    void exporterListe();
    void rafraichirStatistiques();
    // Métiers innovants
    void calculerTournee();
    void optimiserTournee();
    void validerTournee();
    void alerterClients();
    void tic();                                   // recalcul automatique (minuterie)
    // Traçabilité
    void actualiserHistorique();

private:
    Livraison livraisonDepuisFormulaire(int id) const;
    int idSelectionne() const;
    QString valeur(int ligne, int colonne) const;
    void afficherModele(QSqlQueryModel *nouveau);
    void viderFormulaire();
    void remplirCombos(int idCommandeEnCours, int idLivreurEnCours);
    void remplirComboTournee();
    bool prerequisOk();
    void rafraichirIndicateurs();
    void afficherTournee();
    void configurerApparence();

    Ui::MainWindow *ui = nullptr;                 // non possédé (appartient à MainWindow)
    QSqlQueryModel *model = nullptr;
    QSqlQueryModel *modelHistorique = nullptr;
    Livraison Ltmp;

    QChartView *chartStatut = nullptr;
    QChartView *chartLivreur = nullptr;
    CarteTournee *carte = nullptr;
    QTimer *minuterie = nullptr;

    QVector<Etape> m_tournee;                     // tournée affichée (ordre courant)
    double m_kmInitial = 0.0;                     // longueur avant optimisation
    bool m_tourneeOptimisee = false;
};
