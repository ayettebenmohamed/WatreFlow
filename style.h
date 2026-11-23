#pragma once
#include <QString>
#include <QMap>
#include <QPair>
#include <QStringList>
#include <algorithm>

// Feuille de style unique de WaterFlow, conforme à la charte graphique du cahier :
//   #0A4D8C bleu profond (titres, menu, en-têtes)   #00B4D8 cyan (actions, accents)
//   #E6F4FA bleu très clair (fonds)                  #2ECC71 / #F39C12 / #E74C3C (vert / orange / rouge)
// Deux thèmes : JOUR (fond clair) et SOMBRE (fond bleu nuit). Les jetons @NOM sont remplacés
// par la couleur du thème ; le menu, les bandeaux et l'écran de connexion gardent le bleu de la charte.
inline const char *const kStyleSheetModele = R"(
/* ---------- Base ---------- */
QWidget { background:@FOND; color:@TEXTE; font-family:"Segoe UI"; font-size:10pt; }
QLabel { background:transparent; }
QWidget[fond="transparent"] { background:transparent; }
QToolTip { background:#0A4D8C; color:white; border:none; padding:6px 8px; border-radius:4px; }
QLabel#persLabelTitre,
QLabel#livLabelTitre,
QLabel#cmdLabelTitre { color:@TITRE; font-size:20pt; font-weight:800; }
/* Module Personnel : densité d'origine (9 pt) pour conserver la mise en page de ses tableaux */
Personnel QLineEdit,
Personnel QComboBox,
Personnel QDateEdit,
Personnel QTableView,
Personnel QTableWidget,
Personnel QPushButton,
Personnel QHeaderView::section { font-size:9pt; }
QLabel#livLabelSousTitre { color:@DOUX; font-size:10pt; }
QLabel#livLabelHorloge { color:@TITRE; font-weight:600; background:@ALT; border-radius:6px; padding:6px 12px; }
QFrame#livHeroLivraison QLabel#persLabelTitre,
QFrame#livHeroLivraison QLabel#livLabelTitre,
QFrame#livHeroLivraison QLabel#cmdLabelTitre { color:white; font-size:22pt; }
QFrame#livHeroLivraison QLabel#livLabelSousTitre { color:#DDF3FA; }
QFrame#livHeroLivraison QLabel#livLabelHorloge { color:white; background:rgba(255,255,255,0.16); border:1px solid rgba(255,255,255,0.35); }

/* ---------- Barre latérale de l'application ---------- */
QFrame#appSidebar { background:qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #0A4D8C, stop:1 #073763); border:none; }
QLabel#appLogo { color:white; font-size:18pt; font-weight:800; padding:6px 4px 0 4px; }
QLabel#appSlogan { color:#9FD8EE; font-size:8pt; letter-spacing:1px; padding:0 4px 14px 4px; }
QCheckBox#toggleAnimation { color:#CFEAF5; font-size:9pt; padding:6px 4px; }
QCheckBox#toggleAnimation::indicator { width:16px; height:16px; }
QPushButton#btnTheme,
QPushButton#btnDeconnexion { background:rgba(255,255,255,0.10); color:#E6F4FA; border:1px solid rgba(255,255,255,0.22); text-align:left; padding:8px 12px; }
QPushButton#btnTheme:hover { background:rgba(255,255,255,0.20); }
QPushButton#btnDeconnexion:hover { background:#E74C3C; border-color:#E74C3C; color:white; }
QLabel#appUtilisateur { color:#CFEAF5; font-size:9pt; background:rgba(255,255,255,0.08); border-radius:6px; padding:8px; }
QListWidget#listWidget { background:transparent; color:#D7ECF6; border:none; font-size:11pt; outline:none; }
QListWidget#listWidget::item { padding:12px 12px; border-radius:6px; margin:3px 0; }
QListWidget#listWidget::item:hover { background:rgba(255,255,255,0.10); color:white; }
QListWidget#listWidget::item:selected { background:#00B4D8; color:white; font-weight:700; }

/* Barre latérale interne de Personnel (masquée dans l'application intégrée) */






/* ---------- Cartes / alertes ---------- */
QFrame#persFrameAlerte,
QFrame#livFrameAlerteRetard { background:@ALERTE_FOND; border:1px solid @ALERTE_BORD; border-left:5px solid #F39C12; border-radius:8px; }
QFrame#livFrameAlerteRetard QLabel,
QFrame#persFrameAlerte QLabel { color:@ALERTE_TEXTE; }
QFrame#persCardListe,
QFrame#persCardFiche,
QFrame#persCardPlanning,
QFrame#persCardGraphique,
QFrame[role="carte"] { background:@CARTE_FOND; border:1px solid @BORD; border-radius:12px; }
QLabel#persLabelTitreListe,
QLabel#persLabelTitreFiche,
QLabel#persLabelTitrePlanning,
QLabel#persLabelTitreGraphique { color:@TITRE; font-weight:700; font-size:10pt; padding-bottom:3px; }

/* Indicateurs (KPI) */
QFrame[role="kpi"] { background:@KPI_FOND; border:1px solid @BORD; border-top:4px solid @TITRE; border-radius:10px; }
QFrame[role="kpi"][accent="cyan"] { border-top-color:#00B4D8; }
QFrame[role="kpi"][accent="orange"] { border-top-color:#F39C12; }
QFrame[role="kpi"][accent="vert"] { border-top-color:#2ECC71; }
QLabel[role="kpiValeur"] { color:@TITRE; font-size:20pt; font-weight:800; }
QLabel[role="kpiTexte"] { color:@DOUX; font-size:9pt; }

/* ---------- Boutons ---------- */
QPushButton { background:#00B4D8; color:white; border:none; border-radius:6px; padding:8px 14px; font-weight:600; }
QPushButton:hover { background:#0A4D8C; }
QPushButton:pressed { background:#073763; }
QPushButton:disabled { background:@INACTIF; color:@INACTIF_TEXTE; }
QPushButton#persBtnAjouter,
QPushButton#livBtnAjouter,
QPushButton#cmdBtnAjouter,
QPushButton[variante="clair"] { background:@ALT; color:@TITRE; border:1px solid @CLAIR_BORD; }
QPushButton#persBtnAjouter:hover,
QPushButton#livBtnAjouter:hover,
QPushButton#cmdBtnAjouter:hover,
QPushButton[variante="clair"]:hover { background:@CLAIR_SURVOL; }
Livraisons QPushButton#persBtnAjouter,
Livraisons QPushButton#livBtnAjouter,
Livraisons QPushButton#cmdBtnAjouter { background:#00B4D8; color:white; border:none; }
Livraisons QPushButton#persBtnAjouter:hover,
Livraisons QPushButton#livBtnAjouter:hover,
Livraisons QPushButton#cmdBtnAjouter:hover { background:#0A4D8C; }
QPushButton#persBtnEnregistrer,
QPushButton#livBtnModifier,
QPushButton#cmdBtnModifier { background:#0A4D8C; }
QPushButton#persBtnEnregistrer:hover,
QPushButton#livBtnModifier:hover,
QPushButton#cmdBtnModifier:hover { background:#073763; }
QPushButton#persBtnSupprimer,
QPushButton#livBtnSupprimer,
QPushButton#cmdBtnSupprimer,
QPushButton[variante="danger"] { background:@DANGER_FOND; color:@DANGER_TEXTE; border:1px solid @DANGER_BORD; }
QPushButton#persBtnSupprimer:hover,
QPushButton#livBtnSupprimer:hover,
QPushButton#cmdBtnSupprimer:hover,
QPushButton[variante="danger"]:hover { background:#E74C3C; color:white; }
QPushButton#persBtnExporter,
QPushButton#cmdBtnExporterCsv,
QPushButton#livBtnPdf,
QPushButton[variante="sombre"] { background:@BTN_SOMBRE; }
QPushButton#persBtnExporter:hover,
QPushButton#cmdBtnExporterCsv:hover,
QPushButton#livBtnPdf:hover,
QPushButton[variante="sombre"]:hover { background:#0A4D8C; }
QPushButton[variante="alerte"] { background:#F39C12; }
QPushButton[variante="alerte"]:hover { background:#D35400; }
QPushButton[variante="alerte"]:disabled { background:@ALERTE_INACTIF; }
QPushButton#livBtnVoirRetards { background:#F39C12; padding:6px 12px; }

/* ---------- Champs de saisie ---------- */
QLineEdit,
QComboBox,
QDateEdit,
QDateTimeEdit,
QPlainTextEdit { background:@SURFACE; color:@TEXTE; border:1px solid @BORD_CHAMP; border-radius:6px; padding:7px 9px; selection-background-color:#00B4D8; }
QLineEdit:focus,
QComboBox:focus,
QDateEdit:focus,
QDateTimeEdit:focus { border:1px solid #00B4D8; }
QComboBox::drop-down { border:none; width:24px; }
QComboBox QAbstractItemView { background:@SURFACE; color:@TEXTE; selection-background-color:@SELECTION; selection-color:@TITRE; border:1px solid @BORD_CHAMP; }
QCheckBox { background:transparent; }
QMessageBox { background:@FOND; }

/* ---------- Tableaux ---------- */
QTableView,
QTableWidget { background:@SURFACE; color:@TEXTE; border:1px solid @BORD; border-radius:10px; alternate-background-color:@ALT;
    selection-background-color:@SELECTION; selection-color:@TITRE; gridline-color:@ALT; outline:none; }
QTableView::item,
QTableWidget::item { padding:0 4px; border:none; }
Livraisons QTableView::item,
Livraisons QTableWidget::item { padding:4px 8px; }
QTableView::item:selected,
QTableWidget::item:selected { color:@TITRE; }
QHeaderView { background:transparent; }
QHeaderView::section { background:@ENTETE; color:white; padding:9px 8px; border:none; border-right:1px solid @ENTETE_BORD; font-weight:700; }
QTableCornerButton::section { background:@ENTETE; border:none; }
QScrollBar:vertical { background:transparent; width:10px; margin:2px; }
QScrollBar::handle:vertical { background:@DEFILEMENT; border-radius:4px; min-height:30px; }
QScrollBar::add-line,
QScrollBar::sub-line { height:0; width:0; }
QScrollBar:horizontal { background:transparent; height:10px; margin:2px; }
QScrollBar::handle:horizontal { background:@DEFILEMENT; border-radius:4px; min-width:30px; }

/* ---------- Onglets ---------- */
QTabWidget::pane { border:none; }
QTabBar::tab { background:transparent; color:@DOUX; padding:9px 18px; margin-right:4px; border-bottom:3px solid transparent; font-weight:600; }
QTabBar::tab:hover { color:@TITRE; }
QTabBar::tab:selected { color:@TITRE; border-bottom:3px solid #00B4D8; }

/* ---------- Groupes ---------- */
QGroupBox { background:@GROUPE_FOND; border:1px solid @BORD; border-radius:12px; margin-top:16px; padding:18px 12px 12px 12px; font-weight:700; color:@TITRE; }
QGroupBox::title { subcontrol-origin:margin; left:14px; padding:2px 8px; background:#0A4D8C; color:white; border-radius:4px; }
QGroupBox QLabel { color:@ETIQUETTE; font-weight:500; }
QLabel#livLabelEstimation { color:@ESTIMATION; font-weight:600; }
QLabel#livLabelAide { color:@DOUX; font-size:8pt; }

/* ---------- Connexion ---------- */
QDialog#loginDialog { background:@SURFACE; }
QFrame#loginBrandPanel { background:qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #0A4D8C, stop:1 #00B4D8); border:none; }
QLabel#brandWord { color:white; font-size:26pt; font-weight:800; }
QLabel#brandTag { color:#DDF3FA; font-size:9pt; letter-spacing:1px; }
QLabel#loginTitle { color:@TITRE; font-size:23pt; font-weight:800; }
QLabel#loginSub { color:@DOUX; font-size:10pt; }
QLabel#loginFieldLabel { color:@ETIQUETTE; font-weight:650; }
QLabel#loginMark { color:white; background:rgba(255,255,255,0.18); border-radius:29px; font-size:22pt; font-weight:800; }
QLineEdit#loginUser,
QLineEdit#loginPass { padding:11px; font-size:10pt; background:@FOND; border:1px solid @BORD_CHAMP; border-radius:6px; }
QPushButton#loginButton { padding:12px; font-size:10pt; border-radius:6px; }
QLabel#loginHint { color:@DOUX; font-size:8pt; }

/* ---------- Tableau de bord ---------- */
QFrame#dashHero QLabel#dashLabelTitre { color:white; font-size:22pt; font-weight:800; }
QFrame#dashHero QLabel#dashLabelSousTitre { color:#DDF3FA; font-size:10pt; }
QFrame#dashHero QLabel#dashLabelHorloge { color:white; font-weight:600; background:rgba(255,255,255,0.16); border:1px solid rgba(255,255,255,0.35); border-radius:6px; padding:6px 12px; }
QFrame[role="module"] { background:@KPI_FOND; border:1px solid @BORD; border-top:4px solid @TITRE; border-radius:12px; }
QFrame[role="module"][accent="cyan"] { border-top-color:#00B4D8; }
QFrame[role="module"][accent="vert"] { border-top-color:#2ECC71; }
QLabel[role="dashTitreModule"] { color:@TITRE; font-size:14pt; font-weight:800; }
QLabel[role="dashValeur"] { color:@TITRE; font-size:20pt; font-weight:800; }
QLabel[role="dashTexte"] { color:@DOUX; font-size:9pt; }
QListWidget#dashListeAlertes { background:transparent; border:none; font-size:10pt; outline:none; }
QListWidget#dashListeAlertes::item { border-bottom:1px solid @BORD; padding:4px 6px; }
)";

inline QString feuilleDeStyle(bool sombre)
{
    const QMap<QString, QPair<QString, QString>> jetons = {   // jeton -> (jour, sombre)
        {"FOND",           {"#F2F8FB", "#0B1724"}},
        {"SURFACE",        {"#FFFFFF", "#13263A"}},
        {"ALT",            {"#E6F4FA", "#182F47"}},
        {"TEXTE",          {"#14324A", "#E3F1F8"}},
        {"DOUX",           {"#5E7A8F", "#8FB0C6"}},
        {"ETIQUETTE",      {"#36566D", "#B5CEDD"}},
        {"TITRE",          {"#0A4D8C", "#5CC8EE"}},
        {"BORD",           {"#D6E9F2", "#24405A"}},
        {"BORD_CHAMP",     {"#CFE3EE", "#2C4C68"}},
        {"SELECTION",      {"#CDEFF8", "#1D5A80"}},
        {"ENTETE",         {"#0A4D8C", "#0A3B66"}},
        {"ENTETE_BORD",    {"#0E5C9E", "#0E4C80"}},
        {"KPI_FOND",       {"rgba(255,255,255,0.86)", "rgba(19,38,58,0.88)"}},
        {"GROUPE_FOND",    {"rgba(255,255,255,0.93)", "rgba(19,38,58,0.94)"}},
        {"CARTE_FOND",     {"rgba(255,255,255,0.55)", "rgba(19,38,58,0.62)"}},
        {"ALERTE_FOND",    {"#FFF6E8", "#3A2A12"}},
        {"ALERTE_BORD",    {"#F8DDB0", "#6B4A16"}},
        {"ALERTE_TEXTE",   {"#8A5300", "#F8C77A"}},
        {"ALERTE_INACTIF", {"#F3E3C9", "#4A3A22"}},
        {"DANGER_FOND",    {"#FDEDEC", "#3B1E24"}},
        {"DANGER_BORD",    {"#F5C6C1", "#6B2C33"}},
        {"DANGER_TEXTE",   {"#C0392B", "#FF8A80"}},
        {"CLAIR_BORD",     {"#BFE3F1", "#2C4C68"}},
        {"CLAIR_SURVOL",   {"#D3EDF8", "#21405E"}},
        {"BTN_SOMBRE",     {"#14324A", "#1E4A6E"}},
        {"INACTIF",        {"#C9DCE5", "#22384D"}},
        {"INACTIF_TEXTE",  {"#F4F9FB", "#6C879B"}},
        {"DEFILEMENT",     {"#B9D9E8", "#2E5170"}},
        {"ESTIMATION",     {"#00838F", "#4FD6EE"}},
    };
    QString css = QString::fromUtf8(kStyleSheetModele);
    QStringList noms = jetons.keys();
    // les noms longs d'abord : @ALERTE_FOND avant @ALT, @BORD_CHAMP avant @BORD...
    std::sort(noms.begin(), noms.end(), [](const QString &a, const QString &b) { return a.size() > b.size(); });
    for (const QString &n : noms)
        css.replace("@" + n, sombre ? jetons[n].second : jetons[n].first);
    return css;
}
