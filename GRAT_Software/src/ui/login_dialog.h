// login_dialog.h — modal sign-in dialog (design §7.1, Task 8).
//
// Collects username + password, calls AuthService::login, and keeps itself open
// with an informative message on failure (Req 9.1). The validated session is
// exposed to the caller after exec() returns QDialog::Accepted.

#pragma once

#include <QDialog>
#include <memory>

#include "app/session.h"

class AuthService;

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(AuthService &authService, QWidget *parent = nullptr);
    ~LoginDialog() override;

    AuthSession session() const;

private slots:
    void attemptLogin();

private:
    void showError(const QString &message);

    std::unique_ptr<Ui::LoginDialog> m_ui;
    AuthService &m_authService;
    AuthSession m_session;
};
