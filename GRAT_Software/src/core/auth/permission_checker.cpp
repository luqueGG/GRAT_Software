#include "permission_checker.h"

QString PermissionChecker::codename(PermissionAction action, const QString &appLabel,
                                    const QString &model)
{
    Q_UNUSED(appLabel)

    switch (action) {
    case PermissionAction::View:
        return QStringLiteral("view_") + model;
    case PermissionAction::Add:
        return QStringLiteral("add_") + model;
    case PermissionAction::Change:
        return QStringLiteral("change_") + model;
    case PermissionAction::AssignRole:
        return QStringLiteral("assign_role_user");
    }
    return QString();
}

bool PermissionChecker::can(const AuthSession &session, PermissionAction action,
                            const QString &appLabel, const QString &model)
{
    const QString code = codename(action, appLabel, model);
    return !code.isEmpty() && session.permissions.contains(code);
}
