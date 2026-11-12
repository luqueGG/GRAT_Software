#include "table_gateway.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

TableGateway::TableGateway(QSqlDatabase db)
    : m_db(db)
{
}

QList<QVariantMap> TableGateway::selectAll(const TableSpec &spec) const
{
    QList<QVariantMap> rows;
    if (!m_db.isOpen())
        return rows;

    QSqlQuery q(m_db);
    const QString sql = QStringLiteral("SELECT * FROM %1 ORDER BY %2")
                            .arg(QString::fromLatin1(spec.table), QString::fromLatin1(spec.pkColumn));
    if (!q.exec(sql))
        return rows;

    while (q.next()) {
        QVariantMap row;
        const QSqlRecord rec = q.record();
        for (int i = 0; i < rec.count(); ++i)
            row.insert(rec.fieldName(i), q.value(i));
        rows.append(row);
    }
    return rows;
}

QList<QVariantMap> TableGateway::selectChildren(const ChildTableSpec &child,
                                               const QVariant &parentKey) const
{
    QList<QVariantMap> rows;
    if (!m_db.isOpen() || parentKey.isNull() || parentKey.toString().isEmpty())
        return rows;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT * FROM %1 WHERE %2 = ? ORDER BY id")
                  .arg(QString::fromLatin1(child.table), QString::fromLatin1(child.fkToParent)));
    q.addBindValue(parentKey);
    if (!q.exec())
        return rows;

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

QString TableGateway::genericError()
{
    return QStringLiteral("The operation could not be completed.");
}
