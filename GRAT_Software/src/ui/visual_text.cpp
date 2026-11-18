#include "visual_text.h"

#include <QHash>

namespace {

QString translateDisplay(const QString &text, bool isRole, bool isPermission)
{
    if (isRole)
        return VisualText::roleName(text);
    if (isPermission)
        return VisualText::permissionName(text);
    return text;
}

} // namespace

QString VisualText::roleName(const QString &name)
{
    static const QHash<QString, QString> names = {
        {QStringLiteral("Administrator"), QStringLiteral("Administrador")},
        {QStringLiteral("Supervisor"), QStringLiteral("Supervisor")},
        {QStringLiteral("Operator"), QStringLiteral("Operador")},
    };
    return names.value(name, name);
}

QStringList VisualText::roleNames(const QStringList &names)
{
    QStringList result;
    result.reserve(names.size());
    for (const QString &name : names)
        result.append(roleName(name));
    return result;
}

QString VisualText::permissionName(const QString &text)
{
    static const QHash<QString, QString> verbs = {
        {QStringLiteral("add"), QStringLiteral("Agregar")},
        {QStringLiteral("change"), QStringLiteral("Modificar")},
        {QStringLiteral("view"), QStringLiteral("Ver")},
        {QStringLiteral("assign role"), QStringLiteral("Asignar rol")},
    };
    static const QHash<QString, QString> models = {
        {QStringLiteral("constituent"), QStringLiteral("componente")},
        {QStringLiteral("containertype"), QStringLiteral("tipo de contenedor")},
        {QStringLiteral("toxicitylevel"), QStringLiteral("nivel de toxicidad")},
        {QStringLiteral("transporttype"), QStringLiteral("tipo de transporte")},
        {QStringLiteral("country"), QStringLiteral("país")},
        {QStringLiteral("subdivision"), QStringLiteral("subdivisión")},
        {QStringLiteral("destination"), QStringLiteral("destino")},
        {QStringLiteral("transfer"), QStringLiteral("traslado")},
        {QStringLiteral("transfertravelcompany"), QStringLiteral("empresa transportista de traslado")},
        {QStringLiteral("travelcompany"), QStringLiteral("empresa transportista")},
        {QStringLiteral("user"), QStringLiteral("usuario")},
        {QStringLiteral("generatorcompany"), QStringLiteral("empresa generadora")},
        {QStringLiteral("waste"), QStringLiteral("residuo")},
        {QStringLiteral("wasteconstituent"), QStringLiteral("componente de residuo")},
    };

    const QString prefix = QStringLiteral("Can ");
    if (!text.startsWith(prefix))
        return text;
    const QString body = text.mid(prefix.size());

    for (auto it = verbs.cbegin(); it != verbs.cend(); ++it) {
        const QString marker = it.key() + QLatin1Char(' ');
        if (!body.startsWith(marker))
            continue;
        const QString model = body.mid(marker.size());
        return it.value() + QLatin1Char(' ') + models.value(model, model);
    }
    return text;
}

QString VisualText::referenceLabel(const QString &refTable, const QString &refDisplay,
                                   const QString &text)
{
    const bool isRole = refTable == QLatin1String("auth_group")
        && refDisplay == QLatin1String("name");
    const bool isPermission = refTable == QLatin1String("auth_permission")
        && refDisplay == QLatin1String("name");
    if (!isRole && !isPermission)
        return text;

    // Reference list entries are composed as "<pk> - <display>" by TableGateway.
    const QString separator = QStringLiteral(" - ");
    const int split = text.indexOf(separator);
    if (split < 0)
        return translateDisplay(text, isRole, isPermission);

    const QString display = text.mid(split + separator.size());
    return text.left(split) + separator + translateDisplay(display, isRole, isPermission);
}
