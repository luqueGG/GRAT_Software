#include "master_detail_widget.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStringList>
#include <QTableView>
#include <QVBoxLayout>

#include "core/db/database.h"

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
    m_hintLabel->setStyleSheet(QStringLiteral("color: #555555;"));
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
    layout->addWidget(m_grid, 1);

    connect(m_grid->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MasterDetailWidget::onSelectionChanged);

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
}

void MasterDetailWidget::onSelectionChanged()
{
    if (m_refreshing)
        return;

    const QModelIndexList selected = m_grid->selectionModel()->selectedRows();
    m_selectedRow = selected.isEmpty() ? -1 : selected.first().row();
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
