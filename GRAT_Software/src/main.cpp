// GRAT_Software — entry point.
// Responsible Management of Transport Waste (Gestión Responsable de Residuos de
// Transporte). The application UI is entirely in English.
//
// At this stage a minimal QApplication opens an empty QMainWindow. Subsequent tasks
// add the database layer, authentication, and the metadata-driven CRUD screens.

#include <QApplication>
#include <QMainWindow>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("GRAT_Software");
    app.setOrganizationName("GRAT");

    QMainWindow window;
    window.setWindowTitle("GRAT_Software");
    window.resize(1024, 768);
    window.show();

    return app.exec();
}
