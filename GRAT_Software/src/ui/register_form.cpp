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
        auto *labelWidget = new QLabel(label, this);
        labelWidget->setProperty("fieldLabel", true);

        if (column.type == ColumnType::State) {
            auto *badge = new QLabel(this);
            badge->setObjectName(QStringLiteral("field_") + QString::fromLatin1(column.dbColumn));
            field.widget = badge;
            field.stateLabel = badge;
            m_layout->addRow(labelWidget, badge);
        } else {
            QWidget *input = createField(column);
            field.widget = input;
            field.combo = qobject_cast<QComboBox *>(input);
            m_layout->addRow(labelWidget, input);
        }

        m_fields.append(field);
    }

    // created_by_id is operator-invisible metadata: show the authenticated user
    // and never let it be edited (Task 10).
    reloadReferences();

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
    QStringList loadFailures;
    QStringList emptyLists;

    for (Field &field : m_fields) {
        if (!field.combo)
            continue;

        field.combo->clear();
        field.combo->addItem(tr("— Select —"), QVariant());

        const QString label = field.column->label
            ? QString::fromLatin1(field.column->label)
            : QString::fromLatin1(field.column->dbColumn);

        if (!field.column->refTable || !field.column->refPk) {
            field.referenceLoaded = false;
            field.combo->setEnabled(false);
            field.combo->setToolTip(tr("No reference table is configured for this field."));
            loadFailures << label;
            continue;
        }

        const QString refTable = QString::fromLatin1(field.column->refTable);
        const QString refPk = QString::fromLatin1(field.column->refPk);
        const QString refDisplay = field.column->refDisplay
            ? QString::fromLatin1(field.column->refDisplay)
            : QString();

        QString error;
        const QList<RefValue> values = m_gateway.loadRefValues(refTable, refPk, refDisplay,
                                                              &error);
        if (!error.isEmpty()) {
            // Req 10.4: warn and disable the ComboBox when the referenced table
            // cannot be loaded.
            field.referenceLoaded = false;
            field.combo->setEnabled(false);
            field.combo->setToolTip(tr("The related list could not be loaded."));
            loadFailures << label;
            continue;
        }
        if (values.isEmpty()) {
            // A valid but empty reference table offers no selectable value.
            field.referenceLoaded = false;
            field.combo->setEnabled(false);
            field.combo->setToolTip(tr("No selectable records are available for this field."));
            emptyLists << label;
            continue;
        }

        field.referenceLoaded = true;
        for (const RefValue &value : values)
            field.combo->addItem(value.text, value.pk);
        field.combo->setToolTip(QString());
    }

    QStringList warnings;
    if (!loadFailures.isEmpty()) {
        warnings << tr("The related lists for %1 could not be loaded.")
                        .arg(loadFailures.join(QStringLiteral(", ")));
    }
    if (!emptyLists.isEmpty()) {
        warnings << tr("No selectable records are available for %1.")
                        .arg(emptyLists.join(QStringLiteral(", ")));
    }
    if (!warnings.isEmpty())
        warnings << tr("The affected fields have been disabled.");
    setWarning(warnings.join(QLatin1Char(' ')));
}

void RegisterForm::reloadReferences()
{
    // Preserve what the operator currently sees. Reloading must not silently
    // replace the selected record when a connection is restored (Req 10.3).
    QHash<QString, QVariant> selections;
    for (const Field &field : m_fields) {
        if (field.combo)
            selections.insert(QString::fromLatin1(field.column->dbColumn),
                              field.combo->currentData());
    }

    populateRefValues();

    for (Field &field : m_fields) {
        if (!field.combo)
            continue;

        const QString columnName = QString::fromLatin1(field.column->dbColumn);
        QVariant value = selections.value(columnName);
        if (columnName == QLatin1String("created_by_id") && !value.isValid())
            value = m_session.userId;
        selectComboValue(field, value);
    }

    applyMode();
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

void RegisterForm::clear()
{
    setState(QStringLiteral("A"));

    for (Field &field : m_fields) {
        if (field.stateLabel)
            continue;

        const QString columnName = QString::fromLatin1(field.column->dbColumn);
        if (field.combo) {
            if (columnName == QLatin1String("created_by_id"))
                selectComboValue(field, m_session.userId);
            else
                field.combo->setCurrentIndex(0);
            continue;
        }

        if (auto *edit = qobject_cast<QLineEdit *>(field.widget)) {
            edit->clear();
        } else if (auto *spin = qobject_cast<QDoubleSpinBox *>(field.widget)) {
            spin->setValue(0.0);
        } else if (auto *dateEdit = qobject_cast<QDateEdit *>(field.widget)) {
            dateEdit->setDate(QDate::currentDate());
        } else if (auto *dateTimeEdit = qobject_cast<QDateTimeEdit *>(field.widget)) {
            dateTimeEdit->setDateTime(field.column->autoManaged
                                          ? dateTimeEdit->minimumDateTime()
                                          : QDateTime::currentDateTime());
        } else if (auto *check = qobject_cast<QCheckBox *>(field.widget)) {
            check->setChecked(false);
        }
    }
}

QVariantMap RegisterForm::collect() const
{
    QVariantMap values;
    const QString pkColumn = QString::fromLatin1(m_spec.pkColumn);

    for (const Field &field : m_fields) {
        const ColumnSpec &column = *field.column;
        const QString columnName = QString::fromLatin1(column.dbColumn);

        // State changes only through the lifecycle commands; auto fields are
        // database-managed; identity PKs are generated.
        if (column.type == ColumnType::State || column.autoManaged)
            continue;
        if (m_spec.identityPk && columnName == pkColumn)
            continue;

        if (field.combo) {
            values.insert(columnName, field.combo->currentData());
            continue;
        }

        if (auto *edit = qobject_cast<QLineEdit *>(field.widget)) {
            // Empty password on Modify means "keep the current password".
            if (column.type == ColumnType::Password && edit->text().isEmpty())
                continue;
            values.insert(columnName, edit->text());
        } else if (auto *spin = qobject_cast<QDoubleSpinBox *>(field.widget)) {
            values.insert(columnName, spin->value());
        } else if (auto *dateEdit = qobject_cast<QDateEdit *>(field.widget)) {
            values.insert(columnName, dateEdit->date().toString(Qt::ISODate));
        } else if (auto *dateTimeEdit = qobject_cast<QDateTimeEdit *>(field.widget)) {
            values.insert(columnName, dateTimeEdit->dateTime().toString(Qt::ISODate));
        } else if (auto *check = qobject_cast<QCheckBox *>(field.widget)) {
            values.insert(columnName, check->isChecked() ? 1 : 0);
        }
    }

    return values;
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
