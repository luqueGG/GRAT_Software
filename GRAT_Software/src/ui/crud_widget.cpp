#include "crud_widget.h"

#include <QAbstractItemView>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>

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
