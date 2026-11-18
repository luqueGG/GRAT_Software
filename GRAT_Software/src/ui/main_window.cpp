#include "main_window.h"

#include <QAction>
#include <QFont>
#include <QHash>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSize>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStringList>
#include <QTimer>
#include <QTreeWidget>

#include <utility>

#include "core/crud/table_spec.h"
#include "core/auth/permission_checker.h"
#include "core/db/database.h"
#include "ui/crud_widget.h"
#include "ui/admin_roles_widget.h"
#include "ui/visual_icons.h"
#include "ui/visual_text.h"
#include "ui_main_window.h"

namespace {

struct ModuleDef
{
    const char *key;
    const char *title;
};

const ModuleDef kModules[] = {
    {"location", "Ubicaciones"},
    {"catalog", "Catálogos"},
    {"user", "Usuarios"},
    {"waste", "Residuos"},
    {"transfer", "Transporte"},
    {"admin", "Administración"},
};

} // namespace

MainWindow::MainWindow(Database &database, const AuthSession &session, QWidget *parent)
    : QMainWindow(parent)
    , m_ui(std::make_unique<Ui::MainWindow>())
    , m_database(database)
    , m_session(session)
{
    m_ui->setupUi(this);

    m_ui->navigationTree->setIconSize(QSize(18, 18));

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
    // The queued signal avoids refreshing cached pages re-entrantly while a page
    // is in the middle of an operator command.
    connect(&m_database, &Database::connectionChanged,
            this, &MainWindow::refreshConnectionState, Qt::QueuedConnection);
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
    QMenu *fileMenu = menuBar()->addMenu(tr("&Archivo"));
    QAction *exitAction = fileMenu->addAction(tr("S&alir"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QMenu *modulesMenu = menuBar()->addMenu(tr("&Módulos"));
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

    QMenu *helpMenu = menuBar()->addMenu(tr("&Ayuda"));
    QAction *aboutAction = helpMenu->addAction(tr("&Acerca de GRAT_Software"));
    connect(aboutAction, &QAction::triggered, this, [this] {
        QMessageBox::about(this, tr("Acerca de GRAT_Software"),
                           tr("<b>GRAT_Software</b><br>"
                              "Gestión Responsable de Residuos de Transporte.<br><br>"
                              "Ha iniciado sesión como <b>%1</b>.")
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
        moduleItem->setIcon(0, VisualIcons::moduleIcon(QString::fromLatin1(module.key)));
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
            item->setIcon(0, VisualIcons::moduleIcon(QString::fromLatin1(spec.module)));
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
    statusBar()->showMessage(tr("%1 cargado.").arg(current->text(0)), 2500);
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
        m_dbStatusLabel->setText(tr("● Base de datos conectada"));
        m_dbStatusLabel->setStyleSheet(QStringLiteral("color: #176B45; font-weight: 600;"));
        m_dbStatusLabel->setToolTip(QString());
    } else {
        m_dbStatusLabel->setText(tr("● Base de datos no disponible — mantenimiento deshabilitado"));
        m_dbStatusLabel->setStyleSheet(QStringLiteral("color: #b00020;"));
        m_dbStatusLabel->setToolTip(m_database.lastError());
    }

    m_userLabel->setText(tr("Usuario: %1").arg(m_session.username));
    const QStringList translatedRoles = VisualText::roleNames(m_session.roles);
    const QString roles = translatedRoles.isEmpty()
        ? tr("ninguno")
        : translatedRoles.join(QStringLiteral(", "));
    m_rolesLabel->setText(tr("Rol(es): %1").arg(roles));
}

void MainWindow::refreshConnectionState()
{
    const bool wasConnected = m_connected;

    // A real probe plus a safe recovery attempt: reopening only ever targets the
    // original file and never creates a replacement database.
    m_connected = m_database.ensureAvailable();

    updateStatusBar();
    setMaintenanceEnabled(m_connected);

    if (wasConnected && !m_connected) {
        statusBar()->showMessage(
            tr("Se perdió la conexión con la base de datos. Las acciones de mantenimiento están deshabilitadas."));
    } else if (!wasConnected && m_connected) {
        // Reload cached pages so FK ComboBoxes and grids reflect the live data.
        for (QWidget *page : std::as_const(m_pages)) {
            if (auto *crud = qobject_cast<CrudWidget *>(page))
                crud->refreshAfterReconnect();
            else if (auto *roles = qobject_cast<AdminRolesWidget *>(page))
                roles->refreshAfterReconnect();
        }
        statusBar()->showMessage(tr("Conexión con la base de datos restablecida."), 5000);
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
        {QStringLiteral("location_country"), tr("Países")},
        {QStringLiteral("location_subdivision"), tr("Subdivisiones")},
        {QStringLiteral("catalog_constituent"), tr("Componentes")},
        {QStringLiteral("catalog_containertype"), tr("Tipos de contenedores")},
        {QStringLiteral("catalog_toxicitylevel"), tr("Niveles de toxicidad")},
        {QStringLiteral("catalog_transporttype"), tr("Tipos de transporte")},
        {QStringLiteral("user_user"), tr("Usuarios")},
        {QStringLiteral("waste_generatorcompany"), tr("Empresas generadoras")},
        {QStringLiteral("waste_waste"), tr("Residuos")},
        {QStringLiteral("transfer_destination"), tr("Destinos")},
        {QStringLiteral("transfer_travelcompany"), tr("Empresas transportistas")},
        {QStringLiteral("transfer_transfer"), tr("Traslados")},
        {QStringLiteral("auth_group"), tr("Roles")},
    };
    return titles.value(table, table);
}
