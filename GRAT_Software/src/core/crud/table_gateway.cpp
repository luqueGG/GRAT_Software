#include "table_gateway.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "core/auth/permission_checker.h"
#include "core/errors/error_handler.h"

namespace {

QString unavailableMessage()
{
    return QStringLiteral("La base de datos no esta disponible. La operacion se cancelo.");
}

QString saveMessage()
{
    return QStringLiteral("No se pudo guardar el registro. No se realizaron cambios.");
}

QString updateMessage()
{
    return QStringLiteral("No se pudieron guardar los cambios. No se realizaron cambios.");
}

QString stateMessage()
{
    return QStringLiteral("No se pudo cambiar el estado del registro.");
}

QString removeMessage()
{
    return QStringLiteral("No se pudo eliminar el registro.");
}

DbResult unavailableResult()
{
    ErrorHandler::failure(ErrorHandler::Category::Connection,
                          QStringLiteral("database access"), QStringLiteral("Connection is closed."),
                          unavailableMessage());
    return {false, unavailableMessage(), {}, ErrorHandler::Category::Connection};
}

DbResult sqlFailureResult(const QSqlError &error, const QString &operation,
                          const QString &userMessage)
{
    const ErrorHandler::Failure failure = ErrorHandler::sqlFailure(
        error, ErrorHandler::Category::Persistence, operation, userMessage);
    return {false, failure.message, {}, failure.category};
}

} // namespace

TableGateway::TableGateway(QSqlDatabase db)
    : m_db(db)
{
}

QList<QVariantMap> TableGateway::selectAll(const TableSpec &spec, QString *errorMessage) const
{
    if (errorMessage)
        errorMessage->clear();
    QList<QVariantMap> rows;
    if (!m_db.isOpen()) {
        const ErrorHandler::Failure failure = ErrorHandler::failure(
            ErrorHandler::Category::Connection, QStringLiteral("select all"),
            QStringLiteral("Connection is closed."),
            QStringLiteral("No se pudieron cargar los registros porque la base de datos no esta disponible."));
        if (errorMessage)
            *errorMessage = failure.message;
        return rows;
    }

    QSqlQuery q(m_db);
    const QString sql = QStringLiteral("SELECT * FROM %1 ORDER BY %2")
                            .arg(QString::fromLatin1(spec.table), QString::fromLatin1(spec.pkColumn));
    if (!q.exec(sql)) {
        const ErrorHandler::Failure failure = ErrorHandler::sqlFailure(
            q.lastError(), ErrorHandler::Category::Persistence,
            QStringLiteral("select all from %1").arg(QString::fromLatin1(spec.table)),
            QStringLiteral("No se pudieron cargar los registros."));
        if (errorMessage)
            *errorMessage = failure.message;
        return rows;
    }

    while (q.next()) {
        QVariantMap row;
        const QSqlRecord rec = q.record();
        for (int i = 0; i < rec.count(); ++i)
            row.insert(rec.fieldName(i), q.value(i));
        rows.append(row);
    }
    return rows;
}

QList<QVariantMap> TableGateway::rolePermissionCatalog(const QVariant &roleKey,
                                                      QString *errorMessage) const
{
    if (errorMessage)
        errorMessage->clear();
    QList<QVariantMap> rows;
    if (!m_db.isOpen()) {
        const ErrorHandler::Failure failure = ErrorHandler::failure(
            ErrorHandler::Category::Connection, QStringLiteral("permission catalog"),
            QStringLiteral("Connection is closed."),
            QStringLiteral("No se pudo cargar el catalogo de permisos porque la "
                           "base de datos no esta disponible."));
        if (errorMessage)
            *errorMessage = failure.message;
        return rows;
    }

    QSqlQuery query(m_db);
    const bool prepared = query.prepare(QStringLiteral(
        "SELECT p.id, p.name, p.app_label, p.model, p.codename, "
        "EXISTS(SELECT 1 FROM auth_group_permissions gp "
        "WHERE gp.permission_id = p.id AND gp.group_id = :role) AS assigned "
        "FROM auth_permission p ORDER BY p.app_label, p.model, p.codename"));
    if (prepared)
        query.bindValue(QStringLiteral(":role"), roleKey);
    if (!prepared || !query.exec()) {
        const ErrorHandler::Failure failure = ErrorHandler::sqlFailure(
            query.lastError(), ErrorHandler::Category::Persistence,
            QStringLiteral("load permission catalog"),
            QStringLiteral("No se pudo cargar el catalogo de permisos."));
        if (errorMessage)
            *errorMessage = failure.message;
        return rows;
    }
    const QSqlRecord record = query.record();
    while (query.next()) {
        QVariantMap row;
        for (int i = 0; i < record.count(); ++i)
            row.insert(record.fieldName(i), query.value(i));
        rows.append(row);
    }
    return rows;
}

QList<QVariantMap> TableGateway::selectChildren(const ChildTableSpec &child,
                                               const QVariant &parentKey,
                                               QString *errorMessage) const
{
    if (errorMessage)
        errorMessage->clear();
    QList<QVariantMap> rows;
    if (parentKey.isNull() || parentKey.toString().isEmpty())
        return rows;
    if (!m_db.isOpen()) {
        const ErrorHandler::Failure failure = ErrorHandler::failure(
            ErrorHandler::Category::Connection, QStringLiteral("select child rows"),
            QStringLiteral("Connection is closed."),
            QStringLiteral("No se pudieron cargar los registros asignados porque la "
                           "base de datos no esta disponible."));
        if (errorMessage)
            *errorMessage = failure.message;
        return rows;
    }

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT * FROM %1 WHERE %2 = ? ORDER BY id")
                  .arg(QString::fromLatin1(child.table), QString::fromLatin1(child.fkToParent)));
    q.addBindValue(parentKey);
    if (!q.exec()) {
        const ErrorHandler::Failure failure = ErrorHandler::sqlFailure(
            q.lastError(), ErrorHandler::Category::Persistence,
            QStringLiteral("select child rows from %1").arg(QString::fromLatin1(child.table)),
            QStringLiteral("No se pudieron cargar los registros asignados."));
        if (errorMessage)
            *errorMessage = failure.message;
        return rows;
    }

    const QSqlRecord rec = q.record();
    while (q.next()) {
        QVariantMap row;
        for (int i = 0; i < rec.count(); ++i)
            row.insert(rec.fieldName(i), q.value(i));
        rows.append(row);
    }
    return rows;
}

QList<RefValue> TableGateway::loadRefValues(const QString &refTable, const QString &refPk,
                                             const QString &refDisplay,
                                             QString *errorMessage) const
{
    if (errorMessage)
        errorMessage->clear();

    QList<RefValue> values;
    if (!m_db.isOpen()) {
        const ErrorHandler::Failure failure = ErrorHandler::failure(
            ErrorHandler::Category::Connection,
            QStringLiteral("load reference list %1").arg(refTable),
            QStringLiteral("Connection is closed."),
            QStringLiteral("No se pudieron cargar los registros relacionados porque la "
                           "base de datos no esta disponible."));
        if (errorMessage)
            *errorMessage = failure.message;
        return values;
    }

    QSqlQuery q(m_db);
    const QString selectCols = refDisplay.isEmpty()
        ? refPk
        : QStringLiteral("%1, %2").arg(refPk, refDisplay);
    const QString sql = QStringLiteral("SELECT %1 FROM %2 ORDER BY %3")
                            .arg(selectCols, refTable, refPk);
    if (!q.exec(sql)) {
        const ErrorHandler::Failure failure = ErrorHandler::sqlFailure(
            q.lastError(), ErrorHandler::Category::Reference,
            QStringLiteral("load reference list %1").arg(refTable),
            QStringLiteral("No se pudieron cargar los registros relacionados. "
                           "El campo afectado permanece deshabilitado."));
        if (errorMessage)
            *errorMessage = failure.message;
        return values;
    }

    while (q.next()) {
        RefValue rv;
        rv.pk = q.value(0);
        if (refDisplay.isEmpty() || q.record().count() < 2) {
            rv.text = rv.pk.toString();
        } else {
            rv.text = QStringLiteral("%1 - %2").arg(rv.pk.toString(), q.value(1).toString());
        }
        values.append(rv);
    }
    return values;
}

bool TableGateway::exists(const TableSpec &spec, const QVariant &pk) const
{
    if (!m_db.isOpen())
        return false;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT 1 FROM %1 WHERE %2 = ? LIMIT 1")
                  .arg(QString::fromLatin1(spec.table), QString::fromLatin1(spec.pkColumn)));
    q.addBindValue(pk);
    if (!q.exec()) {
        ErrorHandler::sqlFailure(
            q.lastError(), ErrorHandler::Category::Persistence,
            QStringLiteral("check duplicate key in %1").arg(QString::fromLatin1(spec.table)));
        return false;
    }
    return q.next();
}

bool TableGateway::constituentExists(const QVariant &wasteKey, const QVariant &constituentKey,
                                       const QVariant &excludePk) const
{
    if (!m_db.isOpen() || wasteKey.isNull() || constituentKey.isNull())
        return false;

    QString sql = QStringLiteral("SELECT 1 FROM waste_wasteconstituent "
                                "WHERE id_waste_id = :waste AND id_constituent_id = :constituent");
    if (!excludePk.isNull())
        sql += QStringLiteral(" AND id <> :excluded");
    QSqlQuery q(m_db);
    if (!q.prepare(sql + QStringLiteral(" LIMIT 1")))
        return false;
    q.bindValue(QStringLiteral(":waste"), wasteKey);
    q.bindValue(QStringLiteral(":constituent"), constituentKey);
    if (!excludePk.isNull())
        q.bindValue(QStringLiteral(":excluded"), excludePk);
    if (!q.exec()) {
        ErrorHandler::sqlFailure(
            q.lastError(), ErrorHandler::Category::Persistence,
            QStringLiteral("check duplicate constituent"));
        return false;
    }
    return q.next();
}

DbResult TableGateway::insert(const TableSpec &spec, const QVariantMap &values) const
{
    DbResult res;
    if (!m_db.isOpen())
        return unavailableResult();

    // Collect columns to insert (skip identity PK — auto-generated; skip columns
    // not present in values — DB defaults apply, e.g. is_active='A', timestamps).
    QStringList cols;
    const QString pkCol = QString::fromLatin1(spec.pkColumn);
    for (const auto &c : spec.columns) {
        const QString col = QString::fromLatin1(c.dbColumn);
        if (spec.identityPk && col == pkCol)
            continue;
        if (!values.contains(col))
            continue;
        cols << col;
    }
    if (cols.isEmpty()) {
        return {false, QStringLiteral("No hay valores para guardar."), {},
                ErrorHandler::Category::Validation};
    }

    if (!m_db.transaction()) {
        return sqlFailureResult(m_db.lastError(), QStringLiteral("start insert transaction"),
                                saveMessage());
    }

    auto fail = [this](const QSqlError &error, const QString &operation,
                       const QString &message) -> DbResult {
        m_db.rollback();
        return sqlFailureResult(error, operation, message);
    };

    QSqlQuery q(m_db);
    QStringList phs;
    for (const QString &c : cols)
        phs << QStringLiteral(":") + c;
    const QString sql = QStringLiteral("INSERT INTO %1 (%2) VALUES (%3)")
                            .arg(QString::fromLatin1(spec.table), cols.join(QStringLiteral(", ")),
                                 phs.join(QStringLiteral(", ")));
    if (!q.prepare(sql))
        return fail(q.lastError(), QStringLiteral("prepare insert into %1")
                                       .arg(QString::fromLatin1(spec.table)), saveMessage());
    for (const QString &c : cols)
        q.bindValue(QStringLiteral(":") + c, values.value(c));
    if (!q.exec())
        return fail(q.lastError(), QStringLiteral("insert into %1")
                                       .arg(QString::fromLatin1(spec.table)), saveMessage());
    if (q.numRowsAffected() != 1) {
        return fail(QSqlError(), QStringLiteral("insert into %1")
                                     .arg(QString::fromLatin1(spec.table)), saveMessage());
    }

    const QVariant insertedPk = spec.identityPk ? q.lastInsertId() : values.value(pkCol);

    if (!m_db.commit()) {
        const QSqlError error = m_db.lastError();
        m_db.rollback();
        return sqlFailureResult(error, QStringLiteral("commit insert into %1")
                                          .arg(QString::fromLatin1(spec.table)), saveMessage());
    }
    res.success = true;
    res.pk = insertedPk;
    return res;
}

DbResult TableGateway::update(const TableSpec &spec, const QVariant &pk,
                              const QVariantMap &values) const
{
    DbResult res;
    if (!m_db.isOpen())
        return unavailableResult();

    const QString pkCol = QString::fromLatin1(spec.pkColumn);
    QStringList setClauses;
    QStringList cols;
    for (const auto &c : spec.columns) {
        const QString col = QString::fromLatin1(c.dbColumn);
        if (col == pkCol)
            continue; // never update the PK (Req 3.3)
        if (c.isState)
            continue; // state changes only via setState (Req 3.4)
        if (!values.contains(col))
            continue;
        setClauses << QStringLiteral("%1 = :%1").arg(col);
        cols << col;
    }
    if (setClauses.isEmpty()) {
        return {false, QStringLiteral("No hay valores para actualizar."), {},
                ErrorHandler::Category::Validation};
    }

    if (!m_db.transaction()) {
        return sqlFailureResult(m_db.lastError(), QStringLiteral("start update transaction"),
                                updateMessage());
    }

    auto fail = [this](const QSqlError &error, const QString &operation,
                       const QString &message) -> DbResult {
        m_db.rollback();
        return sqlFailureResult(error, operation, message);
    };

    QSqlQuery q(m_db);
    const QString sql = QStringLiteral("UPDATE %1 SET %2 WHERE %3 = :pk")
                            .arg(QString::fromLatin1(spec.table),
                                 setClauses.join(QStringLiteral(", ")), pkCol);
    if (!q.prepare(sql))
        return fail(q.lastError(), QStringLiteral("prepare update %1")
                                       .arg(QString::fromLatin1(spec.table)), updateMessage());
    for (const QString &c : cols)
        q.bindValue(QStringLiteral(":") + c, values.value(c));
    q.bindValue(QStringLiteral(":pk"), pk);
    if (!q.exec())
        return fail(q.lastError(), QStringLiteral("update %1")
                                       .arg(QString::fromLatin1(spec.table)), updateMessage());
    if (q.numRowsAffected() != 1) {
        return fail(QSqlError(), QStringLiteral("update %1 (record not found)")
                                     .arg(QString::fromLatin1(spec.table)),
                    QStringLiteral("El registro ya no esta disponible. "
                                   "Actualice la tabla e intentelo de nuevo."));
    }
    if (!m_db.commit()) {
        const QSqlError error = m_db.lastError();
        m_db.rollback();
        return sqlFailureResult(error, QStringLiteral("commit update %1")
                                          .arg(QString::fromLatin1(spec.table)), updateMessage());
    }
    res.success = true;
    return res;
}

DbResult TableGateway::setState(const TableSpec &spec, const QVariant &pk,
                                const QString &newState) const
{
    DbResult res;
    if (!m_db.isOpen())
        return unavailableResult();
    if (newState != QLatin1String("A") && newState != QLatin1String("I")
        && newState != QLatin1String("*")) {
        return {false, stateMessage(), {}, ErrorHandler::Category::Validation};
    }

    if (!m_db.transaction()) {
        return sqlFailureResult(m_db.lastError(), QStringLiteral("start state transaction"),
                                stateMessage());
    }

    auto fail = [this](const QSqlError &error, const QString &operation,
                       const QString &message) -> DbResult {
        m_db.rollback();
        return sqlFailureResult(error, operation, message);
    };

    QSqlQuery q(m_db);
    const QString sql = QStringLiteral("UPDATE %1 SET is_active = :state WHERE %2 = :pk")
                            .arg(QString::fromLatin1(spec.table),
                                 QString::fromLatin1(spec.pkColumn));
    if (!q.prepare(sql))
        return fail(q.lastError(), QStringLiteral("prepare state change on %1")
                                       .arg(QString::fromLatin1(spec.table)), stateMessage());
    q.bindValue(QStringLiteral(":state"), newState);
    q.bindValue(QStringLiteral(":pk"), pk);
    if (!q.exec())
        return fail(q.lastError(), QStringLiteral("state change on %1")
                                       .arg(QString::fromLatin1(spec.table)), stateMessage());
    if (q.numRowsAffected() != 1) {
        return fail(QSqlError(), QStringLiteral("state change on %1 (record not found)")
                                     .arg(QString::fromLatin1(spec.table)),
                    QStringLiteral("El registro ya no esta disponible. "
                                   "Actualice la tabla e intentelo de nuevo."));
    }
    if (!m_db.commit()) {
        const QSqlError error = m_db.lastError();
        m_db.rollback();
        return sqlFailureResult(error, QStringLiteral("commit state change on %1")
                                          .arg(QString::fromLatin1(spec.table)), stateMessage());
    }
    res.success = true;
    return res;
}

DbResult TableGateway::remove(const TableSpec &spec, const QVariant &pk) const
{
    DbResult res;
    if (!m_db.isOpen())
        return unavailableResult();

    // Tables with a lifecycle are deleted logically via setState('*') (Req 4.1).
    for (const auto &c : spec.columns) {
        if (c.isState) {
            ErrorHandler::failure(ErrorHandler::Category::Validation,
                                  QStringLiteral("physical delete"),
                                  QStringLiteral("Attempted physical delete on managed table %1")
                                      .arg(QString::fromLatin1(spec.table)),
                                  removeMessage());
            return {false, removeMessage(), {}, ErrorHandler::Category::Validation};
        }
    }

    if (!m_db.transaction()) {
        return sqlFailureResult(m_db.lastError(), QStringLiteral("start delete transaction"),
                                removeMessage());
    }

    QSqlQuery q(m_db);
    const QString sql = QStringLiteral("DELETE FROM %1 WHERE %2 = :pk")
                            .arg(QString::fromLatin1(spec.table),
                                 QString::fromLatin1(spec.pkColumn));
    if (!q.prepare(sql)) {
        const QSqlError error = q.lastError();
        m_db.rollback();
        return sqlFailureResult(error, QStringLiteral("prepare delete from %1")
                                          .arg(QString::fromLatin1(spec.table)), removeMessage());
    }
    q.bindValue(QStringLiteral(":pk"), pk);
    if (!q.exec()) {
        const QSqlError error = q.lastError();
        m_db.rollback();
        return sqlFailureResult(error, QStringLiteral("delete from %1")
                                          .arg(QString::fromLatin1(spec.table)), removeMessage());
    }
    if (q.numRowsAffected() != 1) {
        m_db.rollback();
        return {false, QStringLiteral("El registro ya no esta disponible. "
                                      "Actualice la tabla e intentelo de nuevo."), {},
                ErrorHandler::Category::Persistence};
    }
    if (!m_db.commit()) {
        const QSqlError error = m_db.lastError();
        m_db.rollback();
        return sqlFailureResult(error, QStringLiteral("commit delete from %1")
                                          .arg(QString::fromLatin1(spec.table)), removeMessage());
    }
    res.success = true;
    return res;
}

DbResult TableGateway::insertChild(const ChildTableSpec &child, const QVariant &parentKey,
                                    const QVariantMap &values) const
{
    if (parentKey.isNull() || parentKey.toString().isEmpty())
        return {false, QStringLiteral("Seleccione primero un registro principal guardado."), {},
                ErrorHandler::Category::Validation};

    TableSpec spec;
    spec.table = child.table;
    spec.pkColumn = "id";
    spec.identityPk = true;
    spec.columns = child.columns;
    QVariantMap childValues = values;
    childValues.insert(QString::fromLatin1(child.fkToParent), parentKey);
    return insert(spec, childValues);
}

DbResult TableGateway::updateChild(const ChildTableSpec &child, const QVariant &parentKey,
                                    const QVariant &pk, const QVariantMap &values) const
{
    QVariantMap assignments;
    for (const auto &column : child.columns) {
        const QString name = QString::fromLatin1(column.dbColumn);
        if (name == QLatin1String("id") || name == QLatin1String(child.fkToParent)
            || column.isState || column.autoManaged || !column.editable)
            continue;
        if (values.contains(name))
            assignments.insert(name, values.value(name));
    }
    if (assignments.isEmpty())
        return {false, QStringLiteral("No hay valores para actualizar."), {},
                ErrorHandler::Category::Validation};
    return writeChild(child, parentKey, pk, assignments);
}

DbResult TableGateway::removeChild(const ChildTableSpec &child, const QVariant &parentKey,
                                    const QVariant &pk) const
{
    if (child.hasState)
        return setChildState(child, parentKey, pk, QStringLiteral("*"));
    return writeChild(child, parentKey, pk, {});
}

DbResult TableGateway::setChildState(const ChildTableSpec &child, const QVariant &parentKey,
                                      const QVariant &pk, const QString &newState) const
{
    if (!child.hasState || (newState != QLatin1String("A") && newState != QLatin1String("I")
                            && newState != QLatin1String("*")))
        return {false, stateMessage(), {}, ErrorHandler::Category::Validation};
    return writeChild(child, parentKey, pk, {{QStringLiteral("is_active"), newState}});
}

DbResult TableGateway::writeChild(const ChildTableSpec &child, const QVariant &parentKey,
                                   const QVariant &pk, const QVariantMap &assignments) const
{
    if (!m_db.isOpen())
        return unavailableResult();
    if (parentKey.isNull() || parentKey.toString().isEmpty() || pk.isNull())
        return {false, QStringLiteral("Seleccione primero un registro hijo guardado."), {},
                ErrorHandler::Category::Validation};

    QStringList clauses;
    for (auto it = assignments.cbegin(); it != assignments.cend(); ++it)
        clauses << QStringLiteral("%1 = :value_%1").arg(it.key());
    const QString table = QString::fromLatin1(child.table);
    const QString command = assignments.isEmpty()
        ? QStringLiteral("DELETE FROM %1").arg(table)
        : QStringLiteral("UPDATE %1 SET %2").arg(table, clauses.join(QStringLiteral(", ")));

    if (!m_db.transaction()) {
        return sqlFailureResult(m_db.lastError(), QStringLiteral("start child transaction"),
                                QStringLiteral("No se pudo cambiar la fila hija. "
                                               "No se realizaron cambios."));
    }

    auto fail = [this](const QSqlError &error, const QString &operation) -> DbResult {
        m_db.rollback();
        return sqlFailureResult(error, operation,
                                QStringLiteral("No se pudo cambiar la fila hija. "
                                               "No se realizaron cambios."));
    };

    QSqlQuery q(m_db);
    bool ok = q.prepare(command + QStringLiteral(" WHERE id = :child_pk AND %1 = :parent_pk")
                                      .arg(QString::fromLatin1(child.fkToParent)));
    if (ok) {
        for (auto it = assignments.cbegin(); it != assignments.cend(); ++it)
            q.bindValue(QStringLiteral(":value_") + it.key(), it.value());
        q.bindValue(QStringLiteral(":child_pk"), pk);
        q.bindValue(QStringLiteral(":parent_pk"), parentKey);
        ok = q.exec();
    }
    if (!ok)
        return fail(q.lastError(), QStringLiteral("change child row in %1").arg(table));
    if (q.numRowsAffected() != 1)
        return fail(QSqlError(), QStringLiteral("change child row in %1 (record not found)")
                                     .arg(table));
    if (!m_db.commit()) {
        const QSqlError error = m_db.lastError();
        m_db.rollback();
        return sqlFailureResult(error, QStringLiteral("commit child row in %1").arg(table),
                                QStringLiteral("No se pudo cambiar la fila hija. "
                                               "No se realizaron cambios."));
    }
    return {true, {}, pk};
}

QString TableGateway::genericError()
{
    return QStringLiteral("No se pudo completar la operacion.");
}

DbResult TableGateway::grantRolePermission(const AuthSession &session, const QVariant &roleKey,
                                           const QVariant &permissionKey) const
{
    return writeRolePermission(session, roleKey, permissionKey, true);
}

DbResult TableGateway::revokeRolePermission(const AuthSession &session, const QVariant &roleKey,
                                            const QVariant &linkKey) const
{
    return writeRolePermission(session, roleKey, linkKey, false);
}

DbResult TableGateway::writeRolePermission(const AuthSession &session, const QVariant &roleKey,
                                           const QVariant &key, bool grant) const
{
    if (!PermissionChecker::canManageRoles(session))
        return {false, QStringLiteral("No esta autorizado para gestionar roles."), {},
                ErrorHandler::Category::Validation};
    if (!m_db.isOpen())
        return unavailableResult();

    bool roleValid = false;
    bool keyValid = false;
    const qint64 roleId = roleKey.toLongLong(&roleValid);
    const qint64 selectedId = key.toLongLong(&keyValid);
    if (!roleValid || !keyValid || roleId <= 0 || selectedId <= 0)
        return {false, QStringLiteral("Seleccione primero un rol y un permiso validos."), {},
                ErrorHandler::Category::Validation};

    if (!m_db.transaction()) {
        return sqlFailureResult(m_db.lastError(), QStringLiteral("start role transaction"),
                                genericError());
    }

    auto fail = [this](const QSqlError &error, const QString &operation,
                       const QString &message = QString()) -> DbResult {
        m_db.rollback();
        return sqlFailureResult(error, operation,
                                message.isEmpty() ? genericError() : message);
    };

    if (grant) {
        QSqlQuery check(m_db);
        if (!check.prepare(QStringLiteral(
                "SELECT EXISTS(SELECT 1 FROM auth_group WHERE id = :role), "
                "EXISTS(SELECT 1 FROM auth_permission WHERE id = :permission), "
                "EXISTS(SELECT 1 FROM auth_group_permissions "
                "WHERE group_id = :role AND permission_id = :permission)"))) {
            return fail(check.lastError(), QStringLiteral("prepare role permission check"),
                        QStringLiteral("No se pudo asignar el permiso."));
        }
        check.bindValue(QStringLiteral(":role"), roleId);
        check.bindValue(QStringLiteral(":permission"), selectedId);
        if (!check.exec() || !check.next()) {
            return fail(check.lastError(), QStringLiteral("role permission check"),
                        QStringLiteral("No se pudo asignar el permiso."));
        }
        const bool selectionExists = check.value(0).toBool() && check.value(1).toBool();
        const bool duplicate = check.value(2).toBool();
        check.finish();
        if (!selectionExists) {
            return fail(QSqlError(), QStringLiteral("validate role permission selection"),
                        QStringLiteral("El rol o permiso seleccionado ya no esta disponible."));
        }
        if (duplicate) {
            return fail(QSqlError(), QStringLiteral("validate role permission selection"),
                        QStringLiteral("Este permiso ya esta asignado al rol seleccionado."));
        }
    }

    QSqlQuery query(m_db);
    const QString sql = grant
        ? QStringLiteral("INSERT INTO auth_group_permissions (group_id, permission_id) "
                         "VALUES (:role, :selected)")
        : QStringLiteral("DELETE FROM auth_group_permissions "
                         "WHERE group_id = :role AND id = :selected");
    if (!query.prepare(sql)) {
        return fail(query.lastError(), QStringLiteral("prepare role permission change"),
                    QStringLiteral("No se pudo guardar el cambio de permiso."));
    }
    query.bindValue(QStringLiteral(":role"), roleId);
    query.bindValue(QStringLiteral(":selected"), selectedId);
    if (!query.exec() || query.numRowsAffected() != 1) {
        return fail(query.lastError(), QStringLiteral("role permission change"),
                    QStringLiteral("No se pudo guardar el cambio de permiso."));
    }
    const QVariant resultKey = grant ? query.lastInsertId() : key;
    query.finish();
    if (!m_db.commit()) {
        const QSqlError error = m_db.lastError();
        m_db.rollback();
        return sqlFailureResult(error, QStringLiteral("commit role permission change"),
                                QStringLiteral("No se pudo guardar el cambio de permiso."));
    }
    return {true, {}, resultKey};
}
