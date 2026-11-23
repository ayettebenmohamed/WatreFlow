#include "connection.h"
#include <QSqlError>
#include <QSettings>
#include <QCoreApplication>
#include <QDebug>

Connection& Connection::createInstance() {
    static Connection instance;
    return instance;
}

bool Connection::createConnection() {
    m_erreur.clear();

    QSqlDatabase db = QSqlDatabase::contains(QSqlDatabase::defaultConnection)
                          ? QSqlDatabase::database()
                          : QSqlDatabase::addDatabase("QODBC");
    if (db.isOpen())
        return true;

    // Paramètres surchargeables sans recompiler : fichier waterflow.ini à côté de l'exécutable
    //   [database]
    //   dsn=DRIVER={Oracle in XE};DBQ=localhost:1521/XE;
    //   user=wf
    //   password=...
    QSettings ini(QCoreApplication::applicationDirPath() + "/waterflow.ini", QSettings::IniFormat);
    db.setDatabaseName(ini.value("database/dsn", "DRIVER={Oracle in XE};DBQ=localhost:1521/XE;").toString());
    db.setUserName(ini.value("database/user", "wf").toString());
    db.setPassword(ini.value("database/password", "azerty").toString());

    if (!db.open()) {
        m_erreur = db.lastError().text();
        qDebug() << "Échec de la connexion :" << m_erreur;
        return false;
    }
    qDebug() << "Connexion Oracle établie, schéma" << db.userName();
    return true;
}
