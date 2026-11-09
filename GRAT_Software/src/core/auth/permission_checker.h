// permission_checker.h — RBAC checks against an AuthSession (design §9).
//
// Permission codenames follow <action>_<model> (e.g. view_waste). The action map
// keeps UI call sites readable while preserving the seeded codename format.

#pragma once

#include <QString>

#include "app/session.h"

enum class PermissionAction {
    View,       // view_<model>
    Add,        // add_<model>
    Change,     // change_<model> (modify, update and state transitions)
    AssignRole, // assign_role_user
};

class PermissionChecker
{
public:
    static bool can(const AuthSession &session, PermissionAction action,
                    const QString &appLabel, const QString &model);

    // Resolve an action to its auth_permission.codename. appLabel is part of the
    // permission identity in the schema; codenames are globally unique here.
    static QString codename(PermissionAction action, const QString &appLabel,
                            const QString &model);
};
