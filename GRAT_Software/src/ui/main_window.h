// main_window.h — application shell: navigation, workspace and status (Task 9).
//
// The window is built from the TableSpec registry: a module-grouped tree drives a
// central QStackedWidget, and the status bar reports database state plus the
// authenticated operator and role(s).

#pragma once

#include <QHash>
#include <QMainWindow>
#include <memory>

#include "app/session.h"

class Database;
class QLabel;
class QTimer;
class QTreeWidgetItem;
struct TableSpec;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Database &database, const AuthSession &session,
                        QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void showPage(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void refreshConnectionState();

private:
    void buildMenus();
    void buildNavigation();
    void updateStatusBar();
    bool canView(const TableSpec &spec) const;
    QWidget *createPage(const TableSpec &spec);
    void navigateToTable(const QString &table);
    void setMaintenanceEnabled(bool enabled);

    QString moduleTitle(const QString &module) const;
    QString tableTitle(const QString &table) const;

    std::unique_ptr<Ui::MainWindow> m_ui;
    Database &m_database;
    AuthSession m_session;

    QHash<QString, QWidget *> m_pages;
    QHash<QString, QTreeWidgetItem *> m_navigationItems;

    QLabel *m_dbStatusLabel = nullptr;
    QLabel *m_userLabel = nullptr;
    QLabel *m_rolesLabel = nullptr;
    QTimer *m_connectionTimer = nullptr;
    bool m_connected = true;
};
