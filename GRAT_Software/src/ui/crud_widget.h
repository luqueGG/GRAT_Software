// crud_widget.h — table screen: grid + register form + lifecycle commands
// (design §7.3, Task 11).
//
// One CrudWidget renders any managed table from its TableSpec. It shows every
// record regardless of is_active state (Req 2.1) with a state badge column, and
// hosts the metadata-driven RegisterForm.

#pragma once

#include <QList>
#include <QString>
#include <QVariantMap>
#include <QWidget>

#include "app/session.h"
#include "core/crud/table_gateway.h"
#include "core/crud/table_spec.h"

class Database;
class QPushButton;
class QStandardItemModel;
class QTableView;
class RegisterForm;

class CrudWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CrudWidget(Database &database, const TableSpec &spec,
                        const AuthSession &session, const QString &title,
                        QWidget *parent = nullptr);

    void refreshGrid();

private slots:
    void onGridSelectionChanged();

private:
    enum class Operation {
        Idle,
        Adding,
        Modifying
    };

    QString displayValue(const ColumnSpec &column, const QVariant &value) const;
    QString stateText(const QString &state) const;
    QVariantMap currentRecord() const;
    QVariant currentPk() const;
    bool hasColumn(const QString &columnName) const;

    void startAdd();
    void startModify();
    void applyState(const QString &newState);
    void updateRecord();
    void cancelOperation();
    void updateCommandState();

    Database &m_database;
    const TableSpec &m_spec;
    AuthSession m_session;
    QString m_title;
    TableGateway m_gateway;

    RegisterForm *m_form = nullptr;
    QTableView *m_grid = nullptr;
    QStandardItemModel *m_gridModel = nullptr;
    QList<int> m_gridColumns;
    QList<QVariantMap> m_records;
    int m_selectedRow = -1;
    bool m_refreshing = false;
    Operation m_operation = Operation::Idle;

    QPushButton *m_addButton = nullptr;
    QPushButton *m_modifyButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_inactivateButton = nullptr;
    QPushButton *m_reactivateButton = nullptr;
    QPushButton *m_updateButton = nullptr;
    QPushButton *m_cancelButton = nullptr;
};
