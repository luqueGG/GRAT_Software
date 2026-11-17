// Database — SQLite connection manager for GRAT_Software (design §8, Req 8).
//
// Owns the single named QSqlDatabase (QSQLITE) connection at db/grat.db, enables
// foreign keys on every connection, and bootstraps the schema (tables + RBAC seed)
// from the embedded schema resource when the database file is fresh.
//
// Connection recovery reuses the same named connection (so QSqlDatabase handles
// held by TableGateway remain valid) and never creates a missing database during a
// recovery attempt: only the initial open may create db/grat.db.

#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QString>

class Database : public QObject
{
    Q_OBJECT

public:
    explicit Database(QObject *parent = nullptr);
    ~Database() override;

    // Opens the SQLite database at <dbDir>/<dbFileName>. The initial open may
    // create the file and run the embedded schema; PRAGMA foreign_keys = ON is
    // enabled on every connection.
    bool open(const QString &dbDir = QStringLiteral("db"),
              const QString &dbFileName = QStringLiteral("grat.db"),
              const QString &schemaResource = QStringLiteral(":/grat_schema.sqlite.sql"));

    // Closes the live handle but keeps the named connection registered so active
    // QSqlDatabase copies are not invalidated.
    void close();

    bool isConnected() const;
    // Probes the live connection with read-only queries and verifies that the
    // database file and the original schema are still available (Req 8.3–8.4).
    bool isAlive() const;
    QString lastError() const;
    QSqlDatabase connection() const;

    // Safe recovery: reopens the existing file in read-write mode, re-enables
    // foreign keys and verifies the original schema. A missing file or a file
    // without the GRAT schema is reported as unavailable and is never replaced by
    // a newly-created empty database.
    bool reconnect();
    // Returns true when the database is usable; attempts a safe recovery when not.
    bool ensureAvailable();

signals:
    void connectionChanged(bool connected);
    void errorOccurred(const QString &message);

private:
    bool openConnection(bool allowCreate);
    bool enableForeignKeys();
    bool ensureDirectory(const QString &dir) const;
    bool schemaExists() const;
    bool runSchema(const QString &schemaResource);
    void publishFailure(const QString &userMessage);
    void setConnectionState(bool connected);

    // Split a SQL script into individual statements (tracking single-quoted
    // strings so ';' inside literals is not treated as a delimiter).
    static QStringList splitStatements(const QString &sql);
    // Strip '--' comments (full and inline) and whitespace from a statement chunk.
    static QString cleanStatement(const QString &chunk);

    QSqlDatabase m_db;
    QString m_lastError;
    QString m_lastReportedError;
    QString m_connectionName;
    QString m_dbDir;
    QString m_dbFileName;
    QString m_dbPath;
    QString m_schemaResource;
    bool m_connected = false;
};
