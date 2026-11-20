#include "master_detail_widget.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSize>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStringList>
#include <QTableView>
#include <QVBoxLayout>

#include "core/db/database.h"
#include "core/errors/error_handler.h"
#include "ui/register_form.h"
#include "ui/visual_icons.h"
#include "ui/visual_text.h"

namespace {

// Readable titles for the link tables (design §5.2).
QString childTitle(const QString &table)
{
    static const QHash<QString, QString> titles = {
        {QStringLiteral("waste_wasteconstituent"), QStringLiteral("Constituents")},
        {QStringLiteral("transfer_transfertravelcompany"), QStringLiteral("Tramos de transporte")},
        {QStringLiteral("user_user_groups"), QStringLiteral("Roles")},
        {QStringLiteral("user_user_user_permissions"), QStringLiteral("Permisos adicionales")},
        {QStringLiteral("auth_group_permissions"), QStringLiteral("Permisos")},
    };
    return titles.value(table, table);
}

// Every link table uses an identity 'id' primary key (schema.sql).
const ColumnSpec kChildIdColumn{.dbColumn = "id", .label = "ID",
                                .type = ColumnType::Integer, .editable = false};

} // namespace

MasterDetailWidget::MasterDetailWidget(Database &database, const ChildTableSpec &child,
                                       const AuthSession &session, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
    , m_child(child)
    , m_spec(toTableSpec(child))
    , m_session(session)
    , m_gateway(database.connection())
    , m_repository(m_gateway, m_spec, child)
    , m_validator(m_gateway)
    , m_controller(m_spec, child, session, m_repository,
                   [this](const TableSpec &table, const QVariantMap &values, bool isInsert,
                          const QVariantMap &original) {
                       return m_validator.validate(table, values, isInsert, original);
                   })
{
    setObjectName(QStringLiteral("masterDetail_") + QString::fromLatin1(child.table));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_hintLabel = new QLabel(this);
    m_hintLabel->setObjectName(QStringLiteral("detailHintLabel"));
    m_hintLabel->setWordWrap(true);
    layout->addWidget(m_hintLabel);

    m_grid = new QTableView(this);
    m_grid->setObjectName(QStringLiteral("detailGrid"));
    m_gridModel = new QStandardItemModel(this);
    m_grid->setModel(m_gridModel);
    m_grid->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_grid->setSelectionMode(QAbstractItemView::SingleSelection);
    m_grid->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_grid->setAlternatingRowColors(true);
    m_grid->verticalHeader()->setVisible(false);
    m_grid->horizontalHeader()->setStretchLastSection(true);

    connect(m_grid->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MasterDetailWidget::onSelectionChanged);

    // Grid on the left, inline child form + commands on the right.
    auto *body = new QHBoxLayout();
    body->addWidget(m_grid, 3);

    auto *editor = new QVBoxLayout();
    m_form = new RegisterForm(m_spec, m_gateway, m_session, this);
    editor->addWidget(m_form);
    editor->addStretch();

    auto makeButton = [this](const QString &text, const QString &name, const QString &iconKey) {
        auto *button = new QPushButton(text, this);
        button->setObjectName(name);
        button->setIcon(VisualIcons::icon(iconKey));
        button->setIconSize(QSize(16, 16));
        return button;
    };
    m_addButton = makeButton(tr("Agregar"), QStringLiteral("detailAddButton"), QStringLiteral("add_light"));
    m_editButton = makeButton(tr("Editar"), QStringLiteral("detailEditButton"), QStringLiteral("edit_light"));
    // Rows with a lifecycle are deleted logically; plain link rows are removed.
    m_removeButton = makeButton(m_child.hasState ? tr("Eliminar") : tr("Quitar"),
                                QStringLiteral("detailRemoveButton"), QStringLiteral("delete"));
    m_inactivateButton = makeButton(tr("Inactivate"), QStringLiteral("detailInactivateButton"), QStringLiteral("state"));
    m_reactivateButton = makeButton(tr("Reactivate"), QStringLiteral("detailReactivateButton"), QStringLiteral("state"));
    m_saveButton = makeButton(tr("Guardar"), QStringLiteral("detailSaveButton"), QStringLiteral("refresh_light"));
    m_cancelButton = makeButton(tr("Cancelar"), QStringLiteral("detailCancelButton"), QStringLiteral("cancel"));
    if (isRolePermissions()) {
        m_addButton->setText(tr("Otorgar"));
        m_removeButton->setText(tr("Revocar"));
        m_removeButton->setIcon(VisualIcons::icon(QStringLiteral("revoke")));
        m_editButton->hide();
    }
    m_inactivateButton->setVisible(m_child.hasState);
    m_reactivateButton->setVisible(m_child.hasState);

    connect(m_addButton, &QPushButton::clicked, this, &MasterDetailWidget::startAdd);
    connect(m_editButton, &QPushButton::clicked, this, &MasterDetailWidget::startEdit);
    connect(m_removeButton, &QPushButton::clicked, this, &MasterDetailWidget::removeRow);
    connect(m_inactivateButton, &QPushButton::clicked, this,
            [this] { applyState(QStringLiteral("I")); });
    connect(m_reactivateButton, &QPushButton::clicked, this,
            [this] { applyState(QStringLiteral("A")); });
    connect(m_saveButton, &QPushButton::clicked, this, &MasterDetailWidget::save);
    connect(m_cancelButton, &QPushButton::clicked, this, &MasterDetailWidget::cancel);

    auto *rowCommands = new QHBoxLayout();
    rowCommands->addWidget(m_addButton);
    rowCommands->addWidget(m_editButton);
    rowCommands->addWidget(m_removeButton);
    editor->addLayout(rowCommands);

    auto *stateCommands = new QHBoxLayout();
    stateCommands->addWidget(m_inactivateButton);
    stateCommands->addWidget(m_reactivateButton);
    editor->addLayout(stateCommands);

    auto *confirmCommands = new QHBoxLayout();
    confirmCommands->addStretch();
    confirmCommands->addWidget(m_saveButton);
    confirmCommands->addWidget(m_cancelButton);
    editor->addLayout(confirmCommands);

    body->addLayout(editor, 2);
    layout->addLayout(body, 1);

    // The parent key column is implied by the master record, so it is not repeated
    // in the child grid.
    const QString fkToParent = QString::fromLatin1(m_child.fkToParent);
    QStringList headers;
    for (int i = 0; i < static_cast<int>(m_spec.columns.size()); ++i) {
        const ColumnSpec &column = m_spec.columns[static_cast<size_t>(i)];
        if (fkToParent == QLatin1String(column.dbColumn))
            continue;
        m_gridColumns.append(i);
        headers << QString::fromLatin1(column.label);
    }
    m_gridModel->setHorizontalHeaderLabels(headers);

    setParentKey(QVariant());
}

TableSpec MasterDetailWidget::toTableSpec(const ChildTableSpec &child)
{
    TableSpec spec;
    spec.table = child.table;
    spec.module = nullptr;
    spec.pkColumn = "id";
    spec.appLabel = child.appLabel;
    spec.model = child.model;
    spec.identityPk = true;
    spec.columns.push_back(kChildIdColumn);
    spec.columns.insert(spec.columns.end(), child.columns.begin(), child.columns.end());
    return spec;
}

void MasterDetailWidget::setParentKey(const QVariant &key)
{
    m_controller.setParentKey(key);
    const bool hasParent = m_controller.parentKey().isValid();

    m_hintLabel->setText(hasParent
                             ? (isRolePermissions() ? tr("Permisos asignados al rol seleccionado.")
                                                    : tr("Filas vinculadas al registro seleccionado."))
                             : tr("Seleccione un registro guardado arriba para ver sus %1.")
                                   .arg(title().toLower()));
    setEnabled(hasParent);
    refresh();
}

QVariant MasterDetailWidget::parentKey() const
{
    return m_controller.parentKey();
}

QString MasterDetailWidget::title() const
{
    return childTitle(QString::fromLatin1(m_child.table));
}

void MasterDetailWidget::refresh()
{
    m_refreshing = true;

    m_gridModel->removeRows(0, m_gridModel->rowCount());
    m_records.clear();
    m_selectedRow = -1;

    if (m_controller.parentKey().isValid()) {
        // Keep the contextual hint visible while rows load; replace it only with
        // a safe load error below.
        m_hintLabel->setText(isRolePermissions()
                                 ? tr("Permisos asignados al rol seleccionado.")
                                 : tr("Filas vinculadas al registro seleccionado."));

        loadReferenceLabels();

        // Only the rows belonging to the selected parent, in every state.
        const RecordListResult loaded = m_controller.records();
        m_records = loaded.records;
        if (!loaded.success)
            m_hintLabel->setText(loaded.error);
    }

    for (const QVariantMap &record : m_records) {
        const QString recordState = record.value(QStringLiteral("is_active")).toString();
        const bool inactive = m_child.hasState && recordState == QLatin1String("I");
        const bool deleted = m_child.hasState && recordState == QLatin1String("*");

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

    m_grid->clearSelection();
    m_refreshing = false;

    m_controller.cancel();
    m_form->clear();
    m_form->setMode(RegisterForm::Mode::ReadOnly);
    m_form->reloadReferences();
    updateCommandState();
}

void MasterDetailWidget::refreshAfterReconnect()
{
    if (!m_database.isAlive())
        return;

    if (m_controller.operation() != Operation::Idle) {
        m_form->reloadReferences();
        updateCommandState();
        return;
    }

    refresh();
}

bool MasterDetailWidget::hasPendingEdit() const
{
    return m_controller.operation() != Operation::Idle;
}

bool MasterDetailWidget::ensureDatabaseAvailable()
{
    if (m_database.isAlive())
        return true;

    if (m_database.ensureAvailable() && m_database.isAlive()) {
        m_form->reloadReferences();
        return true;
    }

    m_hintLabel->setText(ErrorHandler::defaultMessage(ErrorHandler::Category::Connection));
    return false;
}

void MasterDetailWidget::onSelectionChanged()
{
    if (m_refreshing)
        return;

    const QModelIndexList selected = m_grid->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        if (m_controller.operation() != Operation::Idle)
            return; // keep the pending child form visible

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

    // Selecting another row abandons a pending Add; during Edit it switches the
    // edited row (same behavior as the parent CrudWidget).
    m_controller.selectRecord(m_records.at(row));

    m_form->setMode(m_controller.operation() == Operation::Modifying ? RegisterForm::Mode::Modifying
                                                      : RegisterForm::Mode::ReadOnly);
    m_form->load(m_records.at(row));
    updateCommandState();
}

QVariantMap MasterDetailWidget::currentRecord() const
{
    if (m_selectedRow < 0 || m_selectedRow >= m_records.size())
        return QVariantMap();
    return m_records.at(m_selectedRow);
}

void MasterDetailWidget::startAdd()
{
    if (m_controller.operation() != Operation::Idle || !ensureDatabaseAvailable())
        return;
    const ControllerResult result = m_controller.startAdd();
    if (!result.success)
        return;
    m_selectedRow = -1;
    m_grid->clearSelection();
    m_form->clear();
    m_form->load({{QString::fromLatin1(m_child.fkToParent), m_controller.parentKey()}});
    m_form->setState(QStringLiteral("A"));
    m_form->setMode(RegisterForm::Mode::Adding);
    updateCommandState();
}

void MasterDetailWidget::startEdit()
{
    if (m_controller.operation() != Operation::Idle || !ensureDatabaseAvailable())
        return;
    const ControllerResult result = m_controller.startModify();
    if (!result.success) {
        QMessageBox::information(this, tr("Editar %1").arg(title()), result.error);
        return;
    }
    m_form->load(currentRecord());
    m_form->setMode(RegisterForm::Mode::Modifying);
    updateCommandState();
}

void MasterDetailWidget::removeRow()
{
    if (m_controller.operation() != Operation::Idle || m_selectedRow < 0
        || !m_controller.parentKey().isValid())
        return;
    if (!ensureDatabaseAvailable())
        return;
    if (!allowed(PermissionAction::Change))
        return;

    if (m_child.hasState) {
        applyState(QStringLiteral("*")); // logical delete (Req 4.1)
        return;
    }

    const auto answer = QMessageBox::question(
        this, isRolePermissions() ? tr("Revocar permiso") : tr("Quitar %1").arg(title()),
        isRolePermissions() ? tr("Revocar el permiso seleccionado de este rol?")
                            : tr("Quitar la fila seleccionada? Este vinculo se eliminara permanentemente."));
    if (answer != QMessageBox::Yes || !allowed(PermissionAction::Change))
        return;

    const ControllerResult result = m_controller.removeSelected();
    if (!result.success) {
        QMessageBox::critical(this, tr("Operacion fallida"), result.error);
        return;
    }
    refresh();
    if (isRolePermissions())
        emit permissionsChanged();
}

void MasterDetailWidget::applyState(const QString &newState)
{
    if (m_controller.operation() != Operation::Idle || m_selectedRow < 0 || !m_child.hasState)
        return;
    if (!ensureDatabaseAvailable())
        return;
    if (!allowed(PermissionAction::Change))
        return;

    const ControllerResult result = m_controller.changeState(newState);
    if (!result.success) {
        QMessageBox::critical(this, tr("Operacion fallida"), result.error);
        return;
    }
    refresh();
}

void MasterDetailWidget::save()
{
    if (!m_controller.parentKey().isValid() || !ensureDatabaseAvailable())
        return;
    if (m_controller.operation() == Operation::Idle) {
        QMessageBox::information(this, tr("Guardar %1").arg(title()), m_controller.save({}).error);
        return;
    }
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
        QMessageBox::critical(this, tr("Guardado fallido"), result.error);
        return;
    }
    refresh();
    if (isRolePermissions())
        emit permissionsChanged();
}

void MasterDetailWidget::cancel()
{
    if (m_controller.operation() == Operation::Idle)
        return;

    m_controller.cancel();
    m_selectedRow = -1;
    m_grid->clearSelection();
    m_form->clear();
    m_form->setMode(RegisterForm::Mode::ReadOnly);
    updateCommandState();
}

bool MasterDetailWidget::allowed(PermissionAction action) const
{
    return m_database.isAlive() && m_controller.can(action);
}

bool MasterDetailWidget::isRolePermissions() const
{
    return m_controller.isRolePermissions();
}

void MasterDetailWidget::updateCommandState()
{
    const bool idle = m_controller.operation() == Operation::Idle;
    const bool hasParent = m_controller.parentKey().isValid();
    const bool hasSelection = m_selectedRow >= 0 && m_selectedRow < m_records.size();
    const bool canAdd = allowed(PermissionAction::Add);
    const bool canChange = allowed(PermissionAction::Change);

    m_addButton->setEnabled(idle && hasParent && canAdd);
    m_editButton->setEnabled(idle && hasSelection && m_database.isAlive() && m_controller.canEdit());
    m_removeButton->setEnabled(idle && hasSelection && canChange);
    m_inactivateButton->setEnabled(idle && hasSelection && canChange);
    m_reactivateButton->setEnabled(idle && hasSelection && canChange);
    m_saveButton->setEnabled(!idle && (m_controller.operation() == Operation::Adding ? canAdd : canChange));
    m_cancelButton->setEnabled(!idle);
}

void MasterDetailWidget::loadReferenceLabels()
{
    m_refLabels.clear();
    for (const ColumnSpec &column : m_spec.columns) {
        if (column.type != ColumnType::Fk || !column.refTable || !column.refPk)
            continue;

        const QString refTable = QString::fromLatin1(column.refTable);
        const QString refDisplay = column.refDisplay
            ? QString::fromLatin1(column.refDisplay)
            : QString();
        const QList<RefValue> values = m_gateway.loadRefValues(
            refTable, QString::fromLatin1(column.refPk), refDisplay);

        QHash<QString, QString> labels;
        for (const RefValue &value : values)
            labels.insert(value.pk.toString(),
                          VisualText::referenceLabel(refTable, refDisplay, value.text));
        m_refLabels.insert(QString::fromLatin1(column.dbColumn), labels);
    }
}

QString MasterDetailWidget::displayValue(const ColumnSpec &column, const QVariant &value) const
{
    switch (column.type) {
    case ColumnType::State:
        return stateText(value.toString());
    case ColumnType::Decimal:
        return QString::number(value.toDouble(), 'f', 2);
    case ColumnType::Fk: {
        // Show the referenced record's label instead of the raw key (Req 10.2).
        const QString key = value.toString();
        return m_refLabels.value(QString::fromLatin1(column.dbColumn)).value(key, key);
    }
    default:
        return value.toString();
    }
}

QString MasterDetailWidget::stateText(const QString &state) const
{
    if (state == QLatin1String("A"))
        return tr("Activo");
    if (state == QLatin1String("I"))
        return tr("Inactivo");
    if (state == QLatin1String("*"))
        return tr("Eliminado");
    return state;
}
