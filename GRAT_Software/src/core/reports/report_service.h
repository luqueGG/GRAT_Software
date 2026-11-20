// report_service.h — operational reports and CSV export for GRAT_Software (P4).
//
// Read-only reports over waste and transfer records, permission-gated through
// the existing RBAC matrix (view_waste / view_transfer). No new permissions
// are introduced and no data is modified by generating a report.

#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QVariant>

#include "app/session.h"

class Database;

struct ReportResult
{
    bool ok = false;
    bool permissionDenied = false;
    QString errorMessage;
    QString title;
    QStringList headers;
    QList<QStringList> rows;
    int count = 0;
    double totalAmount = 0.0;
    bool hasTotalAmount = false;
};

class ReportService
{
public:
    explicit ReportService(Database &database, const AuthSession &session);

    // stateFilter: "", "A", "I" or "*". Empty/"Todos" means all states.
    ReportResult wasteReport(const QString &stateFilter) const;
    ReportResult transferReport(const QString &stateFilter) const;

    // Writes the same filtered set shown in the view to a CSV file.
    // Returns true on success; errorMessage receives a safe message on failure.
    static bool writeCsv(const QString &path, const ReportResult &result,
                         QString *errorMessage = nullptr);

    static QString stateLabel(const QString &state);

private:
    Database &m_database;
    AuthSession m_session;

    ReportResult runQuery(const QString &title, const QString &sql,
                          const QStringList &headers, const QString &stateFilter,
                          const QString &permissionApp, const QString &permissionModel,
                          int amountColumnIndex) const;
};