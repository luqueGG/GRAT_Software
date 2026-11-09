#include "auth_service.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QSqlQuery>
#include <QUuid>

namespace {

void setError(QString *errorMessage, const QString &message)
{
    if (errorMessage)
        *errorMessage = message;
}

QString invalidCredentials()
{
    return QStringLiteral("Invalid username or password.");
}

QString databaseUnavailable()
{
    return QStringLiteral("The database is not available.");
}

} // namespace

AuthService::AuthService(QSqlDatabase db)
    : m_db(db)
{
}

std::optional<AuthSession> AuthService::login(const QString &username, const QString &password,
                                              QString *errorMessage) const
{
    setError(errorMessage, QString());

    if (!m_db.isOpen()) {
        setError(errorMessage, databaseUnavailable());
        return std::nullopt;
    }

    const QString trimmedUsername = username.trimmed();
    if (trimmedUsername.isEmpty() || password.isEmpty()) {
        setError(errorMessage, invalidCredentials());
        return std::nullopt;
    }

    // Only active users may sign in (design §7.1 step 1).
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT id, username, password, is_superuser "
        "FROM user_user WHERE username = ? AND is_active = 'A' LIMIT 1"));
    query.addBindValue(trimmedUsername);
    if (!query.exec()) {
        setError(errorMessage, databaseUnavailable());
        return std::nullopt;
    }
    if (!query.next()) {
        // Unknown user and inactive user share the same message so the dialog does
        // not disclose which usernames exist.
        setError(errorMessage, invalidCredentials());
        return std::nullopt;
    }

    const qint64 userId = query.value(0).toLongLong();
    const QString storedUsername = query.value(1).toString();
    const QString storedHash = query.value(2).toString();
    const bool isSuperuser = query.value(3).toInt() != 0;

    if (!verifyPassword(password, storedHash)) {
        setError(errorMessage, invalidCredentials());
        return std::nullopt;
    }

    AuthSession session;
    session.userId = userId;
    session.username = storedUsername;
    session.isSuperuser = isSuperuser;

    // Record the successful sign-in (design §7.1 step 5). This is best-effort: a
    // failed audit update must not invalidate valid credentials.
    QSqlQuery update(m_db);
    update.prepare(QStringLiteral("UPDATE user_user SET last_login = ? WHERE id = ?"));
    update.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    update.addBindValue(userId);
    update.exec();

    return session;
}

bool AuthService::ensureDefaultUsers(QString *errorMessage) const
{
    setError(errorMessage, QString());

    if (!m_db.isOpen()) {
        setError(errorMessage, databaseUnavailable());
        return false;
    }

    QSqlQuery count(m_db);
    if (!count.exec(QStringLiteral("SELECT COUNT(*) FROM user_user")) || !count.next()) {
        setError(errorMessage, QStringLiteral("The default accounts could not be verified."));
        return false;
    }
    if (count.value(0).toLongLong() > 0)
        return true;

    // Demonstration accounts so a fresh installation is immediately usable. The
    // application itself never hardcodes role names; these users are ordinary rows.
    struct SeedUser
    {
        const char *username;
        const char *password;
        const char *firstName;
        const char *lastName;
        const char *email;
        bool isSuperuser;
        bool isStaff;
        const char *role;
    };
    static const SeedUser seeds[] = {
        {"admin", "Admin123!", "System", "Administrator", "admin@grat.local", true, true, "Administrator"},
        {"supervisor", "Supervisor123!", "System", "Supervisor", "supervisor@grat.local", false, true, "Supervisor"},
        {"operator", "Operator123!", "System", "Operator", "operator@grat.local", false, false, "Operator"},
    };

    const bool useTransaction = m_db.transaction();

    for (const SeedUser &seed : seeds) {
        QSqlQuery insert(m_db);
        insert.prepare(QStringLiteral(
            "INSERT INTO user_user "
            "(password, is_superuser, username, first_name, last_name, email, is_active, is_staff) "
            "VALUES (?, ?, ?, ?, ?, ?, 'A', ?)"));
        insert.addBindValue(hashPassword(QString::fromUtf8(seed.password)));
        insert.addBindValue(seed.isSuperuser ? 1 : 0);
        insert.addBindValue(QString::fromUtf8(seed.username));
        insert.addBindValue(QString::fromUtf8(seed.firstName));
        insert.addBindValue(QString::fromUtf8(seed.lastName));
        insert.addBindValue(QString::fromUtf8(seed.email));
        insert.addBindValue(seed.isStaff ? 1 : 0);
        if (!insert.exec()) {
            if (useTransaction)
                m_db.rollback();
            setError(errorMessage, QStringLiteral("The default accounts could not be created."));
            return false;
        }

        const qint64 userId = insert.lastInsertId().toLongLong();
        if (userId <= 0) {
            if (useTransaction)
                m_db.rollback();
            setError(errorMessage, QStringLiteral("The default accounts could not be created."));
            return false;
        }

        QSqlQuery assignRole(m_db);
        assignRole.prepare(QStringLiteral(
            "INSERT INTO user_user_groups (user_id, group_id) "
            "SELECT ?, id FROM auth_group WHERE name = ?"));
        assignRole.addBindValue(userId);
        assignRole.addBindValue(QString::fromUtf8(seed.role));
        if (!assignRole.exec()) {
            if (useTransaction)
                m_db.rollback();
            setError(errorMessage, QStringLiteral("The default accounts could not be created."));
            return false;
        }
    }

    if (useTransaction && !m_db.commit()) {
        setError(errorMessage, QStringLiteral("The default accounts could not be created."));
        return false;
    }
    return true;
}

QString AuthService::hashPassword(const QString &password)
{
    const QString salt = generateSalt();
    const QByteArray digest = QCryptographicHash::hash(
        (salt + password).toUtf8(), QCryptographicHash::Sha256);
    return salt + QLatin1Char('$') + QString::fromLatin1(digest.toHex());
}

bool AuthService::verifyPassword(const QString &password, const QString &storedHash)
{
    const int separator = storedHash.indexOf(QLatin1Char('$'));
    if (separator <= 0 || separator >= storedHash.size() - 1)
        return false;

    const QString salt = storedHash.left(separator);
    const QString expected = storedHash.mid(separator + 1);
    const QByteArray digest = QCryptographicHash::hash(
        (salt + password).toUtf8(), QCryptographicHash::Sha256);
    const QString actual = QString::fromLatin1(digest.toHex());
    return actual.compare(expected, Qt::CaseInsensitive) == 0;
}

QString AuthService::generateSalt()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces).remove(QLatin1Char('-'));
}
