// visual_icons.h — centralised access to the visual resources used by the UI.
// Presentation only: no application state is stored here.

#pragma once

#include <QIcon>
#include <QString>

namespace VisualIcons {

QIcon icon(const QString &key);
QIcon moduleIcon(const QString &module);
QIcon childIcon(const QString &childTable);

} // namespace VisualIcons
