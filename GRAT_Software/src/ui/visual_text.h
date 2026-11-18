// visual_text.h — display-only translations for values stored in SQLite.
// Values written to the database are never modified by this helper.

#pragma once

#include <QString>
#include <QStringList>

namespace VisualText {

QString roleName(const QString &name);
QStringList roleNames(const QStringList &names);
QString permissionName(const QString &name);
QString referenceLabel(const QString &refTable, const QString &refDisplay,
                       const QString &text);

} // namespace VisualText
