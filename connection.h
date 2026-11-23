#ifndef CONNECTION_H
#define CONNECTION_H

#include <QSqlDatabase>
#include <QString>
class Connection {
public:
    static Connection& createInstance();

    bool createConnection();
    QSqlDatabase getDatabase() const { return QSqlDatabase::database(); }
    QString lastError() const { return m_erreur; }

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

private:
    Connection() = default;
    QString m_erreur;
};

#endif
