// master_detail_widget.h — child rows of the selected parent record
// (design §5.2 / §7.4, Task 12).
//
// One MasterDetailWidget renders one ChildTableSpec (a link/association table)
// below its parent's CrudWidget. It lists only the rows that belong to the
// currently selected parent; with no parent selected it stays empty and disabled.

#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QWidget>

#include "app/session.h"
#include "core/crud/table_gateway.h"
#include "core/crud/table_spec.h"

class Database;
class QLabel;
class QStandardItemModel;
class QTableView;

class MasterDetailWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MasterDetailWidget(Database &database, const ChildTableSpec &child,
                                const AuthSession &session, QWidget *parent = nullptr);

    // Key of the selected parent record; an invalid QVariant means "no parent"
    // (nothing selected, or the parent is still being added).
    void setParentKey(const QVariant &key);
    QVariant parentKey() const;

    // Human-readable name of the child table, e.g. for a tab title.
    QString title() const;

    // Reload the child rows of the current parent.
    void refresh();

private slots:
    void onSelectionChanged();

private:
    // A child spec viewed as a regular TableSpec with its identity 'id' PK, so the
    // generic TableGateway / Validator / RegisterForm can be reused unchanged.
    static TableSpec toTableSpec(const ChildTableSpec &child);

    QString displayValue(const ColumnSpec &column, const QVariant &value) const;
    QString stateText(const QString &state) const;
    void loadReferenceLabels();

    Database &m_database;
    const ChildTableSpec &m_child;
    TableSpec m_spec;
    AuthSession m_session;
    TableGateway m_gateway;

    QLabel *m_hintLabel = nullptr;
    QTableView *m_grid = nullptr;
    QStandardItemModel *m_gridModel = nullptr;

    QVariant m_parentKey;
    QList<int> m_gridColumns;
    QList<QVariantMap> m_records;
    int m_selectedRow = -1;
    bool m_refreshing = false;

    // FK column → (referenced pk as string → display label) for the grid.
    QHash<QString, QHash<QString, QString>> m_refLabels;
};
