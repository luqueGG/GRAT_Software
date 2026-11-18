#include "login_dialog.h"

#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>

#include "core/auth/auth_service.h"
#include "ui_login_dialog.h"

LoginDialog::LoginDialog(AuthService &authService, QWidget *parent)
    : QDialog(parent)
    , m_ui(std::make_unique<Ui::LoginDialog>())
    , m_authService(authService)
{
    m_ui->setupUi(this);

    m_ui->errorLabel->clear();
    m_ui->errorLabel->hide();

    if (QPushButton *signIn = m_ui->buttonBox->button(QDialogButtonBox::Ok))
        signIn->setText(tr("Iniciar sesion"));
    if (QPushButton *cancel = m_ui->buttonBox->button(QDialogButtonBox::Cancel))
        cancel->setText(tr("Cancelar"));

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &LoginDialog::attemptLogin);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_ui->passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::attemptLogin);
    connect(m_ui->usernameEdit, &QLineEdit::returnPressed, this,
            [this] { m_ui->passwordEdit->setFocus(); });

    // Clear a stale error as soon as the operator edits the credentials.
    connect(m_ui->usernameEdit, &QLineEdit::textEdited, this,
            [this] { m_ui->errorLabel->hide(); });
    connect(m_ui->passwordEdit, &QLineEdit::textEdited, this,
            [this] { m_ui->errorLabel->hide(); });

    m_ui->usernameEdit->setFocus();
}

LoginDialog::~LoginDialog() = default;

AuthSession LoginDialog::session() const
{
    return m_session;
}

void LoginDialog::attemptLogin()
{
    m_ui->errorLabel->hide();

    const QString username = m_ui->usernameEdit->text().trimmed();
    const QString password = m_ui->passwordEdit->text();
    if (username.isEmpty() || password.isEmpty()) {
        showError(tr("Ingrese su usuario y contrasena."));
        return;
    }

    QString error;
    const std::optional<AuthSession> session = m_authService.login(username, password, &error);
    if (!session) {
        showError(error.isEmpty() ? tr("No se pudo completar el inicio de sesion.") : error);
        m_ui->passwordEdit->clear();
        m_ui->passwordEdit->setFocus();
        return;
    }

    m_session = *session;
    accept();
}

void LoginDialog::showError(const QString &message)
{
    m_ui->errorLabel->setText(message);
    m_ui->errorLabel->setVisible(true);
}
