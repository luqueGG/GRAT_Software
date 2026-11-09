#include "database.h"

#include <QDir>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>

Database::Database(QObject *parent)
    : QObject(parent)
    , m_connectionName(QStringLiteral("grat_connection"))
{
}

Database::~Database()
{
    close();
}

bool Database::open(const QString &dbDir, const QString &dbFileName, const QString &schemaResource)
{
    // Reuse a single named connection; drop any stale one first.
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::removeDatabase(m_connectionName);
    }

    if (!ensureDirectory(dbDir)) {
        m_lastError = tr("Could not create the database directory: %1").arg(dbDir);
        emit errorOccurred(m_lastError);
        emit connectionChanged(false);
        return false;
    }

    const QString dbPath = QDir(dbDir).filePath(dbFileName);

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        m_lastError = tr("Could not open the database: %1").arg(m_db.lastError().text());
        emit errorOccurred(m_lastError);
        emit connectionChanged(false);
        return false;
    }

    // SQLite disables FK enforcement by default — enable it per connection.
    QSqlQuery pragma(m_db);
    if (!pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"))) {
        m_lastError = tr("Could not enable foreign keys: %1").arg(pragma.lastError().text());
        emit errorOccurred(m_lastError);
        emit connectionChanged(false);
        return false;
    }

    // Bootstrap the schema on a fresh database.
    if (!schemaExists()) {
        if (!runSchema(schemaResource)) {
            emit errorOccurred(m_lastError);
            emit connectionChanged(false);
            return false;
        }
    }

    m_lastError.clear();
    emit connectionChanged(true);
    return true;
}

void Database::close()
{
    if (m_db.isOpen())
        m_db.close();
    m_db = QSqlDatabase();
    if (QSqlDatabase::contains(m_connectionName))
        QSqlDatabase::removeDatabase(m_connectionName);
}

bool Database::isConnected() const
{
    return m_db.isOpen();
}

bool Database::isAlive() const
{
    if (!m_db.isOpen())
        return false;

    QSqlQuery probe(m_db);
    return probe.exec(QStringLiteral("SELECT 1")) && probe.next();
}

QString Database::lastError() const
{
    return m_lastError;
}

QSqlDatabase Database::connection() const
{
    return QSqlDatabase::database(m_connectionName, false);
}

bool Database::ensureDirectory(const QString &dir) const
{
    QDir d(dir);
    if (d.exists())
        return true;
    return d.mkpath(QStringLiteral("."));
}

bool Database::schemaExists() const
{
    QSqlQuery q(m_db);
    q.exec(QStringLiteral(
        "SELECT name FROM sqlite_master WHERE type='table' AND name='location_country';"));
    return q.next();
}

bool Database::runSchema(const QString &schemaResource)
{
    QFile file(schemaResource);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = tr("Could not read the schema resource: %1").arg(schemaResource);
        return false;
    }
    const QString sql = QString::fromUtf8(file.readAll());
    file.close();

    const QStringList statements = splitStatements(sql);

    // Wrap schema creation in a single transaction so all statements commit in
    // one fsync (fast) and the schema is created atomically; rollback on any error.
    const bool inTx = m_db.transaction();
    QSqlQuery q(m_db);
    for (const QString &raw : statements) {
        const QString stmt = cleanStatement(raw);
        if (stmt.isEmpty())
            continue;
        if (!q.exec(stmt)) {
            m_lastError = tr("Schema statement failed: %1").arg(q.lastError().text());
            if (inTx)
                m_db.rollback();
            return false;
        }
    }
    if (inTx && !m_db.commit()) {
        m_lastError = tr("Could not commit the schema initialization.");
        return false;
    }
    return true;
}

QStringList Database::splitStatements(const QString &sql)
{
    QStringList statements;
    QString current;
    bool inQuote = false;

    for (int i = 0; i < sql.size(); ++i) {
        const QChar c = sql.at(i);
        if (c == QLatin1Char('\'')) {
            inQuote = !inQuote;
            current += c;
        } else if (c == QLatin1Char(';') && !inQuote) {
            statements << current;
            current.clear();
        } else {
            current += c;
        }
    }
    // Trailing content without a final ';'.
    if (!current.trimmed().isEmpty())
        statements << current;
    return statements;
}

QString Database::cleanStatement(const QString &chunk)
{
    QStringList kept;
    const QStringList lines = chunk.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        QString l = line;
        const int commentIdx = l.indexOf(QStringLiteral("--"));
        if (commentIdx >= 0)
            l = l.left(commentIdx); // strip inline comment
        if (!l.trimmed().isEmpty())
            kept << l;
    }
    return kept.join(QLatin1Char('\n')).trimmed();
}
