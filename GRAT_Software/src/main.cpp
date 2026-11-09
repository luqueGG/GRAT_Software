// GRAT_Software — entry point.
// Responsible Management of Transport Waste (Gestión Responsable de Residuos de
// Transporte). The application UI is entirely in English.
//
// Establishes the SQLite database connection on startup (Req 8.1); if the
// connection cannot be established, the operator is notified and the app exits
// (Req 8.2). Subsequent tasks add authentication and the metadata-driven CRUD
// screens.

#include <QApplication>
#include <QDebug>
#include <QDialog>
#include <QMainWindow>
#include <QMessageBox>

#include "core/auth/auth_service.h"
#include "core/db/database.h"
#include "ui/login_dialog.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("GRAT_Software");
    app.setOrganizationName("GRAT");

    Database database;
    if (!database.open()) {
        QMessageBox::critical(
            nullptr,
            QStringLiteral("GRAT_Software — Database Error"),
            QStringLiteral("Could not connect to the database.\n\n%1\n\n"
                          "The application will now close.")
                .arg(database.lastError()));
        return 1;
    }

    // A fresh database has RBAC roles but no operators yet; create the default
    // demonstration accounts once so the sign-in dialog is usable.
    AuthService authService(database.connection());
    QString seedError;
    if (!authService.ensureDefaultUsers(&seedError))
        qWarning().noquote() << seedError;

    LoginDialog loginDialog(authService);
    if (loginDialog.exec() != QDialog::Accepted)
        return 0;

    const AuthSession session = loginDialog.session();

    QMainWindow window;
    window.setWindowTitle(QStringLiteral("GRAT_Software — %1").arg(session.username));
    window.resize(1024, 768);
    window.show();

    return app.exec();
}
