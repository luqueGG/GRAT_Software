// session.h — authenticated operator session (design §7.1).
//
// Carries the identity and effective RBAC data loaded at sign-in. The session is
// passed to the main window and to every permission-gated screen.

#pragma once

#include <QSet>
#include <QString>
#include <QStringList>

struct AuthSession
{
    qint64 userId = -1;          // user_user.id
    QString username;            // user_user.username
    QStringList roles;           // auth_group names assigned to the user
    QSet<QString> permissions;   // effective codenames (group ∪ user permissions)
    bool isSuperuser = false;    // user_user.is_superuser

    bool isValid() const { return userId >= 0; }
};
