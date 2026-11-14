#include "database.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>

#include "core/errors/error_handler.h"

namespace {

QString connectionFailureMessage()
{
    return QStringLiteral("The database could not be opened. "
                          "GRAT_Software cannot continue without its database.");
}

QString recoveryFailureMessage()
{
    return QStringLiteral("The database file is not available. "
                          "Maintenance remains disabled until the original database "
                          "file is restored.");
}

QString invalidDatabaseMessage()
{
    return QStringLiteral("The database file is not a valid GRAT_Software database. "
                          "Maintenance remains disabled to protect the original data.");
}

} // namespace

Database::Database(QObject *parent)
    : QObject(parent)
    , m_connectionName(QStringLiteral("grat_connection"))
{
}

Database::~Database()
{
    close();
    m_db = QSqlDatabase();
    if (QSqlDatabase::contains(m_connectionName))
        QSqlDatabase::removeDatabase(m_connectionName);
}

bool Database::open(const QString &dbDir, const QString &dbFileName,
                    const QString &schemaResource)
{
    m_dbDir = dbDir;
    m_dbFileName = dbFileName;
    m_dbPath = QDir(dbDir).filePath(dbFileName);
    m_schemaResource = schemaResource;
    return openConnection(true);
}

void Database::close()
{
    if (m_db.isOpen())
        m_db.close();
    setConnectionState(false);
}

bool Database::isConnected() const
{
    return m_db.isOpen();
}

bool Database::isAlive() const
{
    if (!m_db.isOpen())
        return false;

    // SQLite keeps an open handle to a file even if the directory entry changes.
    // Verify that the real file is still present, writable and belongs to this app.
    if (!m_dbPath.isEmpty()) {
        const QFileInfo fileInfo(m_dbPath);
        if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isWritable())
            return false;
    }

    QSqlQuery probe(m_db);
    if (!probe.exec(QStringLiteral("SELECT 1")) || !probe.next())
        return false;
    probe.finish();

    // A database replaced by an empty or unrelated SQLite file must not be used.
    if (!probe.exec(QStringLiteral(
            "SELECT name FROM sqlite_master WHERE type='table' AND name='location_country'"))
        || !probe.next()) {
        return false;
    }
    probe.finish();

    // Foreign-key enforcement is enabled per connection; a reopened handle must
    // confirm that it is still active before maintenance is re-enabled.
    if (!probe.exec(QStringLiteral("PRAGMA foreign_keys")) || !probe.next()
        || probe.value(0).toInt() != 1) {
        return false;
    }

    return true;
}

QString Database::lastError() const
{
    return m_lastError;
}

QSqlDatabase Database::connection() const
{
    return QSqlDatabase::database(m_connectionName, false);
}

bool Database::reconnect()
{
    if (m_dbPath.isEmpty()) {
        publishFailure(recoveryFailureMessage());
        return false;
    }

    // Recovery must never create a database. If the original file is gone, the
    // only safe outcome is to keep maintenance disabled until it is restored.
    const QFileInfo fileInfo(m_dbPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        if (ErrorHandler::shouldAnnounce(QStringLiteral("database-recovery"), 30000)) {
            ErrorHandler::failure(ErrorHandler::Category::Connection,
                                  QStringLiteral("database recovery"),
                                  QStringLiteral("Database file is missing: %1").arg(m_dbPath),
                                  recoveryFailureMessage());
        }
        publishFailure(recoveryFailureMessage());
        return false;
    }

    // Reuse the registered named connection; removing it would invalidate the
    // QSqlDatabase handles held by TableGateway instances.
    if (m_db.isOpen())
        m_db.close();

    return openConnection(false);
}

bool Database::ensureAvailable()
{
    if (isAlive()) {
        if (!m_connected) {
            m_lastError.clear();
            m_lastReportedError.clear();
            setConnectionState(true);
        }
        return true;
    }

    // Mark the loss before attempting recovery so observers receive the
    // connected -> unavailable transition.
    setConnectionState(false);

    if (!reconnect())
        return false;

    if (!isAlive()) {
        publishFailure(recoveryFailureMessage());
        return false;
    }
    return true;
}

bool Database::openConnection(bool allowCreate)
{
    if (allowCreate && !ensureDirectory(m_dbDir)) {
        if (ErrorHandler::shouldAnnounce(QStringLiteral("database-open"), 30000)) {
            ErrorHandler::failure(ErrorHandler::Category::Connection,
                                  QStringLiteral("database directory"),
                                  QStringLiteral("Could not create database directory: %1")
                                      .arg(m_dbDir),
                                  connectionFailureMessage());
        }
        publishFailure(connectionFailureMessage());
        return false;
    }

    // Reuse the registered connection when possible. addDatabase()/removeDatabase()
    // would invalidate active QSqlDatabase references held by cached screens.
    if (QSqlDatabase::contains(m_connectionName)) {
        m_db = QSqlDatabase::database(m_connectionName, false);
    } else {
        m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    }

    if (!m_db.isValid()) {
        publishFailure(connectionFailureMessage());
        return false;
    }

    // Reuse the live connection when possible; close only the handle so active
    // QSqlDatabase copies (TableGateway) remain valid.
    if (m_db.isOpen())
        m_db.close();

    if (allowCreate) {
        m_db.setDatabaseName(m_dbPath);
        m_db.setConnectOptions();
    } else {
        // SQLite URI mode=rw opens an existing file read-write and fails when the
        // file is absent, unlike the default mode=rwc which would create an empty
        // database. This is the strongest guarantee that recovery never replaces
        // the original database with a fresh one.
        QUrl uri = QUrl::fromLocalFile(QFileInfo(m_dbPath).absoluteFilePath());
        uri.setQuery(QStringLiteral("mode=rw"));
        m_db.setDatabaseName(uri.toString(QUrl::FullyEncoded));
        m_db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_URI"));
    }

    if (!m_db.isOpen() && !m_db.open()) {
        const ErrorHandler::Failure failure = ErrorHandler::sqlFailure(
            m_db.lastError(), ErrorHandler::Category::Connection,
            QStringLiteral("open database"), connectionFailureMessage());
        publishFailure(failure.message);
        m_db = QSqlDatabase();
        return false;
    }

    // A recovery must land on the original schema, not on a fresh empty file.
    if (!allowCreate && !schemaExists()) {
        m_db.close();
        if (ErrorHandler::shouldAnnounce(QStringLiteral("database-recovery-schema"), 30000)) {
            ErrorHandler::failure(ErrorHandler::Category::Connection,
                                  QStringLiteral("verify database schema"),
                                  QStringLiteral("GRAT schema is missing from %1").arg(m_dbPath),
                                  invalidDatabaseMessage());
        }
        publishFailure(invalidDatabaseMessage());
        return false;
    }

    if (!enableForeignKeys()) {
        publishFailure(connectionFailureMessage());
        m_db.close();
        return false;
    }

    // Bootstrap the schema only during the initial open of a fresh database.
    if (allowCreate && !schemaExists()) {
        if (!runSchema(m_schemaResource)) {
            publishFailure(m_lastError.isEmpty() ? connectionFailureMessage() : m_lastError);
            m_db.close();
            return false;
        }
    }

    if (!schemaExists()) {
        if (ErrorHandler::shouldAnnounce(QStringLiteral("database-schema"), 30000)) {
            ErrorHandler::failure(ErrorHandler::Category::Connection,
                                  QStringLiteral("verify database schema"),
                                  QStringLiteral("GRAT schema is missing from %1").arg(m_dbPath),
                                  invalidDatabaseMessage());
        }
        publishFailure(invalidDatabaseMessage());
        m_db.close();
        return false;
    }

    m_lastError.clear();
    m_lastReportedError.clear();
    ErrorHandler::clearAnnouncement(QStringLiteral("database-recovery"));
    ErrorHandler::clearAnnouncement(QStringLiteral("database-recovery-schema"));
    ErrorHandler::clearAnnouncement(QStringLiteral("database-open"));
    ErrorHandler::clearAnnouncement(QStringLiteral("database-schema"));
    setConnectionState(true);
    return true;
}

bool Database::enableForeignKeys()
{
    QSqlQuery pragma(m_db);
    if (pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON")))
        return true;

    ErrorHandler::sqlFailure(pragma.lastError(), ErrorHandler::Category::Connection,
                             QStringLiteral("enable foreign keys"),
                             connectionFailureMessage());
    return false;
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
    if (!q.exec(QStringLiteral(
            "SELECT name FROM sqlite_master WHERE type='table' AND name='location_country';"))) {
        return false;
    }
    return q.next();
}

bool Database::runSchema(const QString &schemaResource)
{
    QFile file(schemaResource);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ErrorHandler::failure(ErrorHandler::Category::Connection,
                              QStringLiteral("read schema resource"),
                              QStringLiteral("Could not read schema resource: %1")
                                  .arg(schemaResource),
                              connectionFailureMessage());
        m_lastError = connectionFailureMessage();
        return false;
    }
    const QString sql = QString::fromUtf8(file.readAll());
    file.close();

    const QStringList statements = splitStatements(sql);

    // Wrap schema creation in a single transaction so all statements commit in
    // one fsync (fast) and the schema is created atomically; rollback on any error.
    if (!m_db.transaction()) {
        const ErrorHandler::Failure failure = ErrorHandler::sqlFailure(
            m_db.lastError(), ErrorHandler::Category::Connection,
            QStringLiteral("start schema transaction"), connectionFailureMessage());
        m_lastError = failure.message;
        return false;
    }

    QSqlQuery q(m_db);
    for (const QString &raw : statements) {
        const QString stmt = cleanStatement(raw);
        if (stmt.isEmpty())
            continue;
        if (!q.exec(stmt)) {
            const ErrorHandler::Failure failure = ErrorHandler::sqlFailure(
                q.lastError(), ErrorHandler::Category::Connection,
                QStringLiteral("schema statement"), connectionFailureMessage());
            m_lastError = failure.message;
            m_db.rollback();
            return false;
        }
    }

    if (!m_db.commit()) {
        const ErrorHandler::Failure failure = ErrorHandler::sqlFailure(
            m_db.lastError(), ErrorHandler::Category::Connection,
            QStringLiteral("commit schema transaction"), connectionFailureMessage());
        m_lastError = failure.message;
        m_db.rollback();
        return false;
    }
    return true;
}

void Database::publishFailure(const QString &userMessage)
{
    m_lastError = userMessage;
    if (m_lastError != m_lastReportedError) {
        m_lastReportedError = m_lastError;
        emit errorOccurred(m_lastError);
    }
    setConnectionState(false);
}

void Database::setConnectionState(bool connected)
{
    if (m_connected == connected)
        return;
    m_connected = connected;
    emit connectionChanged(m_connected);
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
