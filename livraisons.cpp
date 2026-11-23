#include "livraisons.h"
#include "ui_mainwindow.h"
#include "historiquelivraison.h"
#include "cartetournee.h"
#include "session.h"
#include "animationlivraison.h"
#include "theme.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QColor>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPrinter>
#include <QSignalBlocker>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QTabBar>
#include <QLocale>

#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>

namespace
{
const QString FORMAT_DATE = "dd/MM/yyyy HH:mm";

// Charte graphique du cahier de spécification
QColor couleurStatut(const QString &statut)
{
    if (statut == "Planifiée") return Theme::sombre() ? QColor("#3D8BD9") : QColor("#0A4D8C");   // bleu éclairci la nuit
    if (statut == "En cours")  return QColor("#00B4D8");
    if (statut == "Livrée")    return QColor("#2ECC71");
    if (statut == "Retardée")  return QColor("#F39C12");
    if (statut == "Échouée")   return QColor("#E74C3C");
    return QColor("#7F8C8D");
}

// Colonne « Statut » dessinée sous forme de pastille colorée
class PastilleStatutDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override
    {
        QStyleOptionViewItem o(opt);
        initStyleOption(&o, idx);
        const QString texte = o.text;
        o.text.clear();
        const QStyle *st = opt.widget ? opt.widget->style() : QApplication::style();
        st->drawControl(QStyle::CE_ItemViewItem, &o, p, opt.widget);
        if (texte.isEmpty()) return;

        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        QFont f = opt.font;
        f.setBold(true);
        f.setPointSizeF(8.5);
        p->setFont(f);
        const int largeur = p->fontMetrics().horizontalAdvance(texte) + 22;
        const QRect r(opt.rect.left() + 6, opt.rect.center().y() - 11, largeur, 22);
        QColor fond = couleurStatut(texte);
        fond.setAlpha(38);
        p->setPen(Qt::NoPen);
        p->setBrush(fond);
        p->drawRoundedRect(r, 11, 11);
        p->setBrush(couleurStatut(texte));
        p->drawEllipse(QPoint(r.left() + 10, r.center().y() + 1), 3, 3);
        p->setPen(Theme::sombre() ? couleurStatut(texte).lighter(150) : couleurStatut(texte).darker(115));
        p->drawText(r.adjusted(16, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, texte);
        p->restore();
    }
    QSize sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &idx) const override
    {
        QSize s = QStyledItemDelegate::sizeHint(opt, idx);
        return {s.width() + 34, qMax(s.height(), 34)};
    }
};

// Couleurs des graphiques Qt Charts selon le thème jour / sombre
void styliserGraphique(QChart *c)
{
    const bool nuit = Theme::sombre();
    const QColor texte = nuit ? QColor("#E3F1F8") : QColor("#14324A");
    const QColor grille = nuit ? QColor("#24405A") : QColor("#D6E9F2");
    c->setBackgroundBrush(nuit ? QColor("#13263A") : QColor(Qt::white));
    c->setTitleBrush(texte);
    c->legend()->setLabelColor(texte);
    for (QAbstractAxis *axe : c->axes()) {
        axe->setLabelsColor(texte);
        axe->setGridLineColor(grille);
        axe->setLinePenColor(grille);
    }
}

QString duree(double minutes)
{
    if (minutes < 0) return "—";
    if (minutes < 60) return QString("%1 min").arg(qRound(minutes));
    const int h = int(minutes / 60);
    const int m = qRound(minutes - h * 60);
    return m ? QString("%1 h %2").arg(h).arg(m, 2, 10, QChar('0')) : QString("%1 h").arg(h);
}

QString pourcentage(int n, int total)
{
    return total > 0 ? QString("%1 %").arg(qRound(100.0 * n / total)) : QString("—");
}

QString enteteHtml(const QString &titre)
{
    return "<table width='100%' cellspacing='0' cellpadding='0'><tr>"
           "<td><span style='font-size:20pt;font-weight:bold;color:#0A4D8C'>WaterFlow</span><br>"
           "<span style='color:#00B4D8;font-size:9pt'>Smart Water Factory — Logistique de livraison</span></td>"
           "<td align='right' style='color:#5B6B7B;font-size:9pt'>Édité le "
           + QDateTime::currentDateTime().toString("dd/MM/yyyy à HH:mm") + "<br>par "
           + Session::utilisateur().toHtmlEscaped() + "</td></tr></table>"
           "<hr style='border:0;border-top:2px solid #0A4D8C'>"
           "<h2 style='color:#0A4D8C;margin:8px 0'>" + titre + "</h2>";
}

void imprimerPdf(const QString &html, const QString &fichier, QPageLayout::Orientation orientation)
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fichier);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(orientation);
    printer.setPageMargins(QMarginsF(14, 14, 14, 14), QPageLayout::Millimeter);
    QTextDocument doc;
    doc.setDefaultFont(QFont("Segoe UI", 9));
    doc.setHtml(html);
    doc.print(&printer);
}
} // namespace

// =====================================================================================
Livraisons::Livraisons(QWidget *parent) : QWidget(parent) {}

Livraisons::~Livraisons() {}

// Relie la page aux widgets du mainwindow.ui (appelée par MainWindow après setupUi)
void Livraisons::initialiser(Ui::MainWindow *u)
{
    ui = u;

    // Objets SQL du module (historique, ordre de tournée, index) : créés si absents
    QString erreurSchema;
    if (!HistoriqueLivraison::preparerSchema(&erreurSchema))
        qWarning() << "Traçabilité indisponible :" << erreurSchema;

    configurerApparence();

    // Zones (référentiel géographique)
    ui->livComboZone->addItems(Geo::zones());
    ui->livComboFiltreZone->addItem("Toutes les zones");
    ui->livComboFiltreZone->addItems(Geo::zones());

    // Tableau principal
    ui->livTableView->setItemDelegateForColumn(COL_STATUT, new PastilleStatutDelegate(ui->livTableView));
    ui->livTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->livTableView->verticalHeader()->setDefaultSectionSize(38);
    ui->livTableHistorique->verticalHeader()->setDefaultSectionSize(32);
    ui->livTableHistorique->setItemDelegateForColumn(4, new PastilleStatutDelegate(ui->livTableHistorique));

    // Graphiques (Qt Charts)
    chartStatut = new QChartView;
    chartStatut->setRenderHint(QPainter::Antialiasing);
    ui->livLayoutChartStatut->addWidget(chartStatut);
    chartLivreur = new QChartView;
    chartLivreur->setRenderHint(QPainter::Antialiasing);
    ui->livLayoutChartLivreur->addWidget(chartLivreur);

    // Tournée
    carte = new CarteTournee;
    ui->livLayoutCarte->addWidget(carte);
    ui->livDateTournee->setDate(QDate::currentDate());
    ui->livTableTournee->setColumnCount(8);
    ui->livTableTournee->setHorizontalHeaderLabels({"Ordre", "N°", "Client", "Destination", "Km",
                                                 "Arrivée prévue", "ETA", "Écart"});
    ui->livTableTournee->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->livTableTournee->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->livTableTournee->verticalHeader()->setDefaultSectionSize(34);

    viderFormulaire();
    rechargerListes();
    actualiserHistorique();

    // CRUD
    connect(ui->livBtnNouveau,    &QPushButton::clicked, this, &Livraisons::nouveau);
    connect(ui->livBtnAjouter,    &QPushButton::clicked, this, &Livraisons::ajouter);
    connect(ui->livBtnModifier,   &QPushButton::clicked, this, &Livraisons::modifier);
    connect(ui->livBtnSupprimer,  &QPushButton::clicked, this, &Livraisons::supprimer);
    connect(ui->livBtnPdf,        &QPushButton::clicked, this, &Livraisons::exporterBordereau);
    connect(ui->livBtnExporterListe, &QPushButton::clicked, this, &Livraisons::exporterListe);
    connect(ui->livTableView,     &QTableView::clicked,  this, &Livraisons::remplirFormulaire);
    connect(ui->livComboCommande, &QComboBox::currentIndexChanged, this, &Livraisons::commandeChoisie);
    // Recherche / filtres / tri : mise à jour immédiate
    connect(ui->livLineEditRecherche, &QLineEdit::textChanged, this, &Livraisons::actualiser);
    connect(ui->livComboFiltreStatut, &QComboBox::currentIndexChanged, this, &Livraisons::actualiser);
    connect(ui->livComboFiltreZone,   &QComboBox::currentIndexChanged, this, &Livraisons::actualiser);
    connect(ui->livComboTri,          &QComboBox::currentIndexChanged, this, &Livraisons::actualiser);
    // Estimation du trajet dans le formulaire
    connect(ui->livLineEditArrivee, &QLineEdit::textChanged, this, &Livraisons::majEstimation);
    connect(ui->livComboZone, &QComboBox::currentIndexChanged, this, &Livraisons::majEstimation);
    connect(ui->livDateTimeEditDepart, &QDateTimeEdit::dateTimeChanged, this, &Livraisons::majEstimation);
    // Bandeau d'alerte
    connect(ui->livBtnVoirRetards, &QPushButton::clicked, this, [this] {
        ui->livTabsLivraison->setCurrentWidget(ui->livTabPlanification);
        ui->livComboTri->setCurrentIndex(Livraison::TriStatut);
    });
    // Onglets
    connect(ui->livTabsLivraison, &QTabWidget::currentChanged, this, [this](int) {
        if (ui->livTabsLivraison->currentWidget() == ui->livTabStatistiques) rafraichirStatistiques();
        if (ui->livTabsLivraison->currentWidget() == ui->livTabTracabilite) actualiserHistorique();
    });
    // Tournées & ETA
    connect(ui->livBtnCalculerTournee, &QPushButton::clicked, this, &Livraisons::calculerTournee);
    connect(ui->livBtnOptimiser, &QPushButton::clicked, this, &Livraisons::optimiserTournee);
    connect(ui->livBtnEnregistrerItineraire, &QPushButton::clicked, this, &Livraisons::validerTournee);
    connect(ui->livBtnAlerterClients, &QPushButton::clicked, this, &Livraisons::alerterClients);
    connect(ui->livComboTourneeLivreur, &QComboBox::currentIndexChanged, this, &Livraisons::calculerTournee);
    connect(ui->livDateTournee, &QDateEdit::dateChanged, this, &Livraisons::calculerTournee);
    // Traçabilité
    connect(ui->livLineEditHistorique, &QLineEdit::textChanged, this, &Livraisons::actualiserHistorique);
    connect(ui->livCheckHistoriqueSelection, &QCheckBox::toggled, this, &Livraisons::actualiserHistorique);
    connect(ui->livBtnActualiserHistorique, &QPushButton::clicked, this, &Livraisons::actualiserHistorique);

    // Suivi en temps réel : recalcul de l'ETA et des alertes chaque minute
    minuterie = new QTimer(this);
    minuterie->setInterval(60 * 1000);
    connect(minuterie, &QTimer::timeout, this, &Livraisons::tic);
    minuterie->start();
    tic();
}


void Livraisons::configurerApparence()
{
    const QList<QFrame *> cartes = {ui->livKpiTotal, ui->livKpiEnCours, ui->livKpiRetard, ui->livKpiLivrees, ui->livKpiPonctualite,
                                    ui->livStatDelai, ui->livStatDuree, ui->livStatCible, ui->livStatPonctualite};
    for (QFrame *f : cartes) {
        f->setProperty("role", "kpi");
        const auto labels = f->findChildren<QLabel *>();
        for (QLabel *l : labels)
            l->setProperty("role", l->objectName().endsWith("Valeur") ? "kpiValeur" : "kpiTexte");
    }
    ui->livKpiRetard->setProperty("accent", "orange");
    ui->livKpiLivrees->setProperty("accent", "vert");
    ui->livKpiEnCours->setProperty("accent", "cyan");
    for (QFrame *f : {ui->livCardChartStatut, ui->livCardChartLivreur, ui->livCardCarte})
        f->setProperty("role", "carte");

    ui->livBtnSupprimer->setProperty("variante", "danger");
    ui->livBtnPdf->setProperty("variante", "sombre");
    ui->livBtnExporterListe->setProperty("variante", "sombre");
    ui->livBtnAlerterClients->setProperty("variante", "alerte");
    ui->livBtnNouveau->setProperty("variante", "clair");

    const QStyle *st = QApplication::style();
    ui->livBtnNouveau->setIcon(st->standardIcon(QStyle::SP_FileIcon));
    ui->livBtnAjouter->setIcon(st->standardIcon(QStyle::SP_DialogApplyButton));
    ui->livBtnModifier->setIcon(st->standardIcon(QStyle::SP_DialogSaveButton));
    ui->livBtnSupprimer->setIcon(st->standardIcon(QStyle::SP_TrashIcon));
    ui->livBtnPdf->setIcon(st->standardIcon(QStyle::SP_FileDialogDetailedView));
    ui->livBtnExporterListe->setIcon(st->standardIcon(QStyle::SP_FileDialogDetailedView));
    ui->livBtnOptimiser->setIcon(st->standardIcon(QStyle::SP_BrowserReload));
    ui->livBtnAlerterClients->setIcon(st->standardIcon(QStyle::SP_MessageBoxWarning));
    ui->livFrameAlerteRetard->hide();

    // Fond animé « eau » visible derrière les onglets : conteneurs rendus transparents
    new AnimationLivraison(this, AnimationLivraison::Clair);                 // carte discrète
    new AnimationLivraison(ui->livHeroLivraison, AnimationLivraison::Bandeau);  // en-tête : route + camions
    QList<QWidget *> transparents = {ui->livTabsLivraison, ui->livTabsLivraison->tabBar(), ui->livTabPlanification,
                                     ui->livTabStatistiques, ui->livTabTournee, ui->livTabTracabilite};
    if (auto *pile = ui->livTabsLivraison->findChild<QStackedWidget *>()) transparents << pile;
    for (QWidget *w : transparents) w->setProperty("fond", "transparent");

    // Les propriétés dynamiques ci-dessus sont lues par la feuille de style : on la réapplique
    for (QWidget *w : findChildren<QWidget *>()) {
        w->style()->unpolish(w);
        w->style()->polish(w);
    }
}

void Livraisons::appliquerTheme()
{
    rafraichirStatistiques();
    carte->update();
    update();
}

void Livraisons::rechargerListes()
{
    remplirCombos(ui->livComboCommande->currentData().toInt(), ui->livComboChauffeur->currentData().toInt());
    remplirComboTournee();
    actualiser();
    rafraichirIndicateurs();
}

// Livreurs = employés actifs au poste « Livreur » ; commandes = à livrer.
void Livraisons::remplirCombos(int idCommandeEnCours, int idLivreurEnCours)
{
    const QSignalBlocker b1(ui->livComboCommande), b2(ui->livComboChauffeur);

    ui->livComboChauffeur->clear();
    for (const ChoixListe &c : Livraison::listeLivreurs())
        ui->livComboChauffeur->addItem(c.extra.isEmpty() ? c.libelle : c.libelle + "  (" + c.extra + ")", c.id);

    ui->livComboCommande->clear();
    for (const ChoixListe &c : Livraison::listeCommandes(idCommandeEnCours)) {
        ui->livComboCommande->addItem(c.libelle, c.id);
        ui->livComboCommande->setItemData(ui->livComboCommande->count() - 1, c.extra, Qt::UserRole + 1);
    }

    int i = ui->livComboChauffeur->findData(idLivreurEnCours);
    ui->livComboChauffeur->setCurrentIndex(i >= 0 ? i : (ui->livComboChauffeur->count() ? 0 : -1));
    i = ui->livComboCommande->findData(idCommandeEnCours);
    ui->livComboCommande->setCurrentIndex(i >= 0 ? i : (ui->livComboCommande->count() ? 0 : -1));

    ui->livComboChauffeur->setPlaceholderText("Aucun livreur : ajoutez-en dans Personnel");
    ui->livComboCommande->setPlaceholderText("Aucune commande à livrer");
}

void Livraisons::remplirComboTournee()
{
    const int courant = ui->livComboTourneeLivreur->currentData().toInt();
    const QSignalBlocker b(ui->livComboTourneeLivreur);
    ui->livComboTourneeLivreur->clear();
    for (const ChoixListe &c : Livraison::listeLivreurs())
        ui->livComboTourneeLivreur->addItem(c.libelle, c.id);
    const int i = ui->livComboTourneeLivreur->findData(courant);
    if (i >= 0) ui->livComboTourneeLivreur->setCurrentIndex(i);
}

// Choix d'une commande : propose son adresse de livraison
void Livraisons::commandeChoisie(int index)
{
    if (index < 0) return;
    const QString adr = ui->livComboCommande->itemData(index, Qt::UserRole + 1).toString();
    if (!adr.isEmpty() && idSelectionne() < 0) {
        ui->livLineEditArrivee->setText(adr);
        // zone déduite de l'adresse quand elle est connue
        const PointGeo p = Geo::localiser(adr, QString());
        for (const QString &z : Geo::zones())
            if (Geo::localiser(z, QString()).nom == p.nom || adr.contains(z, Qt::CaseInsensitive)) {
                ui->livComboZone->setCurrentText(z);
                break;
            }
    }
}

// Estimation du trajet dépôt -> client (aide à la saisie de l'heure d'arrivée)
void Livraisons::majEstimation()
{
    const PointGeo dest = Geo::localiser(ui->livLineEditArrivee->text(), ui->livComboZone->currentText());
    const double km = Geo::distanceKm(Geo::depot(), dest);
    const int minutes = qRound(km / Itineraire::VITESSE_MOYENNE_KMH * 60.0);
    ui->livLabelEstimation->setText(QString("≈ %1 km · %2 de trajet").arg(km, 0, 'f', 1).arg(duree(minutes)));

    // Nouvelle livraison : on propose une arrivée cohérente (trajet + 30 min de marge)
    if (idSelectionne() < 0) {
        const QDateTime depart = ui->livDateTimeEditDepart->dateTime();
        if (ui->livDateTimeEditArrivee->dateTime() < depart.addSecs(minutes * 60))
            ui->livDateTimeEditArrivee->setDateTime(depart.addSecs((minutes + 30) * 60));
    }
}

// Une livraison n'est possible que si Personnel et Commandes ont fourni les données
bool Livraisons::prerequisOk()
{
    if (ui->livComboChauffeur->count() == 0) {
        QMessageBox::warning(this, "Livreur manquant",
            "Aucun livreur disponible.\n\nDans Personnel, ajoutez un employé au poste « "
            + Livraison::POSTE_LIVREUR + " » avec le statut « Actif ».");
        return false;
    }
    if (ui->livComboCommande->count() == 0) {
        QMessageBox::warning(this, "Commande manquante",
            "Aucune commande à livrer.\n\nDans Commandes, ajoutez une commande "
            "(statut autre que « Livrée » ou « Annulée »).");
        return false;
    }
    return true;
}

void Livraisons::afficherModele(QSqlQueryModel *nouveau)
{
    QSqlQueryModel *ancien = model;
    QItemSelectionModel *ancienneSelection = ui->livTableView->selectionModel();

    model = nouveau;
    model->setParent(this);
    ui->livTableView->setModel(model);
    for (int c = COL_VEHICULE; c < NB_COLONNES_LIVRAISON; ++c)
        ui->livTableView->setColumnHidden(c, true);
    ui->livTableView->setColumnHidden(COL_ZONE, true);          // visible dans le formulaire et filtrable
    auto *h = ui->livTableView->horizontalHeader();
    h->setSectionResizeMode(QHeaderView::ResizeToContents);
    h->setSectionResizeMode(COL_CLIENT, QHeaderView::Stretch);

    delete ancienneSelection;
    delete ancien;
}

QString Livraisons::valeur(int ligne, int colonne) const
{
    return model ? model->data(model->index(ligne, colonne)).toString() : QString();
}

void Livraisons::actualiser()
{
    afficherModele(Livraison::rechercher(ui->livLineEditRecherche->text(), ui->livComboFiltreStatut->currentText(),
                                         ui->livComboFiltreZone->currentText(),
                                         Livraison::Tri(ui->livComboTri->currentIndex())));
    ui->livGroupDetails->setTitle("Nouvelle livraison");
}

void Livraisons::viderFormulaire()
{
    ui->livLineEditVehicule->clear();
    ui->livLineEditDepart->setText(Geo::depot().nom);
    ui->livLineEditArrivee->clear();
    const QDateTime debut(QDate::currentDate().addDays(1), QTime(8, 0));
    ui->livDateTimeEditDepart->setDateTime(debut);
    ui->livDateTimeEditArrivee->setDateTime(debut.addSecs(3 * 3600));
    ui->livComboStatut->setCurrentIndex(0);
    ui->livTableView->clearSelection();
    ui->livTableView->setCurrentIndex(QModelIndex());
    ui->livGroupDetails->setTitle("Nouvelle livraison");
    ui->livBtnAjouter->setEnabled(true);
    commandeChoisie(ui->livComboCommande->currentIndex());
    majEstimation();
}

void Livraisons::nouveau()
{
    viderFormulaire();
    remplirCombos(0, 0);
    commandeChoisie(ui->livComboCommande->currentIndex());
    ui->livLineEditVehicule->setFocus();
}

Livraison Livraisons::livraisonDepuisFormulaire(int id) const
{
    QString livreur = ui->livComboChauffeur->currentText();
    const int p = livreur.indexOf("  (");
    if (p > 0) livreur.truncate(p);                    // retire le téléphone affiché
    return Livraison(id,
                     ui->livComboCommande->currentData().toInt(),
                     ui->livComboChauffeur->currentData().toInt(),
                     livreur,
                     ui->livLineEditVehicule->text(),
                     ui->livComboZone->currentText(),
                     ui->livLineEditDepart->text(),
                     ui->livLineEditArrivee->text(),
                     ui->livDateTimeEditDepart->dateTime(),
                     ui->livDateTimeEditArrivee->dateTime(),
                     ui->livComboStatut->currentText());
}

int Livraisons::idSelectionne() const
{
    if (!model) return -1;
    const QModelIndex idx = ui->livTableView->currentIndex();
    if (!idx.isValid()) return -1;
    return valeur(idx.row(), COL_ID).toInt();
}

// ---------- AJOUTER (Create) ----------
void Livraisons::ajouter()
{
    if (!prerequisOk()) return;
    if (idSelectionne() >= 0) {
        QMessageBox::information(this, "Planifier",
            "Une livraison existante est sélectionnée.\nCliquez sur « Nouveau » pour en planifier une autre, "
            "ou sur « Enregistrer » pour modifier celle-ci.");
        return;
    }
    Livraison L = livraisonDepuisFormulaire(0);
    const QString erreur = L.verifier(true);
    if (!erreur.isEmpty()) {
        QMessageBox::warning(this, "Contrôle de saisie", erreur);
        return;
    }
    if (L.ajouter()) {
        viderFormulaire();
        remplirCombos(0, 0);
        actualiser();
        rafraichirIndicateurs();
        QMessageBox::information(this, "Livraison planifiée",
            QString("La livraison n° %1 a été planifiée.\nLa commande liée est passée « %2 ».")
                .arg(L.getId()).arg(Livraison::statutCommandePour(L.getStatut())));
    } else {
        QMessageBox::critical(this, "Échec", "La livraison n'a pas été enregistrée.\n\n" + L.getErreurSql());
    }
}

// ---------- MODIFIER (Update) ----------
void Livraisons::modifier()
{
    const int id = idSelectionne();
    if (id < 0) {
        QMessageBox::warning(this, "Sélection requise", "Sélectionnez une livraison dans le tableau.");
        return;
    }
    Livraison L = livraisonDepuisFormulaire(id);
    const QString erreur = L.verifier();
    if (!erreur.isEmpty()) {
        QMessageBox::warning(this, "Contrôle de saisie", erreur);
        return;
    }
    if (L.modifier()) {
        remplirCombos(L.getIdCommande(), L.getIdEmploye());
        actualiser();
        rafraichirIndicateurs();
        if (!m_tournee.isEmpty()) calculerTournee();
        QMessageBox::information(this, "Modification enregistrée",
            QString("La livraison n° %1 a été mise à jour.").arg(id));
    } else {
        QMessageBox::critical(this, "Échec", "La modification n'a pas été enregistrée.\n\n" + L.getErreurSql());
    }
}

// ---------- SUPPRIMER (Delete) ----------
void Livraisons::supprimer()
{
    const int id = idSelectionne();
    if (id < 0) {
        QMessageBox::warning(this, "Sélection requise", "Sélectionnez une livraison dans le tableau.");
        return;
    }
    QMessageBox box(QMessageBox::Question, "Confirmer la suppression",
        QString("Supprimer définitivement la livraison n° %1 ?\n\n"
                "Sa commande redeviendra « En attente » si elle n'a pas été livrée.\n"
                "La suppression reste visible dans l'onglet Traçabilité.").arg(id),
        QMessageBox::NoButton, this);
    QPushButton *oui = box.addButton("Supprimer", QMessageBox::AcceptRole);
    box.setDefaultButton(box.addButton("Annuler", QMessageBox::RejectRole));
    box.exec();
    if (box.clickedButton() != oui) return;

    if (Ltmp.supprimer(id)) {
        viderFormulaire();
        remplirCombos(0, 0);
        actualiser();
        rafraichirIndicateurs();
        QMessageBox::information(this, "Suppression", QString("La livraison n° %1 a été supprimée.").arg(id));
    } else {
        QMessageBox::critical(this, "Échec", "Suppression non effectuée.\n\n" + Ltmp.getErreurSql());
    }
}

// ---------- CLIC SUR UNE LIGNE : remplit le formulaire ----------
void Livraisons::remplirFormulaire(const QModelIndex &index)
{
    if (!index.isValid()) return;
    const int r = index.row();
    auto v = [&](int c) { return valeur(r, c); };

    remplirCombos(v(COL_COMMANDE).toInt(), v(COL_ID_LIVREUR).toInt());
    if (ui->livComboChauffeur->findData(v(COL_ID_LIVREUR).toInt()) < 0 && !v(COL_LIVREUR).isEmpty()) {
        // livreur devenu inactif : on l'affiche quand même pour ne pas perdre l'information
        ui->livComboChauffeur->addItem(v(COL_LIVREUR) + "  (inactif)", v(COL_ID_LIVREUR).toInt());
        ui->livComboChauffeur->setCurrentIndex(ui->livComboChauffeur->count() - 1);
    }
    if (ui->livComboZone->findText(v(COL_ZONE)) < 0 && !v(COL_ZONE).isEmpty())
        ui->livComboZone->addItem(v(COL_ZONE));
    ui->livComboZone->setCurrentText(v(COL_ZONE));
    ui->livComboStatut->setCurrentText(v(COL_STATUT));
    ui->livLineEditVehicule->setText(v(COL_VEHICULE));
    ui->livLineEditDepart->setText(v(COL_ADR_DEPART));
    ui->livLineEditArrivee->setText(v(COL_ADR_LIVRAISON));
    ui->livDateTimeEditDepart->setDateTime(QDateTime::fromString(v(COL_DEPART), FORMAT_DATE));
    ui->livDateTimeEditArrivee->setDateTime(QDateTime::fromString(v(COL_ARRIVEE), FORMAT_DATE));
    ui->livGroupDetails->setTitle(QString("Livraison n° %1 — %2").arg(v(COL_ID), v(COL_CLIENT)));
    majEstimation();
    if (ui->livCheckHistoriqueSelection->isChecked()) actualiserHistorique();
}

// ---------- INDICATEURS + BANDEAU D'ALERTE ----------
void Livraisons::rafraichirIndicateurs()
{
    const StatsLivraison s = Livraison::statistiques();
    int planifEnCours = 0, retardees = 0, livrees = 0;
    for (const auto &p : s.parStatut) {
        if (p.first == "Planifiée" || p.first == "En cours") planifEnCours += p.second;
        if (p.first == "Retardée") retardees = p.second;
        if (p.first == "Livrée") livrees = p.second;
    }
    const int depassees = Livraison::nombreEnRetard();
    ui->livKpiTotalValeur->setText(QString::number(s.total));
    ui->livKpiEnCoursValeur->setText(QString::number(planifEnCours));
    ui->livKpiRetardValeur->setText(QString::number(qMax(retardees, depassees)));
    ui->livKpiLivreesValeur->setText(QString::number(livrees));
    ui->livKpiPonctualiteValeur->setText(pourcentage(s.livreesALHeure, s.livrees));

    if (depassees > 0) {
        ui->livLabelAlerteRetard->setText(QString("<b>Alerte retard :</b> %1 livraison(s) active(s) ont dépassé "
                                               "leur heure de livraison prévue.").arg(depassees));
        ui->livFrameAlerteRetard->show();
    } else {
        ui->livFrameAlerteRetard->hide();
    }
}

// ---------- STATISTIQUES (Qt Charts) ----------
void Livraisons::rafraichirStatistiques()
{
    const StatsLivraison s = Livraison::statistiques();

    ui->livStatDelaiValeur->setText(s.delaiMoyenHeures < 0 ? "—" : duree(s.delaiMoyenHeures * 60));
    ui->livStatDureeValeur->setText(duree(s.dureeMoyenneMinutes));
    ui->livStatCibleValeur->setText(pourcentage(s.livreesDansCible, s.livrees));
    ui->livStatPonctualiteValeur->setText(pourcentage(s.livreesALHeure, s.livrees));

    // Répartition par statut (anneau)
    auto *pie = new QPieSeries;
    pie->setHoleSize(0.45);
    pie->setPieSize(0.62);
    for (const auto &p : s.parStatut) {
        if (p.second == 0) continue;
        QPieSlice *slice = pie->append(QString("%1 (%2)").arg(p.first).arg(p.second), p.second);
        slice->setColor(couleurStatut(p.first));
        slice->setBorderColor(Qt::white);
        slice->setLabelVisible(true);
        slice->setLabelPosition(QPieSlice::LabelOutside);
        slice->setLabelColor(Theme::sombre() ? QColor("#E3F1F8") : QColor("#14324A"));
    }
    auto *c1 = new QChart;
    c1->addSeries(pie);
    c1->setTitle(QString("Livraisons par statut — %1 au total").arg(s.total));
    c1->setTitleFont(QFont("Segoe UI", 10, QFont::Bold));
    c1->legend()->setAlignment(Qt::AlignBottom);
    styliserGraphique(c1);
    c1->setBackgroundRoundness(10);
    QChart *ancien1 = chartStatut->chart();
    chartStatut->setChart(c1);
    delete ancien1;

    // Par transporteur (barres groupées)
    auto *total = new QBarSet("Total");
    auto *livr = new QBarSet("Livrées");
    auto *ret = new QBarSet("En retard");
    total->setColor(Theme::sombre() ? QColor("#3D8BD9") : QColor("#0A4D8C"));
    livr->setColor(QColor("#2ECC71"));
    ret->setColor(QColor("#F39C12"));
    QStringList noms;
    int maxi = 1;
    for (const auto &l : s.parLivreur) {
        noms << l.nom;
        *total << l.total;
        *livr << l.livrees;
        *ret << l.retards;
        maxi = qMax(maxi, l.total);
    }
    auto *bars = new QBarSeries;
    bars->append(total);
    bars->append(livr);
    bars->append(ret);
    bars->setLabelsVisible(true);
    auto *c2 = new QChart;
    c2->addSeries(bars);
    c2->setTitle("Livraisons par transporteur");
    c2->setTitleFont(QFont("Segoe UI", 10, QFont::Bold));
    auto *axeX = new QBarCategoryAxis;
    axeX->append(noms);
    auto *axeY = new QValueAxis;
    axeY->setRange(0, maxi + 1);
    axeY->setTickCount(qMin(maxi + 2, 8));
    axeY->setLabelFormat("%d");
    c2->addAxis(axeX, Qt::AlignBottom);
    c2->addAxis(axeY, Qt::AlignLeft);
    bars->attachAxis(axeX);
    bars->attachAxis(axeY);
    c2->legend()->setAlignment(Qt::AlignBottom);
    styliserGraphique(c2);
    c2->setBackgroundRoundness(10);
    QChart *ancien2 = chartLivreur->chart();
    chartLivreur->setChart(c2);
    delete ancien2;
}

// ---------- TOURNÉE : chargement + ETA ----------
void Livraisons::calculerTournee()
{
    const int idLivreur = ui->livComboTourneeLivreur->currentData().toInt();
    m_tournee = idLivreur > 0 ? Livraison::tournee(idLivreur, ui->livDateTournee->date()) : QVector<Etape>();
    m_tourneeOptimisee = false;

    QVector<PointGeo> pts;
    QVector<int> ordre;
    for (int i = 0; i < m_tournee.size(); ++i) { pts << m_tournee[i].point; ordre << i; }
    m_kmInitial = Itineraire::longueur(Geo::depot(), pts, ordre);

    Itineraire::calculerEta(m_tournee, Geo::depot(), QDateTime::currentDateTime());
    afficherTournee();
}

void Livraisons::optimiserTournee()
{
    if (m_tournee.size() < 2) {
        QMessageBox::information(this, "Optimisation",
            "Il faut au moins deux livraisons dans la tournée pour optimiser l'ordre de passage.");
        return;
    }
    // Les arrêts terminés et celui en cours (le camion y roule déjà) gardent leur place ;
    // on optimise les arrêts restants à partir du dernier point atteint.
    QVector<Etape> faits, restants;
    for (const Etape &e : m_tournee) (e.terminee() || e.statut == "En cours" ? faits : restants) << e;
    PointGeo depart = Geo::depot();
    for (const Etape &e : faits) if (e.statut != "Échouée") depart = e.point;
    if (restants.size() < 2) {
        QMessageBox::information(this, "Optimisation",
            "Il reste moins de deux arrêts à planifier : l'ordre de passage est déjà optimal.");
        return;
    }

    // Heure à laquelle le camion repart du dernier point atteint (ETA courante)
    Itineraire::calculerEta(m_tournee, Geo::depot(), QDateTime::currentDateTime());
    QDateTime heureDepart = QDateTime::currentDateTime();
    for (const Etape &e : m_tournee)
        if ((e.terminee() || e.statut == "En cours") && e.eta.isValid())
            heureDepart = qMax(heureDepart, e.eta.addSecs(Itineraire::DUREE_ARRET_MIN * 60));

    QVector<PointGeo> pts;
    QVector<QDateTime> echeances;
    for (const Etape &e : restants) { pts << e.point; echeances << e.arriveePrevue; }
    const QVector<int> ordre = Itineraire::optimiser(depart, pts, echeances, heureDepart);

    QVector<Etape> resultat = faits;
    for (int i : ordre) resultat << restants[i];
    m_tournee = resultat;
    m_tourneeOptimisee = true;
    Itineraire::calculerEta(m_tournee, Geo::depot(), QDateTime::currentDateTime());
    afficherTournee();
}

void Livraisons::afficherTournee()
{
    auto *t = ui->livTableTournee;
    t->setRowCount(m_tournee.size());
    double km = 0;
    int retards = 0;
    for (int i = 0; i < m_tournee.size(); ++i) {
        const Etape &e = m_tournee[i];
        km += e.statut == "Échouée" ? 0 : e.kmDepuisPrecedent;
        const bool enRetard = !e.terminee() && e.retardMinutes > Itineraire::TOLERANCE_RETARD_MIN;
        if (enRetard) ++retards;

        QString ecart;
        if (!e.eta.isValid()) ecart = "—";
        else if (e.retardMinutes > 0) ecart = "+" + duree(e.retardMinutes);
        else ecart = "à l'heure";

        const QStringList cellules = {
            QString::number(i + 1), QString::number(e.idLivraison),
            e.client.isEmpty() ? "—" : e.client, e.point.nom,
            QString::number(e.kmDepuisPrecedent, 'f', 1),
            e.arriveePrevue.toString("HH:mm"),
            e.eta.isValid() ? e.eta.toString("HH:mm") + (e.statut == "Livrée" ? " (réelle)" : "") : "—",
            e.statut == "Livrée" || e.statut == "Échouée" ? e.statut : ecart};
        for (int c = 0; c < cellules.size(); ++c) {
            auto *item = new QTableWidgetItem(cellules[c]);
            item->setToolTip(e.adresse + " — statut : " + e.statut);
            if (c == 7) {
                item->setForeground(e.statut == "Échouée" ? QColor("#E74C3C")
                                    : enRetard ? QColor("#D35400")
                                    : QColor("#1E9E58"));
                QFont f = item->font();
                f.setBold(true);
                item->setFont(f);
            }
            t->setItem(i, c, item);
        }
    }

    if (m_tournee.isEmpty()) {
        ui->livLabelResumeTournee->setText(ui->livComboTourneeLivreur->count()
            ? "Aucune livraison prévue pour ce livreur ce jour-là."
            : "Aucun livreur actif : ajoutez un employé au poste « Livreur » dans Personnel.");
    } else {
        QString resume = QString("<b>%1 arrêt(s)</b> · <b>%2 km</b> depuis le dépôt · fin estimée <b>%3</b>")
                             .arg(m_tournee.size()).arg(km, 0, 'f', 1)
                             .arg(m_tournee.last().eta.isValid() ? m_tournee.last().eta.toString("HH:mm") : "—");
        if (m_tourneeOptimisee && m_kmInitial > 0.05) {
            const double gain = m_kmInitial - km;
            resume += gain >= 0.1
                ? QString(" · <span style='color:#1E9E58'><b>itinéraire optimisé : %1 km économisés (%2 %)</b></span>")
                      .arg(gain, 0, 'f', 1).arg(qRound(100.0 * gain / m_kmInitial))
                : QString(" · <span style='color:#1E9E58'><b>itinéraire optimisé : priorité aux heures promises "
                          "(%1 km)</b></span>").arg(km, 0, 'f', 1);
        }
        if (retards > 0)
            resume += QString(" · <span style='color:#D35400'><b>⚠ %1 arrêt(s) en retard</b></span>").arg(retards);
        resume += QString("<br><span style='color:#6B8193'>ETA recalculée automatiquement chaque minute "
                          "(vitesse moyenne %1 km/h, %2 min par arrêt) — dernier calcul à %3</span>")
                      .arg(Itineraire::VITESSE_MOYENNE_KMH).arg(Itineraire::DUREE_ARRET_MIN)
                      .arg(QTime::currentTime().toString("HH:mm"));
        ui->livLabelResumeTournee->setText(resume);
    }
    ui->livBtnEnregistrerItineraire->setEnabled(!m_tournee.isEmpty());
    ui->livBtnOptimiser->setEnabled(m_tournee.size() >= 2);
    ui->livBtnAlerterClients->setEnabled(retards > 0);
    carte->setTournee(Geo::depot(), m_tournee);
}

void Livraisons::validerTournee()
{
    if (m_tournee.isEmpty()) return;
    double km = 0;
    for (const Etape &e : m_tournee) km += e.statut == "Échouée" ? 0 : e.kmDepuisPrecedent;
    QString erreur;
    if (Livraison::enregistrerItineraire(m_tournee, km, &erreur)) {
        QMessageBox::information(this, "Tournée validée",
            QString("L'ordre de passage (%1 arrêts, %2 km) est enregistré pour %3.")
                .arg(m_tournee.size()).arg(km, 0, 'f', 1).arg(ui->livComboTourneeLivreur->currentText()));
        actualiserHistorique();
    } else {
        QMessageBox::critical(this, "Échec", "Itinéraire non enregistré.\n\n" + erreur);
    }
}

// Alerte client : passage au statut « Retardée » + message prêt à envoyer + traçabilité
void Livraisons::alerterClients()
{
    QVector<Etape> enRetard;
    for (const Etape &e : m_tournee)
        if (!e.terminee() && e.retardMinutes > Itineraire::TOLERANCE_RETARD_MIN) enRetard << e;
    if (enRetard.isEmpty()) return;

    QStringList messages;
    for (const Etape &e : enRetard)
        messages << QString("Livraison n° %1 — %2\n"
                            "Bonjour, votre livraison WaterFlow (commande n° %3) prévue à %4 arrivera "
                            "avec environ %5 de retard. Nouvelle heure estimée : %6. Merci de votre compréhension.")
                        .arg(e.idLivraison).arg(e.client).arg(e.idCommande)
                        .arg(e.arriveePrevue.toString("HH:mm"), duree(e.retardMinutes), e.eta.toString("HH:mm"));

    QDialog dlg(this);
    dlg.setWindowTitle("Alerte retard — clients concernés");
    dlg.resize(620, 420);
    auto *lay = new QVBoxLayout(&dlg);
    auto *info = new QLabel(QString("<b>%1 client(s)</b> vont être prévenus. Les livraisons passeront au statut "
                                    "« Retardée » et l'alerte sera enregistrée dans la traçabilité.").arg(enRetard.size()));
    info->setWordWrap(true);
    lay->addWidget(info);
    auto *texte = new QPlainTextEdit(messages.join("\n\n"));
    lay->addWidget(texte);
    auto *boutons = new QDialogButtonBox;
    boutons->addButton("Confirmer et copier les messages", QDialogButtonBox::AcceptRole);
    boutons->addButton("Annuler", QDialogButtonBox::RejectRole);
    lay->addWidget(boutons);
    connect(boutons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(boutons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) return;

    QApplication::clipboard()->setText(texte->toPlainText());
    int ok = 0;
    for (const Etape &e : enRetard) {
        const QString motif = QString("Alerte client : retard estimé %1, ETA %2")
                                  .arg(duree(e.retardMinutes), e.eta.toString("dd/MM HH:mm"));
        if (e.statut == "Retardée") {
            HistoriqueLivraison::enregistrer(e.idLivraison, "Alerte client", e.statut, e.statut, motif);
            ++ok;
        } else if (Livraison::changerStatut(e.idLivraison, "Retardée", motif)) {
            ++ok;
        }
    }
    actualiser();
    rafraichirIndicateurs();
    calculerTournee();
    QMessageBox::information(this, "Alerte envoyée",
        QString("%1 livraison(s) signalée(s) en retard.\nLes messages clients sont copiés dans le presse-papiers.")
            .arg(ok));
}

// Suivi en temps réel : horloge, bandeau, ETA
void Livraisons::tic()
{
    ui->livLabelHorloge->setText(QLocale(QLocale::French).toString(QDateTime::currentDateTime(), "dddd d MMMM yyyy · HH:mm"));
    rafraichirIndicateurs();
    if (!m_tournee.isEmpty()) {
        Itineraire::calculerEta(m_tournee, Geo::depot(), QDateTime::currentDateTime());
        afficherTournee();
    }
}

// ---------- TRAÇABILITÉ ----------
void Livraisons::actualiserHistorique()
{
    if (!HistoriqueLivraison::disponible()) {
        ui->livLineEditHistorique->setPlaceholderText("Traçabilité indisponible : table HISTORIQUE_LIVRAISON absente "
                                                   "(exécuter sql/04_livraison_avance.sql)");
        return;
    }
    const int id = ui->livCheckHistoriqueSelection->isChecked() ? qMax(0, idSelectionne()) : 0;
    QSqlQueryModel *nouveau = HistoriqueLivraison::afficher(id, ui->livLineEditHistorique->text());
    nouveau->setParent(this);
    QItemSelectionModel *ancienneSel = ui->livTableHistorique->selectionModel();
    ui->livTableHistorique->setModel(nouveau);
    delete ancienneSel;
    delete modelHistorique;
    modelHistorique = nouveau;
    ui->livTableHistorique->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->livTableHistorique->horizontalHeader()->setStretchLastSection(true);
}

// ---------- BORDEREAU PDF ----------
void Livraisons::exporterBordereau()
{
    const int r = ui->livTableView->currentIndex().row();
    if (r < 0 || !model) {
        QMessageBox::warning(this, "Sélection requise", "Sélectionnez une livraison dans le tableau.");
        return;
    }
    auto t = [&](int c) { const QString v = valeur(r, c); return v.isEmpty() ? QString("—") : v.toHtmlEscaped(); };

    QString fichier = QFileDialog::getSaveFileName(this, "Enregistrer le bordereau",
                                                   "bordereau_livraison_" + valeur(r, COL_ID) + ".pdf", "PDF (*.pdf)");
    if (fichier.isEmpty()) return;
    if (!fichier.endsWith(".pdf", Qt::CaseInsensitive)) fichier += ".pdf";

    const PointGeo dest = Geo::localiser(valeur(r, COL_ADR_LIVRAISON), valeur(r, COL_ZONE));
    const double km = Geo::distanceKm(Geo::depot(), dest);
    auto ligne = [](const QString &k, const QString &v) {
        return "<tr><td width='32%' style='background:#E6F4FA;color:#0A4D8C;font-weight:bold;padding:6px'>" + k
               + "</td><td style='padding:6px;border-bottom:1px solid #E6F4FA'>" + v + "</td></tr>";
    };
    const QString html = enteteHtml("Bordereau de livraison n° " + t(COL_ID))
        + "<p>Statut : <b style='color:" + couleurStatut(valeur(r, COL_STATUT)).name() + "'>" + t(COL_STATUT) + "</b></p>"
        + "<table width='100%' cellspacing='0' cellpadding='0' style='border:1px solid #CFE6F1'>"
        + ligne("Commande", "n° " + t(COL_COMMANDE)) + ligne("Client", t(COL_CLIENT))
        + ligne("Livreur", t(COL_LIVREUR)) + ligne("Véhicule", t(COL_VEHICULE))
        + ligne("Zone", t(COL_ZONE)) + ligne("Adresse de départ", t(COL_ADR_DEPART))
        + ligne("Adresse de livraison", t(COL_ADR_LIVRAISON))
        + ligne("Distance estimée", QString("%1 km").arg(km, 0, 'f', 1))
        + ligne("Départ prévu", t(COL_DEPART)) + ligne("Livraison prévue", t(COL_ARRIVEE))
        + ligne("Livrée le", t(COL_REELLE)) + "</table>"
        + "<br><br><table width='100%' cellspacing='0' cellpadding='10'><tr>"
          "<td width='50%' style='border:1px solid #CFE6F1;height:90px;vertical-align:top'><b>Signature du livreur</b></td>"
          "<td width='50%' style='border:1px solid #CFE6F1;height:90px;vertical-align:top'><b>Signature et cachet du client</b></td>"
          "</tr></table><p style='color:#7F8C8D;font-size:8pt'>Document généré par WaterFlow. "
          "Toute réserve doit être notée sur ce bordereau au moment de la réception.</p>";
    imprimerPdf(html, fichier, QPageLayout::Portrait);
    HistoriqueLivraison::enregistrer(valeur(r, COL_ID).toInt(), "Bordereau PDF", QString(), QString(), fichier);
    QMessageBox::information(this, "PDF", "Bordereau enregistré :\n" + QDir::toNativeSeparators(fichier));
}

// ---------- LISTE PDF (filtres appliqués) ----------
void Livraisons::exporterListe()
{
    if (!model || model->rowCount() == 0) {
        QMessageBox::information(this, "Export", "Aucune livraison à exporter avec les filtres actuels.");
        return;
    }
    QString fichier = QFileDialog::getSaveFileName(this, "Exporter la liste des livraisons",
        "livraisons_" + QDate::currentDate().toString("yyyyMMdd") + ".pdf", "PDF (*.pdf)");
    if (fichier.isEmpty()) return;
    if (!fichier.endsWith(".pdf", Qt::CaseInsensitive)) fichier += ".pdf";

    QString filtres = "Statut : " + ui->livComboFiltreStatut->currentText() + " · Zone : "
                      + ui->livComboFiltreZone->currentText() + " · " + ui->livComboTri->currentText();
    if (!ui->livLineEditRecherche->text().trimmed().isEmpty())
        filtres += " · Recherche : « " + ui->livLineEditRecherche->text().trimmed() + " »";

    const QList<int> colonnes = {COL_ID, COL_COMMANDE, COL_CLIENT, COL_LIVREUR, COL_VEHICULE, COL_ZONE,
                                 COL_DEPART, COL_ARRIVEE, COL_STATUT};
    QString html = enteteHtml(QString("Liste des livraisons (%1)").arg(model->rowCount()))
                   + "<p style='color:#5B6B7B'>" + filtres.toHtmlEscaped() + "</p>"
                   + "<table width='100%' cellspacing='0' cellpadding='5' style='border-collapse:collapse'><tr>";
    for (int c : colonnes)
        html += "<th style='background:#0A4D8C;color:white;text-align:left'>"
                + model->headerData(c, Qt::Horizontal).toString().toHtmlEscaped() + "</th>";
    html += "</tr>";
    for (int r = 0; r < model->rowCount(); ++r) {
        html += QString("<tr style='background:%1'>").arg(r % 2 ? "#E6F4FA" : "#FFFFFF");
        for (int c : colonnes) {
            const QString v = valeur(r, c).toHtmlEscaped();
            html += c == COL_STATUT
                ? "<td style='color:" + couleurStatut(valeur(r, c)).name() + ";font-weight:bold'>" + v + "</td>"
                : "<td>" + v + "</td>";
        }
        html += "</tr>";
    }
    html += "</table>";
    imprimerPdf(html, fichier, QPageLayout::Landscape);
    QMessageBox::information(this, "PDF", "Liste exportée :\n" + QDir::toNativeSeparators(fichier));
}
