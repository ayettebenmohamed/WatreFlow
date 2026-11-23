#include "mainwindow.h"
#include "logindialog.h"
#include "connection.h"
#include "session.h"
#include "theme.h"
#include <QApplication>
#include <QDebug>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 1. Appliquer le style : thème jour / sombre mémorisé
    a.setStyle("Fusion");
    Theme::appliquer(Theme::sombre());

    // 2. Connexion à la base de données
    Connection& dbConn = Connection::createInstance();
    if (!dbConn.createConnection()) {
        QMessageBox::critical(nullptr, "Base de données",
            "Connexion à la base Oracle impossible :\n" + dbConn.lastError()
            + "\n\nVérifiez qu'Oracle XE est démarré (service OracleServiceXE)\n"
              "et que le schéma WATERFLOW a été créé (scripts du dossier sql/).");
        return 1;
    }

    // 3. Boucle de session : connexion -> fenêtre principale -> (déconnexion -> connexion ...)
    for (;;) {
        LoginDialog login;
        if (login.exec() != QDialog::Accepted)
            return 0;                                   // fenêtre de connexion fermée : on quitte

        int code = 0;
        {
            MainWindow w;                               // recréée à chaque session (utilisateur à jour)
            w.show();
            code = a.exec();
        }                                               // fenêtre détruite ici
        if (code != MainWindow::CODE_DECONNEXION)
            return code;                                // fermeture normale de l'application
        Session::fermer();
    }
}
