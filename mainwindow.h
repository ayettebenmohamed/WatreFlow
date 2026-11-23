#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlQueryModel>
#include <QTableView>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class Livraisons;
class Personnel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    static constexpr int CODE_DECONNEXION = 1000;   // code de sortie de la boucle d'événements -> retour au login

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void ajouterCommande();
    void modifierCommande();
    void supprimerCommande();
    void viderFormulaireCommande();
    void exporterCsv();
    void verifierRetards();
    void trierParDate();
    void selectionCommande(const QModelIndex &index);
    void changerPage(int row);
    void deconnecter();
    void basculerTheme();

private:
    Ui::MainWindow *ui;
    QSqlQueryModel *model = nullptr;
    bool triDate = false;

    Livraisons *pageLivraisons = nullptr;
    Personnel *pagePersonnel = nullptr;
    QPushButton *btnTheme = nullptr;

    // Raccourcis vers les widgets de la page Commandes du mainwindow.ui (préfixe « cmd »)
    QTableView *tableViewCommandes;
    QLineEdit *lineEdit_id;
    QLineEdit *lineEdit_client;
    QLineEdit *lineEdit_produit;
    QLineEdit *lineEdit_quantite;
    QDateEdit *dateEdit_commande;
    QLineEdit *lineEdit_adresse;
    QDateEdit *dateEdit_prevue;
    QComboBox *comboBox_statut;
    QLineEdit *lineEdit_transporteur;
    QLineEdit *lineEdit_montant;

    QPushButton *pushButton_ajouter;
    QPushButton *pushButton_modifier;
    QPushButton *pushButton_supprimer;
    QPushButton *pushButton_vider;

    QTableWidget *tableWidget_alertes;
    QLabel *label_caTotal;
    QLabel *label_statsDetails;

    QPushButton *pushButton_majStatutAuto;
    QPushButton *pushButton_exportCSV;
    QPushButton *pushButton_trierDate;

    QLineEdit *lineEdit_recherche;
    QComboBox *comboBox_filtreStatut;

    // Pages du mainwindow.ui : 0 Tableau de bord, 1 Commandes, 2 Livraisons, 3 Personnel
    enum Page { PageTableauBord, PageCommandes, PageLivraisons, PagePersonnel };
    void configurerPageCommandes();
    void configurerTableauDeBord();
    void rafraichirTableauDeBord();
    void chargerDonnees();
    void calculerStatistiques();
    void detecterCommandesInhabituelles();
    bool lireFormulaire(class Commande &c, int id);
    QString valeur(int row, int col) const;
};

#endif // MAINWINDOW_H
