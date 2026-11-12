#include "table_gateway.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "core/auth/permission_checker.h"

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
        if (errorMessage)
            *errorMessage = QStringLiteral("The database is not connected.");
        return rows;
    }

    QSqlQuery q(m_db);
    const QString sql = QStringLiteral("SELECT * FROM %1 ORDER BY %2")
                            .arg(QString::fromLatin1(spec.table), QString::fromLatin1(spec.pkColumn));
    if (!q.exec(sql)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("The records could not be loaded.");
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
    QSqlQuery query(m_db);
    const bool ok = m_db.isOpen() && query.prepare(QStringLiteral(
        "SELECT p.id, p.name, p.app_label, p.model, p.codename, "
        "EXISTS(SELECT 1 FROM auth_group_permissions gp "
        "WHERE gp.permission_id = p.id AND gp.group_id = :role) AS assigned "
        "FROM auth_permission p ORDER BY p.app_label, p.model, p.codename"));
    if (ok)
        query.bindValue(QStringLiteral(":role"), roleKey);
    if (!ok || !query.exec()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("The permission catalog could not be loaded.");
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
        if (errorMessage)
            *errorMessage = QStringLiteral("The database is not connected.");
        return rows;
    }

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT * FROM %1 WHERE %2 = ? ORDER BY id")
                  .arg(QString::fromLatin1(child.table), QString::fromLatin1(child.fkToParent)));
    q.addBindValue(parentKey);
    if (!q.exec()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("The assigned records could not be loaded.");
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
                                             const QString &refDisplay) const
{
    QList<RefValue> values;
    if (!m_db.isOpen())
        return values;

    QSqlQuery q(m_db);
    const QString selectCols = refDisplay.isEmpty()
        ? refPk
        : QStringLiteral("%1, %2").arg(refPk, refDisplay);
    const QString sql = QStringLiteral("SELECT %1 FROM %2 ORDER BY %3")
                            .arg(selectCols, refTable, refPk);
    if (!q.exec(sql))
        return values;

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
    if (!q.exec())
        return false;
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
    return q.exec() && q.next();
}

DbResult TableGateway::insert(const TableSpec &spec, const QVariantMap &values) const
{
    DbResult res;
    if (!m_db.isOpen()) {
        res.error = QStringLiteral("The database is not connected.");
        return res;
    }

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
        res.error = QStringLiteral("There are no values to save.");
        return res;
    }

    const bool useTx = m_db.transaction();
    QSqlQuery q(m_db);
    QStringList phs;
    for (const QString &c : cols)
        phs << QStringLiteral(":") + c;
    const QString sql = QStringLiteral("INSERT INTO %1 (%2) VALUES (%3)")
                            .arg(QString::fromLatin1(spec.table), cols.join(QStringLiteral(", ")),
                                 phs.join(QStringLiteral(", ")));
    if (!q.prepare(sql)) {
        if (useTx) m_db.rollback();
        res.error = genericError();
        return res;
    }
    for (const QString &c : cols)
        q.bindValue(QStringLiteral(":") + c, values.value(c));
    if (!q.exec()) {
        if (useTx) m_db.rollback();
        res.error = genericError();
        return res;
    }

    QVariant insertedPk = spec.identityPk ? q.lastInsertId() : values.value(pkCol);

    if (useTx && !m_db.commit()) {
        res.error = genericError();
        return res;
    }
    res.success = true;
    res.pk = insertedPk;
    return res;
}

DbResult TableGateway::update(const TableSpec &spec, const QVariant &pk, const QVariantMap &values) const
{
    DbResult res;
    if (!m_db.isOpen()) {
        res.error = QStringLiteral("The database is not connected.");
        return res;
    }

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
        res.error = QStringLiteral("There are no values to update.");
        return res;
    }

    const bool useTx = m_db.transaction();
    QSqlQuery q(m_db);
    const QString sql = QStringLiteral("UPDATE %1 SET %2 WHERE %3 = :pk")
                            .arg(QString::fromLatin1(spec.table), setClauses.join(QStringLiteral(", ")),
                                 pkCol);
    if (!q.prepare(sql)) {
        if (useTx) m_db.rollback();
        res.error = genericError();
        return res;
    }
    for (const QString &c : cols)
        q.bindValue(QStringLiteral(":") + c, values.value(c));
    q.bindValue(QStringLiteral(":pk"), pk);
    if (!q.exec()) {
        if (useTx) m_db.rollback();
        res.error = genericError();
        return res;
    }
    if (useTx && !m_db.commit()) {
        res.error = genericError();
        return res;
    }
    res.success = true;
    return res;
}

DbResult TableGateway::setState(const TableSpec &spec, const QVariant &pk, const QString &newState) const
{
    DbResult res;
    if (!m_db.isOpen()) {
        res.error = QStringLiteral("The database is not connected.");
        return res;
    }

    const bool useTx = m_db.transaction();
    QSqlQuery q(m_db);
    const QString sql = QStringLiteral("UPDATE %1 SET is_active = :state WHERE %2 = :pk")
                            .arg(QString::fromLatin1(spec.table), QString::fromLatin1(spec.pkColumn));
    if (!q.prepare(sql)) {
        if (useTx) m_db.rollback();
        res.error = genericError();
        return res;
    }
    q.bindValue(QStringLiteral(":state"), newState);
    q.bindValue(QStringLiteral(":pk"), pk);
    if (!q.exec()) {
        if (useTx) m_db.rollback();
        res.error = genericError();
        return res;
    }
    if (useTx && !m_db.commit()) {
        res.error = genericError();
        return res;
    }
    res.success = true;
    return res;
}

DbResult TableGateway::remove(const TableSpec &spec, const QVariant &pk) const
{
    DbResult res;
    if (!m_db.isOpen()) {
        res.error = QStringLiteral("The database is not connected.");
        return res;
    }

    // Tables with a lifecycle are deleted logically via setState('*') (Req 4.1).
    for (const auto &c : spec.columns) {
        if (c.isState) {
            res.error = genericError();
            return res;
        }
    }

    const bool useTx = m_db.transaction();
    QSqlQuery q(m_db);
    const QString sql = QStringLiteral("DELETE FROM %1 WHERE %2 = :pk")
                            .arg(QString::fromLatin1(spec.table), QString::fromLatin1(spec.pkColumn));
    if (!q.prepare(sql)) {
        if (useTx) m_db.rollback();
        res.error = genericError();
        return res;
    }
    q.bindValue(QStringLiteral(":pk"), pk);
    if (!q.exec()) {
        if (useTx) m_db.rollback();
        res.error = genericError();
        return res;
    }
    if (useTx && !m_db.commit()) {
        res.error = genericError();
        return res;
    }
    res.success = true;
    return res;
}

DbResult TableGateway::insertChild(const ChildTableSpec &child, const QVariant &parentKey,
                                    const QVariantMap &values) const
{
    if (parentKey.isNull() || parentKey.toString().isEmpty())
        return {false, QStringLiteral("Select a saved parent record first."), {}};

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
        return {false, QStringLiteral("There are no values to update."), {}};
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
        return {false, genericError(), {}};
    return writeChild(child, parentKey, pk, {{QStringLiteral("is_active"), newState}});
}

DbResult TableGateway::writeChild(const ChildTableSpec &child, const QVariant &parentKey,
                                   const QVariant &pk, const QVariantMap &assignments) const
{
    if (!m_db.isOpen())
        return {false, QStringLiteral("The database is not connected."), {}};
    if (parentKey.isNull() || parentKey.toString().isEmpty() || pk.isNull())
        return {false, QStringLiteral("Select a saved child record first."), {}};

    QStringList clauses;
    for (auto it = assignments.cbegin(); it != assignments.cend(); ++it)
        clauses << QStringLiteral("%1 = :value_%1").arg(it.key());
    const QString table = QString::fromLatin1(child.table);
    const QString command = assignments.isEmpty()
        ? QStringLiteral("DELETE FROM %1").arg(table)
        : QStringLiteral("UPDATE %1 SET %2").arg(table, clauses.join(QStringLiteral(", ")));

    if (!m_db.transaction())
        return {false, genericError(), {}};
    QSqlQuery q(m_db);
    bool ok = q.prepare(command + QStringLiteral(" WHERE id = :child_pk AND %1 = :parent_pk")
                                      .arg(QString::fromLatin1(child.fkToParent)));
    if (ok) {
        for (auto it = assignments.cbegin(); it != assignments.cend(); ++it)
            q.bindValue(QStringLiteral(":value_") + it.key(), it.value());
        q.bindValue(QStringLiteral(":child_pk"), pk);
        q.bindValue(QStringLiteral(":parent_pk"), parentKey);
        ok = q.exec() && q.numRowsAffected() == 1;
    }
    if (!ok || !m_db.commit()) {
        m_db.rollback();
        return {false, genericError(), {}};
    }
    return {true, {}, pk};
}

QString TableGateway::genericError()
{
    return QStringLiteral("The operation could not be completed.");
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
        return {false, QStringLiteral("You are not authorized to manage roles."), {}};
    if (!m_db.isOpen())
        return {false, QStringLiteral("The database is not connected."), {}};

    bool roleValid = false;
    bool keyValid = false;
    const qint64 roleId = roleKey.toLongLong(&roleValid);
    const qint64 selectedId = key.toLongLong(&keyValid);
    if (!roleValid || !keyValid || roleId <= 0 || selectedId <= 0)
        return {false, QStringLiteral("Select a valid role and permission first."), {}};
    if (!m_db.transaction())
        return {false, genericError(), {}};

    auto fail = [this](const QString &message) -> DbResult {
        m_db.rollback();
        return {false, message, {}};
    };

    if (grant) {
        QSqlQuery check(m_db);
        if (!check.prepare(QStringLiteral(
                "SELECT EXISTS(SELECT 1 FROM auth_group WHERE id = :role), "
                "EXISTS(SELECT 1 FROM auth_permission WHERE id = :permission), "
                "EXISTS(SELECT 1 FROM auth_group_permissions "
                "WHERE group_id = :role AND permission_id = :permission)")))
            return fail(genericError());
        check.bindValue(QStringLiteral(":role"), roleId);
        check.bindValue(QStringLiteral(":permission"), selectedId);
        if (!check.exec() || !check.next())
            return fail(genericError());
        const bool selectionExists = check.value(0).toBool() && check.value(1).toBool();
        const bool duplicate = check.value(2).toBool();
        check.finish();
        if (!selectionExists)
            return fail(QStringLiteral("The selected role or permission is no longer available."));
        if (duplicate)
            return fail(QStringLiteral("This permission is already assigned to the selected role."));
    }

    QSqlQuery query(m_db);
    const QString sql = grant
        ? QStringLiteral("INSERT INTO auth_group_permissions (group_id, permission_id) "
                         "VALUES (:role, :selected)")
        : QStringLiteral("DELETE FROM auth_group_permissions "
                         "WHERE group_id = :role AND id = :selected");
    if (!query.prepare(sql))
        return fail(genericError());
    query.bindValue(QStringLiteral(":role"), roleId);
    query.bindValue(QStringLiteral(":selected"), selectedId);
    if (!query.exec() || query.numRowsAffected() != 1)
        return fail(genericError());
    const QVariant resultKey = grant ? query.lastInsertId() : key;
    query.finish();
    if (!m_db.commit())
        return fail(genericError());
    return {true, {}, resultKey};
}
