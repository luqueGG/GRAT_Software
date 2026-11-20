#include "crud_controller.h"

#include <QDateTime>
#include <utility>

#include "core/auth/auth_service.h"

CrudController::CrudController(const TableSpec &spec, const AuthSession &session,
                                 IRecordRepository &repository, RecordValidator validate)
    : m_spec(spec), m_session(session), m_repository(repository), m_validate(std::move(validate))
{
}

bool CrudController::can(PermissionAction action) const
{
    return m_session.isValid()
        && PermissionChecker::can(m_session, action, QString::fromLatin1(m_spec.appLabel),
                                   QString::fromLatin1(m_spec.model));
}

bool CrudController::canEdit() const
{
    return can(PermissionAction::Change);
}

bool CrudController::canRead() const
{
    return can(PermissionAction::View);
}

QVariant CrudController::scopeKey() const
{
    return {};
}

RecordListResult CrudController::records()
{
    if (!canRead())
        return {false, QStringLiteral("No esta autorizado para consultar estos registros."), {}};
    return m_repository.load(scopeKey());
}

ControllerResult CrudController::rejected(const QString &message)
{
    return {false, message, {}, {}};
}

bool CrudController::hasColumn(const char *name) const
{
    for (const auto &column : m_spec.columns) {
        if (QLatin1String(column.dbColumn) == QLatin1String(name))
            return true;
    }
    return false;
}

QVariant CrudController::currentPk() const
{
    return m_record.value(QString::fromLatin1(m_spec.pkColumn));
}

bool CrudController::hasSelection() const
{
    return !currentPk().isNull() && !currentPk().toString().isEmpty();
}

ControllerResult CrudController::startAdd()
{
    if (m_operation != Operation::Idle)
        return rejected(QStringLiteral("Ya hay una operacion pendiente."));
    if (!can(PermissionAction::Add))
        return rejected(QStringLiteral("No esta autorizado para agregar registros."));
    m_record.clear();
    m_operation = Operation::Adding;
    return {true, {}, {}, {}};
}

ControllerResult CrudController::startModify()
{
    if (m_operation != Operation::Idle)
        return rejected(QStringLiteral("Ya hay una operacion pendiente."));
    if (!canEdit())
        return rejected(QStringLiteral("No esta autorizado para modificar este registro."));
    if (!hasSelection())
        return rejected(QStringLiteral("Seleccione primero un registro en la tabla."));
    m_operation = Operation::Modifying;
    return {true, {}, {}, {}};
}

void CrudController::selectRecord(const QVariantMap &record)
{
    if (m_operation == Operation::Adding || record.isEmpty())
        m_operation = Operation::Idle;
    m_record = record;
}

void CrudController::cancel()
{
    m_operation = Operation::Idle;
    m_record.clear();
}

QVariantMap CrudController::prepareValues(const QVariantMap &values, bool isInsert) const
{
    QVariantMap prepared;
    for (const auto &column : m_spec.columns) {
        const QString name = QString::fromLatin1(column.dbColumn);
        if (!column.editable || column.autoManaged || column.isState)
            continue;
        if (name == QLatin1String(m_spec.pkColumn) && (!isInsert || m_spec.identityPk))
            continue;
        if (!values.contains(name))
            continue;
        if (!isInsert && column.type == ColumnType::Password && values.value(name).toString().isEmpty())
            continue;
        prepared.insert(name, values.value(name));
    }
    if (isInsert && hasColumn("created_by_id"))
        prepared.insert(QStringLiteral("created_by_id"), m_session.userId);
    if (!isInsert && hasColumn("updated_at"))
        prepared.insert(QStringLiteral("updated_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    return prepared;
}

RecordWriteResult CrudController::persist(const QVariantMap &values, bool isInsert)
{
    return isInsert ? m_repository.add(values, scopeKey())
                    : m_repository.change(currentPk(), values, scopeKey());
}

ControllerResult CrudController::finish(const RecordWriteResult &result)
{
    if (result.success)
        cancel();
    return {result.success, result.error, {}, result.pk};
}

ControllerResult CrudController::save(const QVariantMap &values)
{
    if (m_operation == Operation::Idle)
        return rejected(QStringLiteral("No hay una operacion pendiente. Elija Agregar o Modificar primero."));
    const bool isInsert = m_operation == Operation::Adding;
    if (!(isInsert ? can(PermissionAction::Add) : canEdit()))
        return rejected(QStringLiteral("No esta autorizado para guardar este registro."));
    if (!isInsert && !hasSelection())
        return rejected(QStringLiteral("Seleccione primero un registro en la tabla."));
    if (!m_validate)
        return rejected(QStringLiteral("No se pudo validar el registro."));

    QVariantMap prepared = prepareValues(values, isInsert);
    const auto errors = m_validate(m_spec, prepared, isInsert, isInsert ? QVariantMap() : m_record);
    if (!errors.isEmpty())
        return {false, {}, errors, {}};

    for (const auto &column : m_spec.columns) {
        const QString name = QString::fromLatin1(column.dbColumn);
        if (column.type == ColumnType::Password && prepared.contains(name)
            && !prepared.value(name).toString().isEmpty())
            prepared[name] = AuthService::hashPassword(prepared.value(name).toString());
    }
    return finish(persist(prepared, isInsert));
}

ControllerResult CrudController::changeState(const QString &state)
{
    if (m_operation != Operation::Idle)
        return rejected(QStringLiteral("Complete o cancele la operacion pendiente."));
    if (!can(PermissionAction::Change))
        return rejected(QStringLiteral("No esta autorizado para cambiar el estado."));
    if (!hasSelection())
        return rejected(QStringLiteral("Seleccione primero un registro en la tabla."));
    if (!hasColumn("is_active") || (state != QLatin1String("A") && state != QLatin1String("I")
                                  && state != QLatin1String("*")))
        return rejected(QStringLiteral("El cambio de estado no es valido."));
    return finish(m_repository.setState(currentPk(), state, scopeKey()));
}
