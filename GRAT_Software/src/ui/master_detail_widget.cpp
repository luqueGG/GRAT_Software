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
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStringList>
#include <QTableView>
#include <QVBoxLayout>

#include "core/crud/validator.h"
#include "core/db/database.h"
#include "ui/register_form.h"

namespace {

// Readable titles for the link tables (design §5.2).
QString childTitle(const QString &table)
{
    static const QHash<QString, QString> titles = {
        {QStringLiteral("waste_wasteconstituent"), QStringLiteral("Constituents")},
        {QStringLiteral("transfer_transfertravelcompany"), QStringLiteral("Transport Legs")},
        {QStringLiteral("user_user_groups"), QStringLiteral("Roles")},
        {QStringLiteral("user_user_user_permissions"), QStringLiteral("Extra Permissions")},
        {QStringLiteral("auth_group_permissions"), QStringLiteral("Permissions")},
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

    auto makeButton = [this](const QString &text, const QString &name) {
        auto *button = new QPushButton(text, this);
        button->setObjectName(name);
        return button;
    };
    m_addButton = makeButton(tr("Add"), QStringLiteral("detailAddButton"));
    m_editButton = makeButton(tr("Edit"), QStringLiteral("detailEditButton"));
    // Rows with a lifecycle are deleted logically; plain link rows are removed.
    m_removeButton = makeButton(m_child.hasState ? tr("Delete") : tr("Remove"),
                                QStringLiteral("detailRemoveButton"));
    m_inactivateButton = makeButton(tr("Inactivate"), QStringLiteral("detailInactivateButton"));
    m_reactivateButton = makeButton(tr("Reactivate"), QStringLiteral("detailReactivateButton"));
    m_saveButton = makeButton(tr("Save"), QStringLiteral("detailSaveButton"));
    m_cancelButton = makeButton(tr("Cancel"), QStringLiteral("detailCancelButton"));
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
    const bool hasParent = key.isValid() && !key.toString().isEmpty();

    // A new parent discards any unsaved child edit (Req 6.3).
    m_operation = Operation::Idle;
    m_parentKey = hasParent ? key : QVariant();

    m_hintLabel->setText(hasParent
                             ? tr("Rows linked to the selected record.")
                             : tr("Select a saved record above to see its %1.")
                                   .arg(title().toLower()));
    setEnabled(hasParent);
    refresh();
}

QVariant MasterDetailWidget::parentKey() const
{
    return m_parentKey;
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

    if (m_parentKey.isValid()) {
        loadReferenceLabels();

        // Only the rows belonging to the selected parent, in every state.
        const QString fkToParent = QString::fromLatin1(m_child.fkToParent);
        const QString parentKey = m_parentKey.toString();
        const QList<QVariantMap> allRows = m_gateway.selectAll(m_spec);
        for (const QVariantMap &row : allRows) {
            if (row.value(fkToParent).toString() == parentKey)
                m_records.append(row);
        }
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

    m_form->clear();
    m_form->setMode(RegisterForm::Mode::ReadOnly);
    updateCommandState();
}

void MasterDetailWidget::onSelectionChanged()
{
    if (m_refreshing)
        return;

    const QModelIndexList selected = m_grid->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        if (m_operation != Operation::Idle)
            return; // keep the pending child form visible

        m_selectedRow = -1;
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
    if (m_operation == Operation::Adding)
        m_operation = Operation::Idle;

    m_form->setMode(m_operation == Operation::Editing ? RegisterForm::Mode::Modifying
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
    if (m_operation != Operation::Idle || !m_parentKey.isValid())
        return;

    m_operation = Operation::Adding;
    m_selectedRow = -1;
    m_grid->clearSelection();

    // The FK to the parent is filled in from the selected master record.
    m_form->clear();
    m_form->load({{QString::fromLatin1(m_child.fkToParent), m_parentKey}});
    m_form->setState(QStringLiteral("A"));
    m_form->setMode(RegisterForm::Mode::Adding);
    updateCommandState();
}

void MasterDetailWidget::startEdit()
{
    if (m_operation != Operation::Idle)
        return;

    if (m_selectedRow < 0) {
        QMessageBox::information(this, tr("Edit %1").arg(title()),
                                 tr("Select a row in the list first."));
        return;
    }

    m_operation = Operation::Editing;
    m_form->load(currentRecord());
    m_form->setMode(RegisterForm::Mode::Modifying);
    updateCommandState();
}

void MasterDetailWidget::removeRow()
{
    if (m_operation != Operation::Idle || m_selectedRow < 0)
        return;

    if (m_child.hasState) {
        applyState(QStringLiteral("*")); // logical delete (Req 4.1)
        return;
    }

    const auto answer = QMessageBox::question(
        this, tr("Remove %1").arg(title()),
        tr("Remove the selected row? This link will be deleted permanently."));
    if (answer != QMessageBox::Yes)
        return;

    const DbResult result = m_gateway.remove(m_spec, currentRecord().value(QStringLiteral("id")));
    if (!result.success) {
        QMessageBox::critical(this, tr("Operation failed"), result.error);
        return;
    }
    refresh();
}

void MasterDetailWidget::applyState(const QString &newState)
{
    if (m_operation != Operation::Idle || m_selectedRow < 0 || !m_child.hasState)
        return;

    const DbResult result = m_gateway.setState(
        m_spec, currentRecord().value(QStringLiteral("id")), newState);
    if (!result.success) {
        QMessageBox::critical(this, tr("Operation failed"), result.error);
        return;
    }
    refresh();
}

void MasterDetailWidget::save()
{
    if (m_operation == Operation::Idle) {
        QMessageBox::information(this, tr("Save %1").arg(title()),
                                 tr("There is no pending change to save. "
                                    "Choose Add or Edit first."));
        return;
    }

    const bool isInsert = m_operation == Operation::Adding;
    QVariantMap values = m_form->collect();
    if (isInsert)
        values.insert(QString::fromLatin1(m_child.fkToParent), m_parentKey);
    else if (m_selectedRow < 0)
        return;

    // Same rules as the parent screens: report every invalid field and write
    // nothing when any fails (Req 7.4–7.5).
    const Validator validator(m_gateway);
    const QList<ValidationError> errors = validator.validate(m_spec, values, isInsert);
    if (!errors.isEmpty()) {
        QStringList details;
        details.reserve(errors.size());
        for (const ValidationError &error : errors)
            details << QStringLiteral("• %1").arg(error.reason);
        QMessageBox::warning(this, tr("Validation failed"),
                             tr("Please correct the following fields:\n\n%1")
                                 .arg(details.join(QLatin1Char('\n'))));
        return;
    }

    const DbResult result = isInsert
        ? m_gateway.insert(m_spec, values)
        : m_gateway.update(m_spec, currentRecord().value(QStringLiteral("id")), values);
    if (!result.success) {
        QMessageBox::critical(this, tr("Save failed"), result.error);
        return;
    }

    m_operation = Operation::Idle;
    refresh();
}

void MasterDetailWidget::cancel()
{
    if (m_operation == Operation::Idle)
        return;

    m_operation = Operation::Idle;
    m_selectedRow = -1;
    m_grid->clearSelection();
    m_form->clear();
    m_form->setMode(RegisterForm::Mode::ReadOnly);
    updateCommandState();
}

bool MasterDetailWidget::allowed(PermissionAction action) const
{
    const QString appLabel = QString::fromLatin1(m_child.appLabel);
    const QString model = QString::fromLatin1(m_child.model);

    // Role permissions (admin) are reserved for superusers (design §5.3).
    if (appLabel == QLatin1String("admin"))
        return m_session.isSuperuser;

    // A user's roles and extra permissions are RBAC assignments (design §9).
    if (appLabel == QLatin1String("user"))
        return PermissionChecker::can(m_session, PermissionAction::AssignRole, appLabel, model);

    return PermissionChecker::can(m_session, action, appLabel, model);
}

void MasterDetailWidget::updateCommandState()
{
    const bool idle = m_operation == Operation::Idle;
    const bool hasParent = m_parentKey.isValid();
    const bool hasSelection = m_selectedRow >= 0 && m_selectedRow < m_records.size();
    const bool canAdd = allowed(PermissionAction::Add);
    const bool canChange = allowed(PermissionAction::Change);

    m_addButton->setEnabled(idle && hasParent && canAdd);
    m_editButton->setEnabled(idle && hasSelection && canChange);
    m_removeButton->setEnabled(idle && hasSelection && canChange);
    m_inactivateButton->setEnabled(idle && hasSelection && canChange);
    m_reactivateButton->setEnabled(idle && hasSelection && canChange);
    m_saveButton->setEnabled(!idle);
    m_cancelButton->setEnabled(!idle);
}

void MasterDetailWidget::loadReferenceLabels()
{
    m_refLabels.clear();
    for (const ColumnSpec &column : m_spec.columns) {
        if (column.type != ColumnType::Fk || !column.refTable || !column.refPk)
            continue;

        const QList<RefValue> values = m_gateway.loadRefValues(
            QString::fromLatin1(column.refTable), QString::fromLatin1(column.refPk),
            column.refDisplay ? QString::fromLatin1(column.refDisplay) : QString());

        QHash<QString, QString> labels;
        for (const RefValue &value : values)
            labels.insert(value.pk.toString(), value.text);
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
        return tr("Active");
    if (state == QLatin1String("I"))
        return tr("Inactive");
    if (state == QLatin1String("*"))
        return tr("Deleted");
    return state;
}
