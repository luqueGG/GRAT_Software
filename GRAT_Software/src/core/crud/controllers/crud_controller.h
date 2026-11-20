#pragma once

#include <functional>

#include "core/auth/permission_checker.h"
#include "core/crud/repositories/i_record_repository.h"
#include "core/crud/table_spec.h"
#include "core/crud/validator.h"

using RecordValidator = std::function<QList<ValidationError>(
    const TableSpec &, const QVariantMap &, bool, const QVariantMap &)>;

struct ControllerResult
{
    bool success = false;
    QString error;
    QList<ValidationError> validationErrors;
    QVariant pk;
};

// Application control independent of widgets and SQL. Dependencies must outlive it.
class CrudController
{
public:
    enum class Operation { Idle, Adding, Modifying };

    CrudController(const TableSpec &spec, const AuthSession &session,
                   IRecordRepository &repository, RecordValidator validate);
    virtual ~CrudController() = default;

    virtual bool can(PermissionAction action) const;
    virtual bool canEdit() const;
    virtual RecordListResult records();
    ControllerResult startAdd();
    ControllerResult startModify();
    virtual void selectRecord(const QVariantMap &record);
    void cancel();
    ControllerResult save(const QVariantMap &values);
    ControllerResult changeState(const QString &state);

    Operation operation() const { return m_operation; }
    const QVariantMap &currentRecord() const { return m_record; }

protected:
    virtual bool canRead() const;
    virtual QVariant scopeKey() const;
    virtual QVariantMap prepareValues(const QVariantMap &values, bool isInsert) const;
    virtual RecordWriteResult persist(const QVariantMap &values, bool isInsert);
    ControllerResult finish(const RecordWriteResult &result);
    bool hasColumn(const char *name) const;
    bool hasSelection() const;
    QVariant currentPk() const;
    static ControllerResult rejected(const QString &message);

    TableSpec m_spec;
    AuthSession m_session;
    IRecordRepository &m_repository;

private:
    RecordValidator m_validate;
    QVariantMap m_record;
    Operation m_operation = Operation::Idle;
};
