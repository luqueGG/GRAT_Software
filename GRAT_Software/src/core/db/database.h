// Database — SQLite connection manager for GRAT_Software (design §8, Req 8).
//
// Owns the QSqlDatabase (QSQLITE) connection at db/grat.db, enables foreign keys
// on every connection, and bootstraps the schema (tables + RBAC seed) from the
// embedded schema resource when the database file is fresh.

#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QString>

class Database : public QObject
{
    Q_OBJECT

public:
    explicit Database(QObject *parent = nullptr);
    ~Database();

    // Opens the SQLite database at <dbDir>/<dbFileName>, creating the file if
    // absent. Enables PRAGMA foreign_keys = ON. If the schema is not present,
    // runs the embedded schema SQL to create tables and seed RBAC data.
    bool open(const QString &dbDir = QStringLiteral("db"),
              const QString &dbFileName = QStringLiteral("grat.db"),
              const QString &schemaResource = QStringLiteral(":/grat_schema.sqlite.sql"));

    void close();

    bool isConnected() const;
    // Probes the live connection with a trivial query; reflects loss of the
    // database file/connection after startup (Req 8.3–8.4).
    bool isAlive() const;
    QString lastError() const;
    QSqlDatabase connection() const;

signals:
    void connectionChanged(bool connected);
    void errorOccurred(const QString &message);

private:
    bool ensureDirectory(const QString &dir) const;
    bool schemaExists() const;
    bool runSchema(const QString &schemaResource);

    // Split a SQL script into individual statements (tracking single-quoted
    // strings so ';' inside literals is not treated as a delimiter).
    static QStringList splitStatements(const QString &sql);
    // Strip '--' comments (full and inline) and whitespace from a statement chunk.
    static QString cleanStatement(const QString &chunk);

    QSqlDatabase m_db;
    QString m_lastError;
    QString m_connectionName;
};
