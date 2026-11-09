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
#include <QSplitter>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStringList>
#include <QTableView>
#include <QVBoxLayout>

#include "core/auth/auth_service.h"
#include "core/auth/permission_checker.h"
#include "core/crud/validator.h"
#include "core/db/database.h"
#include "ui/register_form.h"

CrudWidget::CrudWidget(Database &database, const TableSpec &spec,
                       const AuthSession &session, const QString &title,
                       QWidget *parent)
    : QWidget(parent)
    , m_database(database)
    , m_spec(spec)
    , m_session(session)
    , m_title(title)
    , m_gateway(database.connection())
{
    setObjectName(QStringLiteral("crudWidget_") + QString::fromLatin1(spec.table));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto *heading = new QLabel(m_title, this);
    QFont headingFont = heading->font();
    headingFont.setPointSize(18);
    headingFont.setBold(true);
    heading->setFont(headingFont);
    mainLayout->addWidget(heading);

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
    auto makeButton = [this](const QString &text, const QString &name) {
        auto *button = new QPushButton(text, this);
        button->setObjectName(name);
        return button;
    };
    m_addButton = makeButton(tr("Add"), QStringLiteral("addButton"));
    m_modifyButton = makeButton(tr("Modify"), QStringLiteral("modifyButton"));
    m_deleteButton = makeButton(tr("Delete"), QStringLiteral("deleteButton"));
    m_inactivateButton = makeButton(tr("Inactivate"), QStringLiteral("inactivateButton"));
    m_reactivateButton = makeButton(tr("Reactivate"), QStringLiteral("reactivateButton"));
    m_updateButton = makeButton(tr("Update"), QStringLiteral("updateButton"));
    m_cancelButton = makeButton(tr("Cancel"), QStringLiteral("cancelButton"));

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

    m_records = m_gateway.selectAll(m_spec);

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
    m_form->clear();
    m_form->setMode(RegisterForm::Mode::ReadOnly);
    updateCommandState();
}

void CrudWidget::onGridSelectionChanged()
{
    if (m_refreshing)
        return;

    const QModelIndexList selected = m_grid->selectionModel()->selectedRows();
    if (selected.isEmpty()) {
        if (m_operation != Operation::Idle)
            return; // keep the pending Add/Modify form visible

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

    // Selecting another row replaces the displayed record (Req 2.4). During Add
    // this abandons the pending new record; during Modify it keeps Modify mode.
    if (m_operation == Operation::Adding)
        m_operation = Operation::Idle;

    if (m_operation == Operation::Modifying) {
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

bool CrudWidget::hasColumn(const QString &columnName) const
{
    for (const ColumnSpec &column : m_spec.columns) {
        if (columnName == QLatin1String(column.dbColumn))
            return true;
    }
    return false;
}

void CrudWidget::startAdd()
{
    if (m_operation != Operation::Idle)
        return;

    m_operation = Operation::Adding;
    m_selectedRow = -1;
    m_grid->clearSelection();

    m_form->clear();
    m_form->setState(QStringLiteral("A"));
    m_form->setMode(RegisterForm::Mode::Adding);
    updateCommandState();
}

void CrudWidget::startModify()
{
    if (m_operation != Operation::Idle)
        return;

    if (m_selectedRow < 0) {
        QMessageBox::information(this, tr("Modify Record"),
                                 tr("Select a record in the grid first."));
        return;
    }

    m_operation = Operation::Modifying;
    m_form->load(currentRecord());
    m_form->setMode(RegisterForm::Mode::Modifying);
    updateCommandState();
}

void CrudWidget::applyState(const QString &newState)
{
    if (m_operation != Operation::Idle)
        return;

    if (m_selectedRow < 0) {
        QMessageBox::information(this, tr("State Change"),
                                 tr("Select a record in the grid first."));
        return;
    }

    const DbResult result = m_gateway.setState(m_spec, currentPk(), newState);
    if (!result.success) {
        QMessageBox::critical(this, tr("Operation failed"), result.error);
        return;
    }

    m_operation = Operation::Idle;
    refreshGrid();
}

void CrudWidget::updateRecord()
{
    if (m_operation == Operation::Idle) {
        // Req 5.2: Update without a pending operation explains why nothing happens.
        QMessageBox::information(this, tr("Update Record"),
                                 tr("There is no pending operation to update. "
                                    "Choose Add or Modify first."));
        return;
    }

    const bool isInsert = m_operation == Operation::Adding;
    QVariantMap values = m_form->collect();

    if (isInsert) {
        // created_by_id defaults to the authenticated operator (Task 10).
        if (hasColumn(QStringLiteral("created_by_id")))
            values.insert(QStringLiteral("created_by_id"), m_session.userId);
    } else {
        if (m_selectedRow < 0)
            return;
        if (hasColumn(QStringLiteral("updated_at"))) {
            values.insert(QStringLiteral("updated_at"),
                          QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
        }
    }

    // Req 7: validate before any write, collecting every invalid field so the
    // operator can fix them in one pass (Req 7.4–7.5).
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

    // Passwords are stored hashed, never as typed (design §13.3).
    for (const ColumnSpec &column : m_spec.columns) {
        if (column.type != ColumnType::Password)
            continue;
        const QString columnName = QString::fromLatin1(column.dbColumn);
        if (!values.contains(columnName))
            continue;
        const QString plainText = values.value(columnName).toString();
        if (!plainText.isEmpty())
            values[columnName] = AuthService::hashPassword(plainText);
    }

    DbResult result;
    if (isInsert)
        result = m_gateway.insert(m_spec, values);
    else
        result = m_gateway.update(m_spec, currentPk(), values);

    if (!result.success) {
        QMessageBox::critical(this, tr("Update failed"), result.error);
        return;
    }

    m_operation = Operation::Idle;
    refreshGrid();
}

void CrudWidget::cancelOperation()
{
    if (m_operation == Operation::Idle)
        return;

    m_operation = Operation::Idle;
    m_selectedRow = -1;
    m_grid->clearSelection();
    m_form->clear();
    m_form->setState(QStringLiteral("A"));
    m_form->setMode(RegisterForm::Mode::ReadOnly);
    updateCommandState();
}

void CrudWidget::updateCommandState()
{
    const bool idle = m_operation == Operation::Idle;
    const bool hasSelection = m_selectedRow >= 0 && m_selectedRow < m_records.size();

    const QString appLabel = QString::fromLatin1(m_spec.appLabel);
    const QString model = QString::fromLatin1(m_spec.model);
    const bool canAdd = PermissionChecker::can(m_session, PermissionAction::Add,
                                               appLabel, model);
    const bool canChange = PermissionChecker::can(m_session, PermissionAction::Change,
                                                  appLabel, model);

    m_addButton->setEnabled(idle && canAdd);
    m_modifyButton->setEnabled(idle && hasSelection && canChange);
    m_deleteButton->setEnabled(idle && hasSelection && canChange);
    m_inactivateButton->setEnabled(idle && hasSelection && canChange);
    m_reactivateButton->setEnabled(idle && hasSelection && canChange);
    // Update stays clickable in Idle so the operator gets the Req 5.2 message,
    // but only when they may change the table at all.
    m_updateButton->setEnabled(canChange);
    m_cancelButton->setEnabled(!idle);
}

QString CrudWidget::displayValue(const ColumnSpec &column, const QVariant &value) const
{
    switch (column.type) {
    case ColumnType::State:
        return stateText(value.toString());
    case ColumnType::Boolean:
        return value.toInt() != 0 ? tr("Yes") : tr("No");
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
        return tr("Active");
    if (state == QLatin1String("I"))
        return tr("Inactive");
    if (state == QLatin1String("*"))
        return tr("Deleted");
    return state;
}
