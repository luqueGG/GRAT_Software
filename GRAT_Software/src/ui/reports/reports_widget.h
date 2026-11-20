// reports_widget.h — Reports window (P4): filtered views and CSV export.

#pragma once

#include <QDialog>

#include "app/session.h"
#include "core/db/database.h"
#include "core/reports/report_service.h"

class QComboBox;
class QFileDialog;
class QLabel;
class QPushButton;
class QTableWidget;

class ReportsWidget : public QDialog
{
    Q_OBJECT
public:
    explicit ReportsWidget(Database &database, const AuthSession &session,
                           QWidget *parent = nullptr);

private slots:
    void refresh();
    void exportCsv();

private:
    void buildUi();
    void populateTable(const ReportResult &result);

    Database &m_database;
    AuthSession m_session;
    ReportService m_service;

    QComboBox *m_reportCombo = nullptr;
    QComboBox *m_stateCombo = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_countLabel = nullptr;
    QLabel *m_totalLabel = nullptr;
    QPushButton *m_exportButton = nullptr;

    ReportResult m_current;
};