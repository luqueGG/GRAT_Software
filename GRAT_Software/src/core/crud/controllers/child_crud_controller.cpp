#include "child_crud_controller.h"

#include <utility>

ChildCrudController::ChildCrudController(const TableSpec &spec, const ChildTableSpec &child,
                                         const AuthSession &session, IRecordRepository &repository,
                                         RecordValidator validate)
    : CrudController(spec, session, repository, std::move(validate)), m_child(child)
{
}

bool ChildCrudController::hasParent() const
{
    return !m_parentKey.isNull() && !m_parentKey.toString().isEmpty();
}

bool ChildCrudController::isRolePermissions() const
{
    return QLatin1String(m_child.table) == QLatin1String("auth_group_permissions");
}

void ChildCrudController::setParentKey(const QVariant &key)
{
    cancel();
    m_rowsLoaded = false;
    m_parentKey = key;
    if (key.isNull() || key.toString().isEmpty()
        || (QLatin1String(m_child.appLabel) == QLatin1String("admin")
            && !PermissionChecker::canManageRoles(m_session)))
        m_parentKey = {};
}

void ChildCrudController::selectRecord(const QVariantMap &record)
{
    // A selected row must belong to this collection, even outside the widget.
    if (!record.isEmpty()
        && (!hasParent() || record.value(QString::fromLatin1(m_child.fkToParent)).toString()
                              != m_parentKey.toString())) {
        CrudController::selectRecord({});
        return;
    }
    CrudController::selectRecord(record);
}

bool ChildCrudController::can(PermissionAction action) const
{
    if (!hasParent() || !m_session.isValid())
        return false;
    if (QLatin1String(m_child.appLabel) == QLatin1String("admin"))
        return m_rowsLoaded && PermissionChecker::canManageRoles(m_session);
    if (QLatin1String(m_child.appLabel) == QLatin1String("user"))
        return PermissionChecker::can(m_session, PermissionAction::AssignRole,
                                      QString::fromLatin1(m_child.appLabel),
                                      QString::fromLatin1(m_child.model));
    return CrudController::can(action);
}

bool ChildCrudController::canRead() const
{
    if (!hasParent() || !m_session.isValid())
        return false;
    // The first successful read enables role writes; do not require it to read.
    if (QLatin1String(m_child.appLabel) == QLatin1String("admin"))
        return PermissionChecker::canManageRoles(m_session);
    // Viewing a user includes its assignments; changing them still requires
    // assign_role_user. Preserve read-only access for supervisors.
    if (QLatin1String(m_child.appLabel) == QLatin1String("user"))
        return can(PermissionAction::AssignRole)
            || PermissionChecker::can(m_session, PermissionAction::View,
                                       QStringLiteral("user"), QStringLiteral("user"));
    return can(PermissionAction::View);
}

bool ChildCrudController::canEdit() const
{
    return !isRolePermissions() && can(PermissionAction::Change);
}

RecordListResult ChildCrudController::records()
{
    if (!hasParent()) {
        m_rowsLoaded = false;
        return {true, {}, {}};
    }
    RecordListResult result = CrudController::records();
    m_rowsLoaded = result.success;
    return result;
}

QVariant ChildCrudController::scopeKey() const
{
    return m_parentKey;
}

QVariantMap ChildCrudController::prepareValues(const QVariantMap &values, bool isInsert) const
{
    QVariantMap prepared = CrudController::prepareValues(values, isInsert);
    prepared.insert(QString::fromLatin1(m_child.fkToParent), m_parentKey);
    return prepared;
}

RecordWriteResult ChildCrudController::persist(const QVariantMap &values, bool isInsert)
{
    if (isRolePermissions()) {
        if (!isInsert)
            return {false, QStringLiteral("Los permisos se otorgan o revocan; no se modifican."), {}};
        return m_repository.grantPermission(m_session, m_parentKey,
                                            values.value(QStringLiteral("permission_id")));
    }
    return CrudController::persist(values, isInsert);
}

ControllerResult ChildCrudController::removeSelected()
{
    if (operation() != Operation::Idle)
        return rejected(QStringLiteral("Complete o cancele la operacion pendiente."));
    if (!can(PermissionAction::Change))
        return rejected(QStringLiteral("No esta autorizado para quitar este registro."));
    if (!hasSelection())
        return rejected(QStringLiteral("Seleccione primero una fila de la lista."));
    if (m_child.hasState)
        return changeState(QStringLiteral("*"));
    return finish(isRolePermissions()
                      ? m_repository.revokePermission(m_session, m_parentKey, currentPk())
                      : m_repository.remove(currentPk(), m_parentKey));
}
