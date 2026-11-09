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
