// auth_service.h — authentication and RBAC session loading (design §7.1, §9).
//
// Verifies operator credentials against user_user (SHA-256 + per-record salt) and
// builds the AuthSession consumed by PermissionChecker and the UI. Error messages
// are user-facing and never expose SQL or schema internals (Req 9.1–9.2).

#pragma once

#include <QSqlDatabase>
#include <QString>
#include <optional>

#include "app/session.h"

class AuthService
{
public:
    explicit AuthService(QSqlDatabase db);

    // Look up an active user by username and verify the password. On success the
    // session is populated and last_login is updated. On failure returns nullopt
    // and, when provided, an informative non-leaking message (Req 9.1).
    std::optional<AuthSession> login(const QString &username, const QString &password,
                                     QString *errorMessage = nullptr) const;

    // Create the default demonstration accounts on a database that has no users.
    // Existing users are never modified; this only bootstraps an empty user table.
    bool ensureDefaultUsers(QString *errorMessage = nullptr) const;

    // Password storage helpers: "salt$sha256hex" (design §13.3).
    static QString hashPassword(const QString &password);
    static bool verifyPassword(const QString &password, const QString &storedHash);
    static QString generateSalt();

private:
    void loadRoles(AuthSession &session) const;
    void loadPermissions(AuthSession &session) const;

    // QSqlDatabase handles are shallow; mutable lets const helpers run transactions.
    mutable QSqlDatabase m_db;
};
