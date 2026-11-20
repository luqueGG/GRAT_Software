#pragma once

#include "core/crud/controllers/crud_controller.h"

class ChildCrudController final : public CrudController
{
public:
    ChildCrudController(const TableSpec &spec, const ChildTableSpec &child,
                        const AuthSession &session, IRecordRepository &repository,
                        RecordValidator validate);

    void setParentKey(const QVariant &key);
    const QVariant &parentKey() const { return m_parentKey; }
    void selectRecord(const QVariantMap &record) override;
    bool can(PermissionAction action) const override;
    bool canEdit() const override;
    bool isRolePermissions() const;
    RecordListResult records() override;
    ControllerResult removeSelected();

protected:
    bool canRead() const override;
    QVariant scopeKey() const override;
    QVariantMap prepareValues(const QVariantMap &values, bool isInsert) const override;
    RecordWriteResult persist(const QVariantMap &values, bool isInsert) override;

private:
    bool hasParent() const;

    ChildTableSpec m_child;
    QVariant m_parentKey;
    bool m_rowsLoaded = false;
};
