// Admin role selection and the selected role's permission links (Task 13).
#pragma once

#include <QWidget>

#include "app/session.h"
#include "core/crud/table_gateway.h"

class Database;
class MasterDetailWidget;
class QTableView;
class QStandardItemModel;

class AdminRolesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AdminRolesWidget(Database &database, const AuthSession &session,
                              QWidget *parent = nullptr);

private:
    void selectRole();

    AuthSession m_session;
    TableGateway m_gateway;
    QTableView *m_rolesGrid = nullptr;
    QStandardItemModel *m_rolesModel = nullptr;
    MasterDetailWidget *m_detail = nullptr;
};
