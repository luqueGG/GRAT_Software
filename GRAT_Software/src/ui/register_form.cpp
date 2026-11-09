#include "register_form.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDateTime>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVariant>

RegisterForm::RegisterForm(const TableSpec &spec, TableGateway &gateway,
                           const AuthSession &session, QWidget *parent)
    : QWidget(parent)
    , m_spec(spec)
    , m_gateway(gateway)
    , m_session(session)
{
    setObjectName(QStringLiteral("registerForm"));

    m_layout = new QFormLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    for (const ColumnSpec &column : m_spec.columns) {
        Field field;
        field.column = &column;

        const QString label = QString::fromLatin1(column.label);
        if (column.type == ColumnType::State) {
            auto *badge = new QLabel(this);
            badge->setObjectName(QStringLiteral("field_") + QString::fromLatin1(column.dbColumn));
            field.widget = badge;
            field.stateLabel = badge;
            m_layout->addRow(label, badge);
        } else {
            QWidget *input = createField(column);
            field.widget = input;
            field.combo = qobject_cast<QComboBox *>(input);
            m_layout->addRow(label, input);
        }

        m_fields.append(field);
    }

    setState(QStringLiteral("A"));
}

QWidget *RegisterForm::createField(const ColumnSpec &column)
{
    const QString columnName = QString::fromLatin1(column.dbColumn);
    const QString objectName = QStringLiteral("field_") + columnName;

    switch (column.type) {
    case ColumnType::Text: {
        auto *edit = new QLineEdit(this);
        edit->setObjectName(objectName);
        if (column.maxLength > 0)
            edit->setMaxLength(column.maxLength);
        return edit;
    }
    case ColumnType::Password: {
        auto *edit = new QLineEdit(this);
        edit->setObjectName(objectName);
        edit->setEchoMode(QLineEdit::Password);
        if (column.maxLength > 0)
            edit->setMaxLength(column.maxLength);
        return edit;
    }
    case ColumnType::Integer: {
        auto *edit = new QLineEdit(this);
        edit->setObjectName(objectName);
        return edit;
    }
    case ColumnType::Decimal: {
        auto *spin = new QDoubleSpinBox(this);
        spin->setObjectName(objectName);
        spin->setDecimals(2);
        spin->setRange(0.0, 99999999.99);
        spin->setSingleStep(0.01);
        spin->setGroupSeparatorShown(true);
        return spin;
    }
    case ColumnType::Date: {
        auto *dateEdit = new QDateEdit(this);
        dateEdit->setObjectName(objectName);
        dateEdit->setCalendarPopup(true);
        dateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
        dateEdit->setDate(QDate::currentDate());
        return dateEdit;
    }
    case ColumnType::DateTime: {
        auto *dateTimeEdit = new QDateTimeEdit(this);
        dateTimeEdit->setObjectName(objectName);
        dateTimeEdit->setCalendarPopup(true);
        dateTimeEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        dateTimeEdit->setDateTime(QDateTime::currentDateTime());
        return dateTimeEdit;
    }
    case ColumnType::Boolean: {
        auto *check = new QCheckBox(this);
        check->setObjectName(objectName);
        return check;
    }
    case ColumnType::Fk:
        return createCombo(column);
    case ColumnType::State:
        break;
    }

    auto *fallback = new QLineEdit(this);
    fallback->setObjectName(objectName);
    return fallback;
}

QComboBox *RegisterForm::createCombo(const ColumnSpec &column)
{
    auto *combo = new QComboBox(this);
    combo->setObjectName(QStringLiteral("field_") + QString::fromLatin1(column.dbColumn));
    combo->addItem(tr("— Select —"), QVariant());
    return combo;
}

QString RegisterForm::state() const
{
    return m_state;
}

void RegisterForm::setState(const QString &state)
{
    m_state = state;

    QString text;
    QString color;
    if (state == QLatin1String("A")) {
        text = tr("Active");
        color = QStringLiteral("#1b7f3a");
    } else if (state == QLatin1String("I")) {
        text = tr("Inactive");
        color = QStringLiteral("#8a6d00");
    } else if (state == QLatin1String("*")) {
        text = tr("Deleted");
        color = QStringLiteral("#b00020");
    } else {
        text = state;
        color = QStringLiteral("#555555");
    }

    for (const Field &field : m_fields) {
        if (!field.stateLabel)
            continue;
        field.stateLabel->setText(tr("● %1").arg(text));
        field.stateLabel->setStyleSheet(
            QStringLiteral("color: %1; font-weight: 600;").arg(color));
    }
}
