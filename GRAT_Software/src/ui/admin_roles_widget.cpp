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
#include "core/errors/error_handler.h"
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

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName(QStringLiteral("rolesErrorLabel"));
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setStyleSheet(QStringLiteral("color: #b00020; font-weight: 600;"));
    m_errorLabel->hide();
    layout->addWidget(m_errorLabel);

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
    loadRoles();

    m_detail = new MasterDetailWidget(database, spec.children.front(), m_session, splitter);

    auto *catalog = new QWidget(splitter);
    auto *catalogLayout = new QVBoxLayout(catalog);
    catalogLayout->setContentsMargins(0, 0, 0, 0);
    m_catalogSummary = new QLabel(catalog);
    m_catalogSummary->setObjectName(QStringLiteral("permissionCatalogSummary"));
    m_catalogSummary->setWordWrap(true);
    catalogLayout->addWidget(m_catalogSummary);
    m_catalogGrid = new QTableView(catalog);
    m_catalogGrid->setObjectName(QStringLiteral("permissionCatalogGrid"));
    m_catalogModel = new QStandardItemModel(this);
    m_catalogModel->setHorizontalHeaderLabels(
        {tr("ID"), tr("Name"), tr("App Label"), tr("Model"), tr("Codename"), tr("Assignment")});
    m_catalogGrid->setModel(m_catalogModel);
    m_catalogGrid->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_catalogGrid->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_catalogGrid->setSelectionMode(QAbstractItemView::SingleSelection);
    m_catalogGrid->setAlternatingRowColors(true);
    m_catalogGrid->verticalHeader()->hide();
    m_catalogGrid->horizontalHeader()->setStretchLastSection(true);
    catalogLayout->addWidget(m_catalogGrid);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    splitter->setStretchFactor(2, 2);
    layout->addWidget(splitter, 1);
    connect(m_rolesGrid->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AdminRolesWidget::selectRole);
    connect(m_detail, &MasterDetailWidget::permissionsChanged,
            this, &AdminRolesWidget::refreshCatalog);
    refreshCatalog();
}

void AdminRolesWidget::loadRoles()
{
    if (!PermissionChecker::canManageRoles(m_session) || !m_rolesGrid)
        return;

    QVariant selectedKey;
    if (m_detail)
        selectedKey = m_detail->parentKey();
    if (!selectedKey.isValid() && m_rolesGrid->selectionModel()) {
        const QModelIndexList selected = m_rolesGrid->selectionModel()->selectedRows();
        if (!selected.isEmpty())
            selectedKey = selected.first().data(Qt::UserRole);
    }

    m_rolesModel->removeRows(0, m_rolesModel->rowCount());

    const TableSpec *spec = findTableSpec(QStringLiteral("auth_group"));
    if (!spec)
        return;

    QString roleError;
    const auto roles = m_gateway.selectAll(*spec, &roleError);
    m_rolesGrid->setEnabled(roleError.isEmpty());
    if (!roleError.isEmpty()) {
        m_errorLabel->setText(roleError);
        m_errorLabel->show();
    } else {
        m_errorLabel->clear();
        m_errorLabel->hide();
    }

    int rowToSelect = -1;
    for (const QVariantMap &role : roles) {
        const QVariant roleId = role.value(QStringLiteral("id"));
        auto *id = new QStandardItem(roleId.toString());
        id->setData(roleId, Qt::UserRole);
        auto *name = new QStandardItem(role.value(QStringLiteral("name")).toString());
        id->setEditable(false);
        name->setEditable(false);
        m_rolesModel->appendRow({id, name});
        if (selectedKey.isValid() && roleId.toString() == selectedKey.toString())
            rowToSelect = m_rolesModel->rowCount() - 1;
    }

    if (rowToSelect >= 0)
        m_rolesGrid->selectRow(rowToSelect);
    else if (m_detail)
        m_detail->setParentKey(QVariant());
}

void AdminRolesWidget::refreshAfterReconnect()
{
    if (!PermissionChecker::canManageRoles(m_session))
        return;

    loadRoles();
    refreshCatalog();
    if (m_detail)
        m_detail->refreshAfterReconnect();
}

void AdminRolesWidget::selectRole()
{
    if (!PermissionChecker::canManageRoles(m_session))
        return;

    const QModelIndexList rows = m_rolesGrid->selectionModel()->selectedRows();
    m_detail->setParentKey(rows.isEmpty() ? QVariant()
                                        : rows.first().data(Qt::UserRole));
    refreshCatalog();
}

void AdminRolesWidget::refreshCatalog()
{
    if (!PermissionChecker::canManageRoles(m_session) || !m_catalogModel || !m_detail)
        return;

    m_catalogModel->removeRows(0, m_catalogModel->rowCount());
    const QVariant roleKey = m_detail->parentKey();
    QString error;
    const auto permissions = m_gateway.rolePermissionCatalog(roleKey, &error);
    if (!error.isEmpty()) {
        m_catalogSummary->setText(error);
        return;
    }

    int assignedCount = 0;
    const QStringList columns = {QStringLiteral("id"), QStringLiteral("name"),
                                 QStringLiteral("app_label"), QStringLiteral("model"),
                                 QStringLiteral("codename")};
    for (const QVariantMap &permission : permissions) {
        QList<QStandardItem *> items;
        for (const QString &column : columns)
            items.append(new QStandardItem(permission.value(column).toString()));
        const bool assigned = permission.value(QStringLiteral("assigned")).toBool();
        assignedCount += assigned ? 1 : 0;
        items.append(new QStandardItem(roleKey.isValid()
            ? (assigned ? tr("Assigned") : tr("Available")) : tr("Select a role")));
        for (QStandardItem *item : items)
            item->setEditable(false);
        m_catalogModel->appendRow(items);
    }
    m_catalogSummary->setText(roleKey.isValid()
        ? tr("System permissions (read-only): %1 — Assigned: %2 — Available: %3")
              .arg(permissions.size()).arg(assignedCount).arg(permissions.size() - assignedCount)
        : tr("System permissions (read-only): %1. Select a role to see assignments.")
              .arg(permissions.size()));
    m_catalogGrid->resizeColumnsToContents();
}
