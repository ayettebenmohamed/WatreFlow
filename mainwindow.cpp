#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "connection.h"
#include "commande.h"
#include "livraisons.h"
#include "personnel.h"
#include "session.h"
#include "animationlivraison.h"
#include "theme.h"
#include "employe.h"
#include "livraison.h"
#include <QTimer>
#include <QLocale>
#include <memory>
#include <QCheckBox>
#include <QMessageBox>
#include <QStatusBar>
#include <QStyle>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QDate>
#include <QDebug>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QListWidget>
#include <QGroupBox>
#include <QFrame>
#include <QIntValidator>
#include <QDoubleValidator>

// Colonnes du modèle renvoyé par Commande::afficher()
enum ColCmd { C_ID, C_CLIENT, C_PRODUITS, C_QTE, C_DATE, C_ADRESSE, C_PREVUE, C_STATUT, C_MONTANT, C_TRANSP };

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);              // UN SEUL fichier .ui : barre latérale + 4 pages (dashboard)
    resize(1400, 900);

    Connection::createInstance().createConnection();   // déjà ouverte dans main : ne fait rien

    // Page Commandes : widgets décrits dans mainwindow.ui (préfixe « cmd »)
    configurerPageCommandes();

    // Pages Personnel et Livraisons : widgets promus du mainwindow.ui (préfixes « pers » / « liv »).
    // Leurs contrôleurs sont reliés aux widgets une fois setupUi() terminé.
    pagePersonnel = ui->pagePersonnel;
    pagePersonnel->initialiser(ui);
    pageLivraisons = ui->pageLivraisons;     // dépend de Personnel (livreurs) et Commandes
    pageLivraisons->initialiser(ui);

    // Page d'accueil : tableau de bord des trois modules
    configurerTableauDeBord();

    // Barre latérale : icônes + utilisateur connecté
    const QList<QStyle::StandardPixmap> icones = {QStyle::SP_ComputerIcon, QStyle::SP_FileDialogContentsView,
                                                  QStyle::SP_ArrowRight, QStyle::SP_DirHomeIcon};
    for (int i = 0; i < ui->listWidget->count() && i < icones.size(); ++i) {
        ui->listWidget->item(i)->setIcon(style()->standardIcon(icones[i]));
        ui->listWidget->item(i)->setSizeHint(QSize(0, 46));
    }
    ui->appUtilisateur->setText("Connecté : " + Session::utilisateur());

    // Fond animé de la barre latérale + interrupteur (préférence mémorisée)
    new AnimationLivraison(ui->appSidebar, AnimationLivraison::Sombre);
    auto *anim = new QCheckBox("Fond animé");
    anim->setObjectName("toggleAnimation");
    anim->setChecked(AnimationLivraison::active());
    anim->setCursor(Qt::PointingHandCursor);
    connect(anim, &QCheckBox::toggled, this, [](bool on) { AnimationLivraison::setActive(on); });
    ui->layoutSidebar->insertWidget(ui->layoutSidebar->indexOf(ui->appUtilisateur), anim);

    // Bouton mode jour / mode sombre (préférence mémorisée)
    btnTheme = new QPushButton;
    btnTheme->setObjectName("btnTheme");
    btnTheme->setCursor(Qt::PointingHandCursor);
    btnTheme->setToolTip("Basculer entre le mode jour et le mode sombre");
    btnTheme->setText(Theme::sombre() ? "☀  Mode jour" : "🌙  Mode sombre");
    connect(btnTheme, &QPushButton::clicked, this, &MainWindow::basculerTheme);
    ui->layoutSidebar->insertWidget(ui->layoutSidebar->indexOf(anim), btnTheme);

    // Déconnexion : retour à l'écran de connexion (raccourci Ctrl+Maj+Q)
    auto *btnDeconnexion = new QPushButton("⏻  Se déconnecter");
    btnDeconnexion->setObjectName("btnDeconnexion");
    btnDeconnexion->setCursor(Qt::PointingHandCursor);
    btnDeconnexion->setShortcut(QKeySequence("Ctrl+Shift+Q"));
    btnDeconnexion->setToolTip("Fermer la session et revenir à l'écran de connexion (Ctrl+Maj+Q)");
    connect(btnDeconnexion, &QPushButton::clicked, this, &MainWindow::deconnecter);
    ui->layoutSidebar->addWidget(btnDeconnexion);

    statusBar()->showMessage("Base Oracle connectée · schéma "
                             + Connection::createInstance().getDatabase().userName().toUpper());

    connect(ui->listWidget, &QListWidget::currentRowChanged, this, &MainWindow::changerPage);
    ui->listWidget->setCurrentRow(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::basculerTheme()
{
    Theme::appliquer(!Theme::sombre());
    btnTheme->setText(Theme::sombre() ? "☀  Mode jour" : "🌙  Mode sombre");
    pageLivraisons->appliquerTheme();                 // graphiques, carte de tournée, fond animé
}

void MainWindow::deconnecter()
{
    QMessageBox box(QMessageBox::Question, "Déconnexion",
                    QString("Fermer la session de %1 ?").arg(Session::utilisateur()),
                    QMessageBox::NoButton, this);
    QPushButton *oui = box.addButton("Se déconnecter", QMessageBox::AcceptRole);
    box.setDefaultButton(box.addButton("Annuler", QMessageBox::RejectRole));
    box.exec();
    if (box.clickedButton() != oui) return;

    hide();
    QCoreApplication::exit(CODE_DECONNEXION);       // main() réaffiche l'écran de connexion
}

// À chaque changement de page on recharge : une livraison modifie les commandes,
// un employé ajouté dans Personnel doit apparaître dans la liste des livreurs.
void MainWindow::changerPage(int row)
{
    ui->stackedWidget->setCurrentIndex(row);
    switch (row) {
    case PageTableauBord: rafraichirTableauDeBord(); break;
    case PageCommandes:   chargerDonnees(); break;
    case PageLivraisons:  pageLivraisons->rechargerListes(); break;
    default: break;
    }
}

// ---------------------------------------------------------------- Page Commandes (mainwindow.ui)
void MainWindow::configurerPageCommandes()
{
    tableViewCommandes      = ui->cmdTableView;
    lineEdit_id             = ui->cmdLineEditId;
    lineEdit_client         = ui->cmdLineEditClient;
    lineEdit_produit        = ui->cmdLineEditProduit;
    lineEdit_quantite       = ui->cmdLineEditQuantite;
    dateEdit_commande       = ui->cmdDateEditCommande;
    lineEdit_adresse        = ui->cmdLineEditAdresse;
    dateEdit_prevue         = ui->cmdDateEditPrevue;
    comboBox_statut         = ui->cmdComboStatut;
    lineEdit_transporteur   = ui->cmdLineEditTransporteur;
    lineEdit_montant        = ui->cmdLineEditMontant;
    pushButton_ajouter      = ui->cmdBtnAjouter;
    pushButton_modifier     = ui->cmdBtnModifier;
    pushButton_supprimer    = ui->cmdBtnSupprimer;
    pushButton_vider        = ui->cmdBtnVider;
    pushButton_majStatutAuto = ui->cmdBtnVerifierRetards;
    pushButton_exportCSV    = ui->cmdBtnExporterCsv;
    pushButton_trierDate    = ui->cmdBtnTrierDate;
    tableWidget_alertes     = ui->cmdTableAlertes;
    label_caTotal           = ui->cmdLabelCaTotal;
    label_statsDetails      = ui->cmdLabelStats;
    lineEdit_recherche      = ui->cmdLineEditRecherche;
    comboBox_filtreStatut   = ui->cmdComboFiltreStatut;

    // Contrôle de saisie
    lineEdit_quantite->setValidator(new QIntValidator(1, 1000000, this));
    auto *valMontant = new QDoubleValidator(0, 1e9, 3, this);
    valMontant->setNotation(QDoubleValidator::StandardNotation);
    valMontant->setLocale(QLocale::c());
    lineEdit_montant->setValidator(valMontant);
    dateEdit_commande->setDate(QDate::currentDate());
    dateEdit_prevue->setDate(QDate::currentDate().addDays(5));
    tableWidget_alertes->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    connect(pushButton_ajouter, &QPushButton::clicked, this, &MainWindow::ajouterCommande);
    connect(pushButton_modifier, &QPushButton::clicked, this, &MainWindow::modifierCommande);
    connect(pushButton_supprimer, &QPushButton::clicked, this, &MainWindow::supprimerCommande);
    connect(pushButton_vider, &QPushButton::clicked, this, &MainWindow::viderFormulaireCommande);
    connect(pushButton_trierDate, &QPushButton::clicked, this, &MainWindow::trierParDate);
    connect(pushButton_exportCSV, &QPushButton::clicked, this, &MainWindow::exporterCsv);
    connect(pushButton_majStatutAuto, &QPushButton::clicked, this, &MainWindow::verifierRetards);
    connect(lineEdit_recherche, &QLineEdit::textChanged, this, &MainWindow::chargerDonnees);
    connect(comboBox_filtreStatut, &QComboBox::currentIndexChanged, this, &MainWindow::chargerDonnees);
    connect(tableViewCommandes, &QTableView::clicked, this, &MainWindow::selectionCommande);
}

// ---------------------------------------------------------------- Tableau de bord (mainwindow.ui)
void MainWindow::configurerTableauDeBord()
{
    new AnimationLivraison(ui->pageTableauBord, AnimationLivraison::Clair);   // fond : carte discrète
    new AnimationLivraison(ui->dashHero, AnimationLivraison::Bandeau);        // en-tête : route + camions

    // Les cartes ont des propriétés dynamiques (role, accent) lues par la feuille de style :
    // Qt les a stylées à leur création, avant que setupUi() ne pose ces propriétés -> on réapplique.
    for (QWidget *w : ui->pageTableauBord->findChildren<QWidget *>()) {
        w->style()->unpolish(w);
        w->style()->polish(w);
    }

    // Boutons « Ouvrir … » : navigation vers le module
    connect(ui->dashCartePersonnelBouton, &QPushButton::clicked, this, [this] { ui->listWidget->setCurrentRow(PagePersonnel); });
    connect(ui->dashCarteCommandesBouton, &QPushButton::clicked, this, [this] { ui->listWidget->setCurrentRow(PageCommandes); });
    connect(ui->dashCarteLivraisonsBouton, &QPushButton::clicked, this, [this] { ui->listWidget->setCurrentRow(PageLivraisons); });

    // Horloge + actualisation automatique (toutes les 30 s quand le tableau de bord est affiché)
    auto *minuterie = new QTimer(this);
    auto tic = [this] {
        ui->dashLabelHorloge->setText(QLocale(QLocale::French).toString(QDateTime::currentDateTime(),
                                                                         "dddd d MMMM yyyy · HH:mm"));
        if (ui->stackedWidget->currentIndex() == PageTableauBord) rafraichirTableauDeBord();
    };
    connect(minuterie, &QTimer::timeout, this, tic);
    minuterie->start(30 * 1000);
    ui->dashLabelHorloge->setText(QLocale(QLocale::French).toString(QDateTime::currentDateTime(),
                                                                     "dddd d MMMM yyyy · HH:mm"));
}

// Indicateurs des trois modules : uniquement via les classes Modèle (aucune requête SQL ici)
void MainWindow::rafraichirTableauDeBord()
{
    QStringList alertes;                         // texte | couleur

    // --- Personnel
    Employe e;
    std::unique_ptr<QSqlQueryModel> mp(e.afficher());
    int actifs = 0, absents = 0, livreurs = 0;
    for (int r = 0; r < mp->rowCount(); ++r) {
        const QString statut = mp->index(r, 6).data().toString();
        const QString poste = mp->index(r, 3).data().toString();
        if (statut == "Actif") {
            ++actifs;
            if (poste.compare("Livreur", Qt::CaseInsensitive) == 0) ++livreurs;
        } else {
            ++absents;
            alertes << QString("👤  %1 %2 est « %3 »|#5E7A8F")
                           .arg(mp->index(r, 2).data().toString(), mp->index(r, 1).data().toString(), statut);
        }
    }
    ui->dashPersEffectifValeur->setText(QString::number(mp->rowCount()));
    ui->dashPersActifsValeur->setText(QString::number(actifs));
    ui->dashPersAbsentsValeur->setText(QString::number(absents));
    ui->dashPersLivreursValeur->setText(QString::number(livreurs));

    // --- Commandes
    std::unique_ptr<QSqlQueryModel> mc(Commande::afficher());
    int attente = 0, grosses = 0;
    double ca = 0.0;
    for (int r = 0; r < mc->rowCount(); ++r) {
        const QString statut = mc->index(r, C_STATUT).data().toString();
        if (statut == "En attente") ++attente;
        if (statut != "Annulée") ca += mc->index(r, C_MONTANT).data().toDouble();
        if (mc->index(r, C_QTE).data().toInt() > 200) {
            ++grosses;
            alertes << QString("📦  Commande n° %1 (%2) : quantité élevée, %3 unités|#F39C12")
                           .arg(mc->index(r, C_ID).data().toString(), mc->index(r, C_CLIENT).data().toString(),
                                mc->index(r, C_QTE).data().toString());
        }
    }
    if (attente > 0)
        alertes.prepend(QString("🕑  %1 commande(s) en attente de livraison|#0A4D8C").arg(attente));
    ui->dashCmdTotalValeur->setText(QString::number(mc->rowCount()));
    ui->dashCmdAttenteValeur->setText(QString::number(attente));
    ui->dashCmdCaValeur->setText(QLocale(QLocale::French).toString(ca, 'f', 0));
    ui->dashCmdAlertesValeur->setText(QString::number(grosses));

    // --- Livraisons
    const StatsLivraison s = Livraison::statistiques();
    int actives = 0, retardees = 0;
    for (const auto &p : s.parStatut) {
        if (p.first == "Planifiée" || p.first == "En cours") actives += p.second;
        if (p.first == "Retardée") retardees = p.second;
    }
    const int depassees = Livraison::nombreEnRetard();
    ui->dashLivTotalValeur->setText(QString::number(s.total));
    ui->dashLivActivesValeur->setText(QString::number(actives));
    ui->dashLivRetardsValeur->setText(QString::number(qMax(retardees, depassees)));
    ui->dashLivPonctualiteValeur->setText(s.livrees ? QString("%1 %").arg(qRound(100.0 * s.livreesALHeure / s.livrees))
                                                    : QString("—"));
    if (depassees > 0)
        alertes.prepend(QString("🚚  %1 livraison(s) active(s) ont dépassé l'heure prévue|#E74C3C").arg(depassees));

    // --- Liste des alertes
    ui->dashListeAlertes->clear();
    if (alertes.isEmpty())
        alertes << "✅  Aucune alerte : tout est sous contrôle|#2ECC71";
    for (const QString &a : alertes) {
        const QStringList p = a.split('|');
        auto *item = new QListWidgetItem(p.value(0));
        item->setForeground(QColor(Theme::sombre() && p.value(1) == "#0A4D8C" ? "#5CC8EE" : p.value(1)));
        item->setSizeHint(QSize(0, 34));
        ui->dashListeAlertes->addItem(item);
    }
}

QString MainWindow::valeur(int row, int col) const
{
    return model ? model->index(row, col).data().toString() : QString();
}

// READ : recharge le tableau avec la recherche / le filtre en cours
void MainWindow::chargerDonnees()
{
    QSqlQueryModel *nouveau = Commande::afficher(lineEdit_recherche->text(),
                                                 comboBox_filtreStatut->currentText(), triDate);
    nouveau->setParent(this);
    QItemSelectionModel *ancienneSel = tableViewCommandes->selectionModel();
    tableViewCommandes->setModel(nouveau);
    delete ancienneSel;
    delete model;
    model = nouveau;

    auto *h = tableViewCommandes->horizontalHeader();
    h->setSectionResizeMode(QHeaderView::ResizeToContents);
    h->setSectionResizeMode(C_CLIENT, QHeaderView::Stretch);
    h->setSectionResizeMode(C_ADRESSE, QHeaderView::Stretch);

    calculerStatistiques();
    detecterCommandesInhabituelles();
}

bool MainWindow::lireFormulaire(Commande &c, int id)
{
    const QString client = lineEdit_client->text().trimmed();
    const QString produits = lineEdit_produit->text().trimmed();
    bool okQ = false, okM = false;
    const int qte = lineEdit_quantite->text().toInt(&okQ);
    const double montant = lineEdit_montant->text().replace(',', '.').toDouble(&okM);

    if (client.isEmpty() || produits.isEmpty() || lineEdit_adresse->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Erreur de saisie", "Client, produits et adresse de livraison sont obligatoires.");
        return false;
    }
    if (!okQ || qte <= 0) {
        QMessageBox::warning(this, "Erreur de saisie", "La quantité doit être un entier positif.");
        return false;
    }
    if (!okM || montant < 0) {
        QMessageBox::warning(this, "Erreur de saisie", "Le montant doit être un nombre positif.");
        return false;
    }
    if (dateEdit_prevue->date() < dateEdit_commande->date()) {
        QMessageBox::warning(this, "Erreur de saisie", "La date prévisionnelle doit être après la date de commande.");
        return false;
    }
    c = Commande(id, client, produits, qte, dateEdit_commande->date(), lineEdit_adresse->text(),
                 dateEdit_prevue->date(), comboBox_statut->currentText(), montant,
                 lineEdit_transporteur->text());
    return true;
}

void MainWindow::ajouterCommande()
{
    Commande c;
    if (!lireFormulaire(c, 0)) return;
    if (c.ajouter()) {
        chargerDonnees();
        viderFormulaireCommande();
        QMessageBox::information(this, "Succès", QString("Commande n° %1 ajoutée !").arg(c.getId()));
    } else {
        QMessageBox::critical(this, "Erreur", "Impossible d'ajouter.\n" + c.erreur());
    }
}

void MainWindow::modifierCommande()
{
    // L'identifiant vient du formulaire (rempli au clic, vidé par « Vider ») :
    // on ne modifie jamais une ligne restée « courante » après un Vider.
    const int id = lineEdit_id->text().toInt();
    if (id <= 0) {
        QMessageBox::warning(this, "Sélection requise", "Veuillez sélectionner une commande.");
        return;
    }
    Commande c;
    if (!lireFormulaire(c, id)) return;
    if (c.modifier()) {
        chargerDonnees();
        QMessageBox::information(this, "Succès", "Commande modifiée !");
    } else {
        QMessageBox::critical(this, "Erreur", "Impossible de modifier.\n" + c.erreur());
    }
}

void MainWindow::supprimerCommande()
{
    const int id = lineEdit_id->text().toInt();
    if (id <= 0) {
        QMessageBox::warning(this, "Sélection requise", "Veuillez sélectionner une commande.");
        return;
    }
    if (QMessageBox::question(this, "Confirmation", QString("Supprimer la commande n° %1 ?").arg(id),
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;
    Commande c;
    if (c.supprimer(id)) {
        chargerDonnees();
        viderFormulaireCommande();
        QMessageBox::information(this, "Supprimé", "Commande supprimée.");
    } else {
        QMessageBox::critical(this, "Erreur", "Impossible de supprimer.\n" + c.erreur());
    }
}

void MainWindow::viderFormulaireCommande()
{
    lineEdit_id->clear();
    lineEdit_client->clear();
    lineEdit_produit->clear();
    lineEdit_quantite->clear();
    lineEdit_adresse->clear();
    lineEdit_montant->clear();
    lineEdit_transporteur->clear();
    dateEdit_commande->setDate(QDate::currentDate());
    dateEdit_prevue->setDate(QDate::currentDate().addDays(5));
    comboBox_statut->setCurrentIndex(0);
    tableViewCommandes->clearSelection();
    tableViewCommandes->setCurrentIndex(QModelIndex());
}

void MainWindow::selectionCommande(const QModelIndex &index)
{
    if (!index.isValid()) return;
    const int r = index.row();
    lineEdit_id->setText(valeur(r, C_ID));
    lineEdit_client->setText(valeur(r, C_CLIENT));
    lineEdit_produit->setText(valeur(r, C_PRODUITS));
    lineEdit_quantite->setText(valeur(r, C_QTE));
    dateEdit_commande->setDate(QDate::fromString(valeur(r, C_DATE), "yyyy-MM-dd"));
    lineEdit_adresse->setText(valeur(r, C_ADRESSE));
    dateEdit_prevue->setDate(QDate::fromString(valeur(r, C_PREVUE), "yyyy-MM-dd"));
    comboBox_statut->setCurrentText(valeur(r, C_STATUT));
    lineEdit_montant->setText(valeur(r, C_MONTANT));
    lineEdit_transporteur->setText(valeur(r, C_TRANSP));
}

void MainWindow::trierParDate()
{
    triDate = !triDate;
    pushButton_trierDate->setText(triDate ? "Trier par ID" : "Trier par Date");
    chargerDonnees();
}

void MainWindow::exporterCsv()
{
    QString filename = QFileDialog::getSaveFileName(this, "Exporter en CSV", "commandes.csv", "CSV Files (*.csv)");
    if (filename.isEmpty()) return;
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Erreur", "Impossible d'écrire le fichier.");
        return;
    }
    if (!model) return;
    QTextStream out(&file);
    out << QChar(0xFEFF);                                // BOM : accents corrects dans Excel
    out << "ID;Client;Produits;Qte;DateCmd;Adresse;DatePrev;Statut;Montant;Transporteur\n";
    for (int i = 0; i < model->rowCount(); ++i) {
        QStringList rowData;
        for (int j = 0; j <= C_TRANSP; ++j)
            rowData << valeur(i, j).replace(';', ',');
        out << rowData.join(";") << "\n";
    }
    QMessageBox::information(this, "Export Réussi", "Fichier CSV exporté !");
}

void MainWindow::verifierRetards()
{
    int retardCount = 0;
    const QDate aujourdhui = QDate::currentDate();
    for (int i = 0; i < model->rowCount(); ++i) {
        const QString statut = valeur(i, C_STATUT);
        const QDate datePrev = QDate::fromString(valeur(i, C_PREVUE), "yyyy-MM-dd");
        if (statut != "Livrée" && statut != "Annulée" && datePrev.isValid() && datePrev < aujourdhui)
            retardCount++;
    }
    QMessageBox::information(this, "Suivi", QString("%1 commande(s) active(s) en retard.").arg(retardCount));
}

void MainWindow::calculerStatistiques()
{
    double caTotal = 0.0;
    int attente = 0, preparation = 0, expediee = 0, livree = 0;
    for (int i = 0; i < model->rowCount(); ++i) {
        const QString statut = valeur(i, C_STATUT);
        if (statut != "Annulée")                         // une commande annulée ne rapporte rien
            caTotal += model->index(i, C_MONTANT).data().toDouble();
        if (statut == "En attente") attente++;
        else if (statut == "En préparation") preparation++;
        else if (statut == "Expédiée") expediee++;
        else if (statut == "Livrée") livree++;
    }
    label_caTotal->setText(QString("Chiffre d'Affaires Total : %1 TND").arg(caTotal, 0, 'f', 2));
    label_statsDetails->setText(QString("En attente: %1 | En préparation: %2 | Expédiées: %3 | Livrées: %4")
                                    .arg(attente).arg(preparation).arg(expediee).arg(livree));
}

void MainWindow::detecterCommandesInhabituelles()
{
    tableWidget_alertes->setRowCount(0);
    for (int i = 0; i < model->rowCount(); ++i) {
        if (model->index(i, C_QTE).data().toInt() > 200) {
            const int row = tableWidget_alertes->rowCount();
            tableWidget_alertes->insertRow(row);
            tableWidget_alertes->setItem(row, 0, new QTableWidgetItem(valeur(i, C_ID)));
            tableWidget_alertes->setItem(row, 1, new QTableWidgetItem(valeur(i, C_CLIENT)));
            tableWidget_alertes->setItem(row, 2, new QTableWidgetItem("Quantité élevée (> 200 unités)"));
        }
    }
}
