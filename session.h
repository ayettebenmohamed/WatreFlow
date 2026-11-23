#pragma once
#include <QString>

// Utilisateur connecté (renseigné par LoginDialog).
// Sert à la traçabilité : chaque action importante est enregistrée avec son auteur.
class Session
{
public:
    static QString utilisateur() { return instance().m_utilisateur.isEmpty() ? QStringLiteral("système")
                                                                             : instance().m_utilisateur; }
    static void ouvrir(const QString &login) { instance().m_utilisateur = login; }
    static void fermer() { instance().m_utilisateur.clear(); }

private:
    static Session &instance() { static Session s; return s; }
    QString m_utilisateur;
};
