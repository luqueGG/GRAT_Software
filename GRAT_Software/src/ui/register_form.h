// register_form.h — metadata-driven record form (design §6/§7.3, Task 10).
//
// Builds one input widget per ColumnSpec: text, decimal, date/date-time, boolean,
// FK ComboBox, password and a read-only state badge. The same form serves Add,
// Modify and read-only record viewing.

#pragma once

#include <QList>
#include <QString>
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
    explicit RegisterForm(const TableSpec &spec, TableGateway &gateway,
                          const AuthSession &session, QWidget *parent = nullptr);

    QString state() const;
    void setState(const QString &state);

private:
    struct Field
    {
        const ColumnSpec *column = nullptr;
        QWidget *widget = nullptr;
        QComboBox *combo = nullptr;
        QLabel *stateLabel = nullptr;
    };

    QWidget *createField(const ColumnSpec &column);
    QComboBox *createCombo(const ColumnSpec &column);

    const TableSpec &m_spec;
    TableGateway &m_gateway;
    const AuthSession &m_session;
    QFormLayout *m_layout = nullptr;
    QList<Field> m_fields;
    QString m_state = QStringLiteral("A");
};
