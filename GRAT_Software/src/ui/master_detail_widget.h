// master_detail_widget.h — child rows of the selected parent record
// (design §5.2 / §7.4, Task 12).
//
// One MasterDetailWidget renders one ChildTableSpec (a link/association table)
// below its parent's CrudWidget. It lists only the rows that belong to the
// currently selected parent; with no parent selected it stays empty and disabled.
//
// Child rows are added, edited and removed inline with a compact RegisterForm.
// The FK to the parent is filled in automatically. waste_wasteconstituent keeps
// its is_active lifecycle (logical delete / inactivate / reactivate); the other
// link tables have no state and are physically inserted and deleted (design §4).

#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QWidget>

#include "app/session.h"
#include "core/auth/permission_checker.h"
#include "core/crud/table_gateway.h"
#include "core/crud/table_spec.h"

class Database;
class QLabel;
class QPushButton;
class QStandardItemModel;
class QTableView;
class RegisterForm;

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

    // Reload FK lists and child rows after a connection recovery (Req 8.4).
    void refreshAfterReconnect();

    // True while an Add/Edit form is open, so a parent refresh can preserve it.
    bool hasPendingEdit() const;

signals:
    void permissionsChanged();

private slots:
    void onSelectionChanged();

private:
    enum class Operation {
        Idle,
        Adding,
        Editing
    };

    void startAdd();
    void startEdit();
    void removeRow();
    void applyState(const QString &newState);
    void save();
    void cancel();
    void updateCommandState();
    bool allowed(PermissionAction action) const;
    bool ensureDatabaseAvailable();
    bool isRolePermissions() const;
    QVariantMap currentRecord() const;

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
    RegisterForm *m_form = nullptr;

    QPushButton *m_addButton = nullptr;
    QPushButton *m_editButton = nullptr;
    QPushButton *m_removeButton = nullptr;
    QPushButton *m_inactivateButton = nullptr;
    QPushButton *m_reactivateButton = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_cancelButton = nullptr;

    Operation m_operation = Operation::Idle;
    QVariant m_parentKey;
    QList<int> m_gridColumns;
    QList<QVariantMap> m_records;
    int m_selectedRow = -1;
    bool m_refreshing = false;
    bool m_rowsLoaded = false;

    // FK column → (referenced pk as string → display label) for the grid.
    QHash<QString, QHash<QString, QString>> m_refLabels;
};
