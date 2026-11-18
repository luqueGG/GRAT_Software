// GRAT_Software — entry point.
// Responsible Management of Transport Waste (Gestión Responsable de Residuos de
// Transporte). La interfaz de la aplicación está completamente en español.
//
// Establishes the SQLite database connection on startup (Req 8.1); if the
// connection cannot be established, the operator is notified and the app exits
// (Req 8.2). Subsequent tasks add authentication and the metadata-driven CRUD
// screens.

#include <QApplication>
#include <QDebug>
#include <QDialog>
#include <QFile>
#include <QMessageBox>

#include <exception>

#include "core/auth/auth_service.h"
#include "core/db/database.h"
#include "core/errors/error_handler.h"
#include "ui/login_dialog.h"
#include "ui/main_window.h"

namespace {

// Last-resort central guard for unexpected exceptions raised from a UI event.
// The error is logged with its technical text and the operator sees a safe
// message; the event loop keeps running (Req 9.1, 9.4).
class GuardedApplication : public QApplication
{
public:
    using QApplication::QApplication;

    bool notify(QObject *receiver, QEvent *event) override
    {
        try {
            return QApplication::notify(receiver, event);
        } catch (const std::exception &error) {
            const ErrorHandler::Failure failure =
                ErrorHandler::exceptionFailure(error, QStringLiteral("UI event"));
            QMessageBox::critical(nullptr, QStringLiteral("GRAT_Software"), failure.message);
            return false;
        } catch (...) {
            const ErrorHandler::Failure failure =
                ErrorHandler::unknownExceptionFailure(QStringLiteral("UI event"));
            QMessageBox::critical(nullptr, QStringLiteral("GRAT_Software"), failure.message);
            return false;
        }
    }
};

} // namespace

int main(int argc, char *argv[])
{
    GuardedApplication app(argc, argv);
    app.setApplicationName("GRAT_Software");
    app.setOrganizationName("GRAT");

    // Visual theme only: the stylesheet is compiled into the binary as a Qt
    // resource, so no runtime files or paths are required.
    QFile styleFile(QStringLiteral(":/grat_theme.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));

    Database database;
    if (!database.open()) {
        QMessageBox::critical(
            nullptr,
            QStringLiteral("GRAT_Software - Error de base de datos"),
            QStringLiteral("No se pudo conectar con la base de datos.\n\n%1\n\n"
                          "La aplicacion se cerrara.")
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

    MainWindow window(database, session);
    window.show();

    return app.exec();
}
