// GRAT_Software — entry point.
// Responsible Management of Transport Waste (Gestión Responsable de Residuos de
// Transporte). The application UI is entirely in English.
//
// Establishes the SQLite database connection on startup (Req 8.1); if the
// connection cannot be established, the operator is notified and the app exits
// (Req 8.2). Subsequent tasks add authentication and the metadata-driven CRUD
// screens.

#include <QApplication>
#include <QMainWindow>
#include <QMessageBox>

#include "core/db/database.h"

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

    QMainWindow window;
    window.setWindowTitle("GRAT_Software");
    window.resize(1024, 768);
    window.show();

    return app.exec();
}
