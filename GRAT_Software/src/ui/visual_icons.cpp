#include "visual_icons.h"

namespace {

QString resourcePath(const QString &key)
{
    return QStringLiteral(":/icons/") + key + QStringLiteral(".svg");
}

} // namespace

QIcon VisualIcons::icon(const QString &key)
{
    return QIcon(resourcePath(key));
}

QIcon VisualIcons::moduleIcon(const QString &module)
{
    if (module == QLatin1String("location"))
        return icon(QStringLiteral("location"));
    if (module == QLatin1String("catalog"))
        return icon(QStringLiteral("catalog"));
    if (module == QLatin1String("user"))
        return icon(QStringLiteral("users"));
    if (module == QLatin1String("waste"))
        return icon(QStringLiteral("waste"));
    if (module == QLatin1String("transfer"))
        return icon(QStringLiteral("transfer"));
    if (module == QLatin1String("admin"))
        return icon(QStringLiteral("admin"));
    return icon(QStringLiteral("catalog"));
}

QIcon VisualIcons::childIcon(const QString &childTable)
{
    if (childTable == QLatin1String("auth_group_permissions"))
        return icon(QStringLiteral("grant"));
    if (childTable == QLatin1String("user_user_groups"))
        return icon(QStringLiteral("users"));
    if (childTable == QLatin1String("user_user_user_permissions"))
        return icon(QStringLiteral("grant"));
    return icon(QStringLiteral("catalog"));
}
