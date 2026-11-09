#include "register_form.h"

#include <QAbstractSpinBox>
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
#include <QTime>
#include <QVariant>

#include "app/session.h"
#include "core/crud/table_gateway.h"

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

    m_warningLabel = new QLabel(this);
    m_warningLabel->setObjectName(QStringLiteral("referenceWarningLabel"));
    m_warningLabel->setWordWrap(true);
    m_warningLabel->setStyleSheet(QStringLiteral("color: #8a6d00; font-weight: 600;"));
    m_warningLabel->hide();
    m_layout->addRow(m_warningLabel);

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

    // created_by_id is operator-invisible metadata: show the authenticated user
    // and never let it be edited (Task 10).
    populateRefValues();

    for (Field &field : m_fields) {
        if (!field.combo)
            continue;
        if (QString::fromLatin1(field.column->dbColumn) == QLatin1String("created_by_id"))
            selectComboValue(field, m_session.userId);
    }

    setState(QStringLiteral("A"));
    setMode(Mode::ReadOnly);
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
        dateTimeEdit->setMinimumDateTime(QDateTime(QDate(1900, 1, 1), QTime(0, 0)));
        dateTimeEdit->setSpecialValueText(tr("—"));
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

void RegisterForm::setMode(Mode mode)
{
    m_mode = mode;
    applyMode();
}

RegisterForm::Mode RegisterForm::mode() const
{
    return m_mode;
}

void RegisterForm::applyMode()
{
    const QString pkColumn = QString::fromLatin1(m_spec.pkColumn);

    for (const Field &field : m_fields) {
        if (!field.column || field.stateLabel)
            continue;

        const QString columnName = QString::fromLatin1(field.column->dbColumn);
        bool editable = field.column->editable && !field.column->autoManaged
            && field.column->type != ColumnType::State;

        if (columnName == pkColumn) {
            // Identity PKs are generated and always locked; natural PKs are
            // editable only while adding a new record (Req 3.3).
            editable = !m_spec.identityPk && m_mode == Mode::Adding;
        }

        if (m_mode == Mode::ReadOnly)
            editable = false;

        if (auto *edit = qobject_cast<QLineEdit *>(field.widget)) {
            edit->setReadOnly(!editable);
        } else if (auto *spin = qobject_cast<QAbstractSpinBox *>(field.widget)) {
            spin->setReadOnly(!editable);
            spin->setButtonSymbols(editable ? QAbstractSpinBox::UpDownArrows
                                            : QAbstractSpinBox::NoButtons);
        } else if (auto *check = qobject_cast<QCheckBox *>(field.widget)) {
            check->setEnabled(editable);
        } else if (field.combo) {
            field.combo->setEnabled(editable && field.referenceLoaded);
        }
    }
}

void RegisterForm::populateRefValues()
{
    bool anyFailure = false;

    for (Field &field : m_fields) {
        if (!field.combo)
            continue;

        field.combo->clear();
        field.combo->addItem(tr("— Select —"), QVariant());

        if (!field.column->refTable || !field.column->refPk) {
            field.referenceLoaded = false;
            field.combo->setEnabled(false);
            field.combo->setToolTip(tr("No reference table is configured for this field."));
            anyFailure = true;
            continue;
        }

        const QString refTable = QString::fromLatin1(field.column->refTable);
        const QString refPk = QString::fromLatin1(field.column->refPk);
        const QString refDisplay = field.column->refDisplay
            ? QString::fromLatin1(field.column->refDisplay)
            : QString();

        const QList<RefValue> values = m_gateway.loadRefValues(refTable, refPk, refDisplay);
        if (values.isEmpty()) {
            // Req 10.4: warn and disable the ComboBox when the referenced table
            // cannot be loaded (or has no selectable rows).
            field.referenceLoaded = false;
            field.combo->setEnabled(false);
            field.combo->setToolTip(
                tr("The referenced list could not be loaded or is empty."));
            anyFailure = true;
            continue;
        }

        field.referenceLoaded = true;
        for (const RefValue &value : values)
            field.combo->addItem(value.text, value.pk);
        field.combo->setToolTip(QString());
    }

    if (anyFailure) {
        setWarning(tr("Some reference lists could not be loaded or are empty. "
                      "The related fields have been disabled."));
    } else {
        setWarning(QString());
    }
}

void RegisterForm::selectComboValue(Field &field, const QVariant &value)
{
    if (!field.combo)
        return;

    int index = value.isValid() ? field.combo->findData(value) : 0;
    if (index < 0 && value.isValid()) {
        // Keep a stored value visible even if its reference row is unavailable.
        field.combo->addItem(value.toString(), value);
        index = field.combo->count() - 1;
    }
    field.combo->setCurrentIndex(index < 0 ? 0 : index);
}

void RegisterForm::setWarning(const QString &message)
{
    if (!m_warningLabel)
        return;

    if (message.isEmpty()) {
        m_warningLabel->clear();
        m_warningLabel->hide();
        return;
    }

    m_warningLabel->setText(message);
    m_warningLabel->show();
}

void RegisterForm::load(const QVariantMap &record)
{
    for (Field &field : m_fields) {
        const QString columnName = QString::fromLatin1(field.column->dbColumn);
        const QVariant value = record.value(columnName);

        if (field.stateLabel) {
            if (value.isValid())
                setState(value.toString());
            continue;
        }

        if (field.combo) {
            selectComboValue(field, value);
            continue;
        }

        if (auto *edit = qobject_cast<QLineEdit *>(field.widget)) {
            // A stored password hash is never shown back to the operator.
            if (field.column->type == ColumnType::Password)
                edit->clear();
            else
                edit->setText(value.toString());
        } else if (auto *spin = qobject_cast<QDoubleSpinBox *>(field.widget)) {
            spin->setValue(value.toDouble());
        } else if (auto *dateEdit = qobject_cast<QDateEdit *>(field.widget)) {
            const QDate date = QDate::fromString(value.toString(), Qt::ISODate);
            dateEdit->setDate(date.isValid() ? date : QDate::currentDate());
        } else if (auto *dateTimeEdit = qobject_cast<QDateTimeEdit *>(field.widget)) {
            const QDateTime dateTime = QDateTime::fromString(value.toString(), Qt::ISODate);
            dateTimeEdit->setDateTime(dateTime.isValid() ? dateTime
                                                         : dateTimeEdit->minimumDateTime());
        } else if (auto *check = qobject_cast<QCheckBox *>(field.widget)) {
            check->setChecked(value.toInt() != 0);
        }
    }
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
