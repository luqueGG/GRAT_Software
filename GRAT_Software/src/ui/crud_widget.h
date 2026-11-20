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
#include "core/crud/repositories/sql_record_repository.h"
#include "core/crud/controllers/crud_controller.h"

class Database;
class MasterDetailWidget;
class QLabel;
class QPushButton;
class QStandardItemModel;
class QTableView;
class QTabWidget;
class RegisterForm;

class CrudWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CrudWidget(Database &database, const TableSpec &spec,
                        const AuthSession &session, const QString &title,
                        QWidget *parent = nullptr);

    void refreshGrid();

    // Re-verifies the database, reloads FK lists and refreshes the grid after a
    // connection recovery (Req 8.4, Req 10.4).
    void refreshAfterReconnect();

private slots:
    void onGridSelectionChanged();

private:
    using Operation = CrudController::Operation;

    QString displayValue(const ColumnSpec &column, const QVariant &value) const;
    QString stateText(const QString &state) const;
    QVariantMap currentRecord() const;
    QVariant currentPk() const;

    void startAdd();
    void startModify();
    void applyState(const QString &newState);
    void updateRecord();
    void cancelOperation();
    void updateCommandState();
    void updateDetails();
    bool ensureDatabaseAvailable();
    void showError(const QString &message);
    void clearError();

    Database &m_database;
    const TableSpec &m_spec;
    AuthSession m_session;
    QString m_title;
    TableGateway m_gateway;
    SqlRecordRepository m_repository;
    Validator m_validator;
    CrudController m_controller;

    RegisterForm *m_form = nullptr;
    QLabel *m_errorLabel = nullptr;
    QTableView *m_grid = nullptr;
    QStandardItemModel *m_gridModel = nullptr;
    QList<int> m_gridColumns;
    QList<QVariantMap> m_records;
    int m_selectedRow = -1;
    bool m_refreshing = false;

    QPushButton *m_addButton = nullptr;
    QPushButton *m_modifyButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_inactivateButton = nullptr;
    QPushButton *m_reactivateButton = nullptr;
    QPushButton *m_updateButton = nullptr;
    QPushButton *m_cancelButton = nullptr;

    // Master-detail children (design §7.4): one tab per ChildTableSpec.
    QTabWidget *m_detailTabs = nullptr;
    QList<MasterDetailWidget *> m_details;
};
