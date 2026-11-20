#include "crud_widget.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QDateTime>
#include <QFont>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSize>
#include <QSplitter>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStringList>
#include <QTabWidget>
#include <QTableView>
#include <QVBoxLayout>

#include "core/auth/permission_checker.h"
#include "core/db/database.h"
#include "core/errors/error_handler.h"
#include "ui/master_detail_widget.h"
#include "ui/register_form.h"
#include "ui/visual_icons.h"

CrudWidget::CrudWidget(Database &database, const TableSpec &spec,
                       const AuthSession &session, const QString &title,
                       QWidget *parent)
    : QWidget(parent)
    , m_database(database)
    , m_spec(spec)
    , m_session(session)
    , m_title(title)
    , m_gateway(database.connection())
    , m_repository(m_gateway, spec)
    , m_validator(m_gateway)
    , m_controller(spec, session, m_repository,
                   [this](const TableSpec &table, const QVariantMap &values, bool isInsert,
                          const QVariantMap &original) {
                       return m_validator.validate(table, values, isInsert, original);
                   })
{
    setObjectName(QStringLiteral("crudWidget_") + QString::fromLatin1(spec.table));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto *heading = new QLabel(m_title, this);
    heading->setObjectName(QStringLiteral("sectionHeading"));
    QFont headingFont = heading->font();
    headingFont.setPointSize(18);
    headingFont.setBold(true);
    heading->setFont(headingFont);
    mainLayout->addWidget(heading);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName(QStringLiteral("operationErrorLabel"));
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setStyleSheet(QStringLiteral("color: #b00020; font-weight: 600;"));
    m_errorLabel->hide();
    mainLayout->addWidget(m_errorLabel);

    auto *splitter = new QSplitter(Qt::Vertical, this);

    m_grid = new QTableView(splitter);
    m_grid->setObjectName(QStringLiteral("grid"));
    m_gridModel = new QStandardItemModel(this);
    m_grid->setModel(m_gridModel);
    m_grid->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_grid->setSelectionMode(QAbstractItemView::SingleSelection);
    m_grid->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_grid->setAlternatingRowColors(true);
    m_grid->verticalHeader()->setVisible(false);
    m_grid->horizontalHeader()->setStretchLastSection(true);

    connect(m_grid->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &CrudWidget::onGridSelectionChanged);

    auto *lowerPanel = new QWidget(splitter);
    auto *lowerLayout = new QVBoxLayout(lowerPanel);
    lowerLayout->setContentsMargins(0, 0, 0, 0);

    m_form = new RegisterForm(m_spec, m_gateway, m_session, lowerPanel);
    auto *formScroll = new QScrollArea(lowerPanel);
    formScroll->setObjectName(QStringLiteral("formScroll"));
    formScroll->setWidgetResizable(true);
    formScroll->setFrameShape(QFrame::NoFrame);
    formScroll->setWidget(m_form);
    lowerLayout->addWidget(formScroll, 1);

    auto *buttonLayout = new QHBoxLayout();
    auto makeButton = [this](const QString &text, const QString &name, const QString &iconKey) {
        auto *button = new QPushButton(text, this);
        button->setObjectName(name);
        button->setIcon(VisualIcons::icon(iconKey));
        button->setIconSize(QSize(16, 16));
        return button;
    };
    m_addButton = makeButton(tr("Agregar"), QStringLiteral("addButton"), QStringLiteral("add_light"));
    m_modifyButton = makeButton(tr("Modificar"), QStringLiteral("modifyButton"), QStringLiteral("edit_light"));
    m_deleteButton = makeButton(tr("Eliminar"), QStringLiteral("deleteButton"), QStringLiteral("delete"));
    m_inactivateButton = makeButton(tr("Desactivar"), QStringLiteral("inactivateButton"), QStringLiteral("state"));
    m_reactivateButton = makeButton(tr("Reactivar"), QStringLiteral("reactivateButton"), QStringLiteral("state"));
    m_updateButton = makeButton(tr("Actualizar"), QStringLiteral("updateButton"), QStringLiteral("refresh_light"));
    m_cancelButton = makeButton(tr("Cancelar"), QStringLiteral("cancelButton"), QStringLiteral("cancel"));

    connect(m_addButton, &QPushButton::clicked, this, &CrudWidget::startAdd);
    connect(m_modifyButton, &QPushButton::clicked, this, &CrudWidget::startModify);
    connect(m_deleteButton, &QPushButton::clicked, this,
            [this] { applyState(QStringLiteral("*")); });
    connect(m_inactivateButton, &QPushButton::clicked, this,
            [this] { applyState(QStringLiteral("I")); });
    connect(m_reactivateButton, &QPushButton::clicked, this,
            [this] { applyState(QStringLiteral("A")); });
    connect(m_updateButton, &QPushButton::clicked, this, &CrudWidget::updateRecord);
    connect(m_cancelButton, &QPushButton::clicked, this, &CrudWidget::cancelOperation);

    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_modifyButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addWidget(m_inactivateButton);
    buttonLayout->addWidget(m_reactivateButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_updateButton);
    buttonLayout->addWidget(m_cancelButton);
    lowerLayout->addLayout(buttonLayout);

    splitter->addWidget(m_grid);
    splitter->addWidget(lowerPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    // Master-detail (design §5.2 / §7.4): the selected record's child rows are
    // listed below the form, one tab per link table.
    if (!m_spec.children.empty()) {
        m_detailTabs = new QTabWidget(splitter);
        m_detailTabs->setObjectName(QStringLiteral("detailTabs"));
        for (const ChildTableSpec &child : m_spec.children) {
            auto *detail = new MasterDetailWidget(m_database, child, m_session, m_detailTabs);
            m_detailTabs->addTab(detail, detail->title());
            m_details.append(detail);
        }
        splitter->addWidget(m_detailTabs);
        splitter->setStretchFactor(2, 2);
    }

    mainLayout->addWidget(splitter, 1);

    refreshGrid();
}

void CrudWidget::refreshGrid()
{
    m_refreshing = true;

    m_gridColumns.clear();
    QStringList headers;
    for (int i = 0; i < static_cast<int>(m_spec.columns.size()); ++i) {
        const ColumnSpec &column = m_spec.columns[static_cast<size_t>(i)];
        if (column.type == ColumnType::Password)
            continue;
        m_gridColumns.append(i);
        headers << QString::fromLatin1(column.label);
    }

    m_gridModel->clear();
    m_gridModel->setColumnCount(headers.size());
    m_gridModel->setHorizontalHeaderLabels(headers);

    const RecordListResult loaded = m_controller.records();
    m_records = loaded.records;
    if (loaded.success)
        clearError();
    else
        showError(loaded.error);

    for (const QVariantMap &record : m_records) {
        const QString recordState = record.value(QStringLiteral("is_active")).toString();
        const bool inactive = recordState == QLatin1String("I");
        const bool deleted = recordState == QLatin1String("*");

        QList<QStandardItem *> items;
        items.reserve(m_gridColumns.size());

        for (int specIndex : m_gridColumns) {
            const ColumnSpec &column = m_spec.columns[static_cast<size_t>(specIndex)];
            auto *item = new QStandardItem(
                displayValue(column, record.value(QString::fromLatin1(column.dbColumn))));
            item->setEditable(false);

            if (column.isState) {
                QFont stateFont = item->font();
                stateFont.setBold(true);
                item->setFont(stateFont);
            }

            if (deleted) {
                item->setForeground(QBrush(QColor(QStringLiteral("#b00020"))));
                QFont font = item->font();
                font.setStrikeOut(true);
                item->setFont(font);
            } else if (inactive) {
                item->setForeground(QBrush(QColor(QStringLiteral("#777777"))));
            }

            items.append(item);
        }

        m_gridModel->appendRow(items);
    }

    m_selectedRow = -1;
    m_grid->clearSelection();
    m_refreshing = false;

    // Refreshing always starts from a clean, read-only form (Req 2.5/5.3).
    m_controller.cancel();
    m_form->clear();
    m_form->setMode(RegisterForm::Mode::ReadOnly);
    m_form->reloadReferences();
    updateCommandState();
}

void CrudWidget::refreshAfterReconnect()
{
    if (!m_database.isAlive())
        return;

    clearError();

    bool childPending = false;
    for (MasterDetailWidget *detail : m_details) {
        if (detail->hasPendingEdit()) {
            childPending = true;
            break;
        }
    }

    if (m_controller.operation() != Operation::Idle || childPending) {
        // Never discard an operator's unsaved form because the timer noticed the
        // recovery; update the reference lists and command state in place.
        m_form->reloadReferences();
        for (MasterDetailWidget *detail : m_details) {
            if (detail->hasPendingEdit())
                detail->refreshAfterReconnect();
        }
        updateCommandState();
        return;
    }
    refreshGrid();
}

bool CrudWidget::ensureDatabaseAvailable()
{
    if (m_database.isAlive())
        return true;

    if (m_database.ensureAvailable() && m_database.isAlive()) {
        clearError();
        m_form->reloadReferences();
        return true;
    }

    showError(ErrorHandler::defaultMessage(ErrorHandler::Category::Connection));
    return false;
}

void CrudWidget::showError(const QString &message)
{
    if (!m_errorLabel)
        return;
    m_errorLabel->setText(message);
    m_errorLabel->show();
}

void CrudWidget::clearError()
{
    if (!m_errorLabel)
        return;
    m_errorLabel->clear();
    m_errorLabel->hide();
}

void CrudWidget::onGridSelectionChanged()
{
    if (m_refreshing)
        return;

    const QModelIndexList selected = m_grid->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        if (m_controller.operation() != Operation::Idle)
            return; // keep the pending Add/Modify form visible

        m_selectedRow = -1;
        m_controller.selectRecord({});
        m_form->clear();
        m_form->setMode(RegisterForm::Mode::ReadOnly);
        updateCommandState();
        return;
    }

    const int row = selected.first().row();
    if (row < 0 || row >= m_records.size())
        return;

    m_selectedRow = row;

    // Selecting another row replaces the displayed record (Req 2.4). During Add
    // this abandons the pending new record; during Modify it keeps Modify mode.
    m_controller.selectRecord(m_records.at(row));

    if (m_controller.operation() == Operation::Modifying) {
        m_form->setMode(RegisterForm::Mode::Modifying);
        m_form->load(m_records.at(row));
    } else {
        m_form->setMode(RegisterForm::Mode::ReadOnly);
        m_form->load(m_records.at(row));
    }

    updateCommandState();
}

QVariantMap CrudWidget::currentRecord() const
{
    if (m_selectedRow < 0 || m_selectedRow >= m_records.size())
        return QVariantMap();
    return m_records.at(m_selectedRow);
}

QVariant CrudWidget::currentPk() const
{
    return currentRecord().value(QString::fromLatin1(m_spec.pkColumn));
}

void CrudWidget::startAdd()
{
    if (m_controller.operation() != Operation::Idle || !ensureDatabaseAvailable())
        return;
    const ControllerResult result = m_controller.startAdd();
    if (!result.success) {
        QMessageBox::information(this, tr("Agregar registro"), result.error);
        return;
    }
    m_selectedRow = -1;
    m_grid->clearSelection();
    m_form->clear();
    m_form->setState(QStringLiteral("A"));
    m_form->setMode(RegisterForm::Mode::Adding);
    updateCommandState();
}

void CrudWidget::startModify()
{
    if (m_controller.operation() != Operation::Idle || !ensureDatabaseAvailable())
        return;
    const ControllerResult result = m_controller.startModify();
    if (!result.success) {
        QMessageBox::information(this, tr("Modificar registro"), result.error);
        return;
    }
    m_form->load(currentRecord());
    m_form->setMode(RegisterForm::Mode::Modifying);
    updateCommandState();
}

void CrudWidget::applyState(const QString &newState)
{
    if (m_controller.operation() != Operation::Idle || !ensureDatabaseAvailable())
        return;
    const ControllerResult result = m_controller.changeState(newState);
    if (!result.success) {
        QMessageBox::critical(this, tr("Operacion fallida"), result.error);
        return;
    }
    refreshGrid();
}

void CrudWidget::updateRecord()
{
    if (m_controller.operation() == Operation::Idle) {
        QMessageBox::information(this, tr("Actualizar registro"), m_controller.save({}).error);
        return;
    }
    if (!ensureDatabaseAvailable())
        return;
    const ControllerResult result = m_controller.save(m_form->collect());
    if (!result.validationErrors.isEmpty()) {
        QStringList details;
        for (const ValidationError &error : result.validationErrors)
            details << QStringLiteral("- %1").arg(error.reason);
        QMessageBox::warning(this, tr("Validation failed"),
                             tr("Corrija los siguientes campos:\n\n%1")
                                 .arg(details.join(QLatin1Char('\n'))));
        return;
    }
    if (!result.success) {
        QMessageBox::critical(this, tr("Actualizacion fallida"), result.error);
        return;
    }
    refreshGrid();
}

void CrudWidget::cancelOperation()
{
    if (m_controller.operation() == Operation::Idle)
        return;

    m_controller.cancel();
    m_selectedRow = -1;
    m_grid->clearSelection();
    m_form->clear();
    m_form->setState(QStringLiteral("A"));
    m_form->setMode(RegisterForm::Mode::ReadOnly);
    updateCommandState();
}

void CrudWidget::updateCommandState()
{
    const bool idle = m_controller.operation() == Operation::Idle;
    const bool hasSelection = m_selectedRow >= 0 && m_selectedRow < m_records.size();
    const bool dbAvailable = m_database.isAlive();

    const bool canAdd = m_controller.can(PermissionAction::Add);
    const bool canChange = m_controller.canEdit();

    m_addButton->setEnabled(idle && canAdd && dbAvailable);
    m_modifyButton->setEnabled(idle && hasSelection && canChange && dbAvailable);
    m_deleteButton->setEnabled(idle && hasSelection && canChange && dbAvailable);
    m_inactivateButton->setEnabled(idle && hasSelection && canChange && dbAvailable);
    m_reactivateButton->setEnabled(idle && hasSelection && canChange && dbAvailable);
    // Update stays clickable in Idle so the operator gets the Req 5.2 message,
    // but only when they may change the table and the database is available.
    m_updateButton->setEnabled((m_controller.operation() == Operation::Adding ? canAdd : canChange)
                               && dbAvailable);
    m_cancelButton->setEnabled(!idle);

    updateDetails();
}

void CrudWidget::updateDetails()
{
    if (m_details.isEmpty())
        return;

    // Children belong to a saved parent: none while adding or with no selection.
    const bool hasSelection = m_selectedRow >= 0 && m_selectedRow < m_records.size();
    const QVariant parentKey = (hasSelection && m_controller.operation() != Operation::Adding)
        ? currentPk()
        : QVariant();

    for (MasterDetailWidget *detail : m_details) {
        // Only reload when the parent actually changes, so an unsaved child edit
        // survives unrelated command-state updates.
        if (detail->parentKey().toString() != parentKey.toString()
            || detail->parentKey().isValid() != parentKey.isValid()) {
            detail->setParentKey(parentKey);
        }
    }
}

QString CrudWidget::displayValue(const ColumnSpec &column, const QVariant &value) const
{
    switch (column.type) {
    case ColumnType::State:
        return stateText(value.toString());
    case ColumnType::Boolean:
        return value.toInt() != 0 ? tr("Si") : tr("No");
    case ColumnType::Decimal:
        return QString::number(value.toDouble(), 'f', 2);
    case ColumnType::DateTime: {
        QString text = value.toString();
        text.replace(QLatin1Char('T'), QLatin1Char(' '));
        return text;
    }
    case ColumnType::Password:
        return QStringLiteral("********");
    default:
        return value.toString();
    }
}

QString CrudWidget::stateText(const QString &state) const
{
    if (state == QLatin1String("A"))
        return tr("Activo");
    if (state == QLatin1String("I"))
        return tr("Inactivo");
    if (state == QLatin1String("*"))
        return tr("Eliminado");
    return state;
}
