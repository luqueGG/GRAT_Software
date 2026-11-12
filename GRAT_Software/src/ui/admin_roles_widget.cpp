#include "admin_roles_widget.h"

#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>

#include "core/auth/permission_checker.h"
#include "core/db/database.h"
#include "ui/master_detail_widget.h"

AdminRolesWidget::AdminRolesWidget(Database &database, const AuthSession &session, QWidget *parent)
    : QWidget(parent)
    , m_session(session)
    , m_gateway(database.connection())
{
    setObjectName(QStringLiteral("adminRolesWidget"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto *heading = new QLabel(tr("Roles"), this);
    QFont font = heading->font();
    font.setPointSize(18);
    font.setBold(true);
    heading->setFont(font);
    layout->addWidget(heading);

    if (!PermissionChecker::canManageRoles(m_session)) {
        layout->addWidget(new QLabel(tr("You are not authorized to manage roles."), this));
        return;
    }

    auto *hint = new QLabel(tr("Select a role to manage its permissions. "
                              "Changes take effect for users on their next sign-in."), this);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto *splitter = new QSplitter(Qt::Vertical, this);
    m_rolesGrid = new QTableView(splitter);
    m_rolesGrid->setObjectName(QStringLiteral("rolesGrid"));
    m_rolesModel = new QStandardItemModel(this);
    m_rolesModel->setHorizontalHeaderLabels({tr("ID"), tr("Role Name")});
    m_rolesGrid->setModel(m_rolesModel);
    m_rolesGrid->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_rolesGrid->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_rolesGrid->setSelectionMode(QAbstractItemView::SingleSelection);
    m_rolesGrid->setAlternatingRowColors(true);
    m_rolesGrid->verticalHeader()->hide();
    m_rolesGrid->horizontalHeader()->setStretchLastSection(true);

    const TableSpec &spec = *findTableSpec(QStringLiteral("auth_group"));
    for (const QVariantMap &role : m_gateway.selectAll(spec)) {
        auto *id = new QStandardItem(role.value(QStringLiteral("id")).toString());
        id->setData(role.value(QStringLiteral("id")), Qt::UserRole);
        auto *name = new QStandardItem(role.value(QStringLiteral("name")).toString());
        id->setEditable(false);
        name->setEditable(false);
        m_rolesModel->appendRow({id, name});
    }

    m_detail = new MasterDetailWidget(database, spec.children.front(), m_session, splitter);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    layout->addWidget(splitter, 1);
    connect(m_rolesGrid->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AdminRolesWidget::selectRole);
}

void AdminRolesWidget::selectRole()
{
    if (!PermissionChecker::canManageRoles(m_session))
        return;

    const QModelIndexList rows = m_rolesGrid->selectionModel()->selectedRows();
    m_detail->setParentKey(rows.isEmpty() ? QVariant()
                                        : rows.first().data(Qt::UserRole));
}
