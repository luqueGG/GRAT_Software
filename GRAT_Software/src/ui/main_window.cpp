#include "main_window.h"

#include <QAction>
#include <QFont>
#include <QHash>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTimer>
#include <QTreeWidget>

#include "core/crud/table_spec.h"
#include "core/auth/permission_checker.h"
#include "core/db/database.h"
#include "ui/crud_widget.h"
#include "ui/admin_roles_widget.h"
#include "ui_main_window.h"

namespace {

struct ModuleDef
{
    const char *key;
    const char *title;
};

const ModuleDef kModules[] = {
    {"location", "Location"},
    {"catalog", "Catalog"},
    {"user", "User"},
    {"waste", "Waste"},
    {"transfer", "Transfer"},
    {"admin", "Admin"},
};

} // namespace

MainWindow::MainWindow(Database &database, const AuthSession &session, QWidget *parent)
    : QMainWindow(parent)
    , m_ui(std::make_unique<Ui::MainWindow>())
    , m_database(database)
    , m_session(session)
{
    m_ui->setupUi(this);

    m_dbStatusLabel = new QLabel(this);
    m_userLabel = new QLabel(this);
    m_rolesLabel = new QLabel(this);
    m_dbStatusLabel->setObjectName(QStringLiteral("dbStatusLabel"));
    m_userLabel->setObjectName(QStringLiteral("userLabel"));
    m_rolesLabel->setObjectName(QStringLiteral("rolesLabel"));
    statusBar()->addWidget(m_dbStatusLabel);
    statusBar()->addPermanentWidget(m_userLabel);
    statusBar()->addPermanentWidget(m_rolesLabel);

    buildNavigation();
    connect(m_ui->navigationTree, &QTreeWidget::currentItemChanged,
            this, &MainWindow::showPage);
    buildMenus();
    updateStatusBar();

    // Observe database health: the Database signal covers lifecycle changes and
    // the timer catches loss/restoration of the live connection (Req 8.3–8.4).
    connect(&m_database, &Database::connectionChanged,
            this, &MainWindow::refreshConnectionState);
    m_connectionTimer = new QTimer(this);
    m_connectionTimer->setInterval(3000);
    connect(m_connectionTimer, &QTimer::timeout,
            this, &MainWindow::refreshConnectionState);
    m_connectionTimer->start();
    refreshConnectionState();

    setWindowTitle(QStringLiteral("GRAT_Software — %1").arg(m_session.username));

    // Open the first permitted table so the workspace is never blank.
    if (m_ui->navigationTree->topLevelItemCount() > 0) {
        QTreeWidgetItem *firstModule = m_ui->navigationTree->topLevelItem(0);
        if (firstModule->childCount() > 0)
            m_ui->navigationTree->setCurrentItem(firstModule->child(0));
    }
}

MainWindow::~MainWindow() = default;

void MainWindow::buildMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *exitAction = fileMenu->addAction(tr("E&xit"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QMenu *modulesMenu = menuBar()->addMenu(tr("&Modules"));
    for (const ModuleDef &module : kModules) {
        QMenu *moduleMenu = nullptr;
        for (const TableSpec &spec : allTableSpecs()) {
            if (QString::fromLatin1(spec.module) != QLatin1String(module.key))
                continue;
            if (!canView(spec))
                continue;

            if (!moduleMenu)
                moduleMenu = modulesMenu->addMenu(moduleTitle(QString::fromLatin1(module.key)));

            const QString table = QString::fromLatin1(spec.table);
            QAction *action = moduleMenu->addAction(tableTitle(table));
            connect(action, &QAction::triggered, this,
                    [this, table] { navigateToTable(table); });
        }
    }

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction *aboutAction = helpMenu->addAction(tr("&About GRAT_Software"));
    connect(aboutAction, &QAction::triggered, this, [this] {
        QMessageBox::about(this, tr("About GRAT_Software"),
                           tr("<b>GRAT_Software</b><br>"
                              "Responsible Management of Transport Waste.<br><br>"
                              "Signed in as <b>%1</b>.")
                               .arg(m_session.username.toHtmlEscaped()));
    });
}

void MainWindow::buildNavigation()
{
    m_ui->navigationTree->clear();
    m_navigationItems.clear();

    for (const ModuleDef &module : kModules) {
        auto *moduleItem = new QTreeWidgetItem(m_ui->navigationTree);
        moduleItem->setText(0, moduleTitle(QString::fromLatin1(module.key)));
        moduleItem->setFlags(moduleItem->flags() & ~Qt::ItemIsSelectable);
        QFont moduleFont = moduleItem->font(0);
        moduleFont.setBold(true);
        moduleItem->setFont(0, moduleFont);

        int visibleTables = 0;
        for (const TableSpec &spec : allTableSpecs()) {
            if (QString::fromLatin1(spec.module) != QLatin1String(module.key))
                continue;
            if (!canView(spec))
                continue;

            auto *item = new QTreeWidgetItem(moduleItem);
            const QString table = QString::fromLatin1(spec.table);
            item->setText(0, tableTitle(table));
            item->setData(0, Qt::UserRole, table);
            m_navigationItems.insert(table, item);
            ++visibleTables;
        }

        // A module with no viewable table is hidden entirely (Task 9).
        if (visibleTables == 0) {
            delete moduleItem;
            continue;
        }

        moduleItem->setExpanded(true);
    }
}

bool MainWindow::canView(const TableSpec &spec) const
{
    // The Admin → Roles screen is outside the 43-permission matrix and reserved
    // for Administrators/superusers (design §5.3).
    if (QString::fromLatin1(spec.module) == QLatin1String("admin"))
        return PermissionChecker::canManageRoles(m_session);

    return PermissionChecker::can(m_session, PermissionAction::View,
                                  QString::fromLatin1(spec.appLabel),
                                  QString::fromLatin1(spec.model));
}

void MainWindow::showPage(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    Q_UNUSED(previous)

    if (!current)
        return;

    const QString table = current->data(0, Qt::UserRole).toString();
    if (table.isEmpty())
        return;

    const TableSpec *spec = findTableSpec(table);
    if (!spec || !canView(*spec))
        return;

    QWidget *page = m_pages.value(table, nullptr);
    if (!page) {
        page = createPage(*spec);
        m_pages.insert(table, page);
        m_ui->stackedWidget->addWidget(page);
    }

    m_ui->stackedWidget->setCurrentWidget(page);
    statusBar()->showMessage(tr("%1 loaded.").arg(current->text(0)), 2500);
}

QWidget *MainWindow::createPage(const TableSpec &spec)
{
    if (QString::fromLatin1(spec.table) == QLatin1String("auth_group"))
        return new AdminRolesWidget(m_database, m_session, m_ui->stackedWidget);

    return new CrudWidget(m_database, spec, m_session,
                          tableTitle(QString::fromLatin1(spec.table)),
                          m_ui->stackedWidget);
}

void MainWindow::navigateToTable(const QString &table)
{
    QTreeWidgetItem *item = m_navigationItems.value(table, nullptr);
    if (!item)
        return;

    if (QTreeWidgetItem *parent = item->parent())
        parent->setExpanded(true);

    m_ui->navigationTree->setCurrentItem(item);
    m_ui->navigationTree->scrollToItem(item);
    m_ui->navigationTree->setFocus();
}

void MainWindow::updateStatusBar()
{
    if (m_connected) {
        m_dbStatusLabel->setText(tr("● DB connected"));
        m_dbStatusLabel->setStyleSheet(QStringLiteral("color: #1b7f3a;"));
    } else {
        m_dbStatusLabel->setText(tr("● DB unavailable"));
        m_dbStatusLabel->setStyleSheet(QStringLiteral("color: #b00020;"));
    }

    m_userLabel->setText(tr("User: %1").arg(m_session.username));
    const QString roles = m_session.roles.isEmpty()
        ? tr("none")
        : m_session.roles.join(QStringLiteral(", "));
    m_rolesLabel->setText(tr("Role(s): %1").arg(roles));
}

void MainWindow::refreshConnectionState()
{
    const bool wasConnected = m_connected;
    m_connected = m_database.isAlive();

    updateStatusBar();
    setMaintenanceEnabled(m_connected);

    if (wasConnected && !m_connected) {
        statusBar()->showMessage(
            tr("Database connection lost. Maintenance actions are disabled."));
    } else if (!wasConnected && m_connected) {
        statusBar()->showMessage(tr("Database connection restored."), 5000);
    }
}

void MainWindow::setMaintenanceEnabled(bool enabled)
{
    // Every table page (and, from Task 11, its command buttons) is disabled while
    // the database is unavailable, and re-enabled when it returns.
    m_ui->stackedWidget->setEnabled(enabled);
}

QString MainWindow::moduleTitle(const QString &module) const
{
    for (const ModuleDef &def : kModules) {
        if (module == QLatin1String(def.key))
            return tr(def.title);
    }
    return module;
}

QString MainWindow::tableTitle(const QString &table) const
{
    static const QHash<QString, QString> titles = {
        {QStringLiteral("location_country"), tr("Countries")},
        {QStringLiteral("location_subdivision"), tr("Subdivisions")},
        {QStringLiteral("catalog_constituent"), tr("Constituents")},
        {QStringLiteral("catalog_containertype"), tr("Container Types")},
        {QStringLiteral("catalog_toxicitylevel"), tr("Toxicity Levels")},
        {QStringLiteral("catalog_transporttype"), tr("Transport Types")},
        {QStringLiteral("user_user"), tr("Users")},
        {QStringLiteral("waste_generatorcompany"), tr("Generator Companies")},
        {QStringLiteral("waste_waste"), tr("Waste")},
        {QStringLiteral("transfer_destination"), tr("Destinations")},
        {QStringLiteral("transfer_travelcompany"), tr("Travel Companies")},
        {QStringLiteral("transfer_transfer"), tr("Transfers")},
        {QStringLiteral("auth_group"), tr("Roles")},
    };
    return titles.value(table, table);
}
