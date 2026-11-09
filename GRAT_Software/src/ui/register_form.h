// register_form.h — metadata-driven record form (design §6/§7.3, Task 10).
//
// Builds one input widget per ColumnSpec: text, decimal, date/date-time, boolean,
// FK ComboBox, password and a read-only state badge. The same form serves Add,
// Modify and read-only record viewing.

#pragma once

#include <QList>
#include <QString>
#include <QVariantMap>
#include <QWidget>

#include "core/crud/table_spec.h"

class AuthSession;
class QComboBox;
class QFormLayout;
class QLabel;
class TableGateway;

class RegisterForm : public QWidget
{
    Q_OBJECT

public:
    enum class Mode {
        ReadOnly,  // browsing a selected record
        Adding,    // new record; natural PK editable
        Modifying  // existing record; PK locked
    };

    explicit RegisterForm(const TableSpec &spec, TableGateway &gateway,
                          const AuthSession &session, QWidget *parent = nullptr);

    void setMode(Mode mode);
    Mode mode() const;

    void load(const QVariantMap &record);
    void clear();
    QVariantMap collect() const;

    QString state() const;
    void setState(const QString &state);

private:
    struct Field
    {
        const ColumnSpec *column = nullptr;
        QWidget *widget = nullptr;
        QComboBox *combo = nullptr;
        QLabel *stateLabel = nullptr;
        bool referenceLoaded = true;
    };

    QWidget *createField(const ColumnSpec &column);
    QComboBox *createCombo(const ColumnSpec &column);
    void applyMode();
    void populateRefValues();
    void selectComboValue(Field &field, const QVariant &value);
    void setWarning(const QString &message);

    const TableSpec &m_spec;
    TableGateway &m_gateway;
    const AuthSession &m_session;
    QFormLayout *m_layout = nullptr;
    QLabel *m_warningLabel = nullptr;
    QList<Field> m_fields;
    QString m_state = QStringLiteral("A");
    Mode m_mode = Mode::ReadOnly;
};
