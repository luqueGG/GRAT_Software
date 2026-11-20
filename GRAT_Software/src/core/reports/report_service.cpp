#include "report_service.h"

#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextStream>

#include "core/auth/permission_checker.h"
#include "core/db/database.h"
#include "core/errors/error_handler.h"

namespace {
const QString kAllStates = QStringLiteral("Todos");
}

ReportService::ReportService(Database &database, const AuthSession &session)
    : m_database(database), m_session(session)
{
}

QString ReportService::stateLabel(const QString &state)
{
    if (state.isEmpty() || state == kAllStates)
        return QStringLiteral("Todos");
    if (state == QStringLiteral("A"))
        return QStringLiteral("Activo");
    if (state == QStringLiteral("I"))
        return QStringLiteral("Inactivo");
    if (state == QStringLiteral("*"))
        return QStringLiteral("Eliminado");
    return state;
}

ReportResult ReportService::wasteReport(const QString &stateFilter) const
{
    const QStringList headers = {
        QStringLiteral("ID residuo"),
        QStringLiteral("Empresa generadora"),
        QStringLiteral("Cantidad"),
        QStringLiteral("Toxicidad"),
        QStringLiteral("Creado por"),
        QStringLiteral("Fecha creacion"),
        QStringLiteral("Estado")
    };
    const QString sql =
        QStringLiteral("SELECT w.id_waste, g.name_company, w.amount_waste, "
                       "t.name, u.username, w.created_at, w.is_active "
                       "FROM waste_waste w "
                       "JOIN waste_generatorcompany g ON g.id_gen_company = w.id_gen_company_id "
                       "JOIN catalog_toxicitylevel t ON t.id = w.id_toxicity_id "
                       "JOIN user_user u ON u.id = w.created_by_id "
                       "%1 ORDER BY w.id_waste");
    return runQuery(QStringLiteral("Residuos registrados"), sql, headers, stateFilter,
                    QStringLiteral("waste"), QStringLiteral("waste"), 2);
}

ReportResult ReportService::transferReport(const QString &stateFilter) const
{
    const QStringList headers = {
        QStringLiteral("ID traslado"),
        QStringLiteral("Empresa transportista"),
        QStringLiteral("Destino"),
        QStringLiteral("Residuo"),
        QStringLiteral("Tipo contenedor"),
        QStringLiteral("Cantidad"),
        QStringLiteral("Fecha entrega"),
        QStringLiteral("Creado por"),
        QStringLiteral("Fecha creacion"),
        QStringLiteral("Estado")
    };
    const QString sql =
        QStringLiteral("SELECT tr.id_transfer, tc.name, d.name, tr.id_waste_id, "
                       "ct.name, tr.amount_waste, tr.delivery_date, u.username, "
                       "tr.created_at, tr.is_active "
                       "FROM transfer_transfer tr "
                       "JOIN transfer_travelcompany tc ON tc.id_travel_company = tr.id_travel_company_id "
                       "JOIN transfer_destination d ON d.id = tr.id_destination_id "
                       "JOIN catalog_containertype ct ON ct.id = tr.id_container_type_id "
                       "JOIN user_user u ON u.id = tr.created_by_id "
                       "%1 ORDER BY tr.id_transfer");
    return runQuery(QStringLiteral("Traslados registrados"), sql, headers, stateFilter,
                    QStringLiteral("transfer"), QStringLiteral("transfer"), 5);
}

ReportResult ReportService::runQuery(const QString &title, const QString &sql,
                                    const QStringList &headers,
                                    const QString &stateFilter,
                                    const QString &permissionApp,
                                    const QString &permissionModel,
                                    int amountColumnIndex) const
{
    ReportResult result;
    result.title = title;
    result.headers = headers;

    if (!PermissionChecker::can(m_session, PermissionAction::View,
                                permissionApp, permissionModel)) {
        result.permissionDenied = true;
        result.errorMessage = QStringLiteral("No tiene permiso para consultar este reporte.");
        return result;
    }

    QSqlDatabase db = m_database.connection();
    if (!m_database.isConnected() || !db.isOpen()) {
        result.errorMessage = ErrorHandler::defaultMessage(ErrorHandler::Category::Connection);
        return result;
    }

    QString whereClause;
    if (!stateFilter.isEmpty() && stateFilter != kAllStates) {
        // Determine alias: waste uses w, transfer uses tr
        const QString alias = sql.contains(QStringLiteral("FROM waste_waste")) ? QStringLiteral("w") : QStringLiteral("tr");
        whereClause = QStringLiteral("WHERE %1.is_active = :st").arg(alias);
    }

    const QString finalSql = sql.arg(whereClause.isEmpty() ? QString() : whereClause);

    QSqlQuery query(db);
    query.prepare(finalSql);
    if (!whereClause.isEmpty())
        query.bindValue(QStringLiteral(":st"), stateFilter);

    if (!query.exec()) {
        const auto failure = ErrorHandler::sqlFailure(query.lastError(),
            ErrorHandler::Category::Persistence, QStringLiteral("report query"));
        result.errorMessage = failure.message;
        return result;
    }

    double total = 0.0;
    bool hasAmount = false;
    while (query.next()) {
        QStringList row;
        for (int i = 0; i < headers.size(); ++i) {
            const QVariant v = query.value(i);
            if (i == amountColumnIndex) {
                const double d = v.toDouble();
                total += d;
                hasAmount = true;
                row << QString::number(d, 'f', 2);
            } else {
                row << v.toString();
            }
        }
        result.rows.append(row);
    }
    result.count = result.rows.size();
    result.totalAmount = total;
    result.hasTotalAmount = hasAmount;
    result.ok = true;
    return result;
}

bool ReportService::writeCsv(const QString &path, const ReportResult &result,
                            QString *errorMessage)
{
    if (path.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("No se selecciono una ubicacion de guardado.");
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        if (errorMessage)
            *errorMessage = ErrorHandler::defaultMessage(ErrorHandler::Category::Persistence);
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << QChar(0xFEFF); // UTF-8 BOM for spreadsheet compatibility

    auto csvField = [](const QString &value) {
        QString v = value;
        // Neutralize CSV formula injection.
        if (!v.isEmpty() && (v[0] == QLatin1Char('=') || v[0] == QLatin1Char('+') ||
                             v[0] == QLatin1Char('-') || v[0] == QLatin1Char('@'))) {
            v.prepend(QLatin1Char('\''));
        }
        if (v.contains(QLatin1Char(',')) || v.contains(QLatin1Char('"')) ||
            v.contains(QLatin1Char('\n')) || v.contains(QLatin1Char('\r'))) {
            v.replace(QLatin1Char('"'), QStringLiteral("\"\""));
            v = QStringLiteral("\"%1\"").arg(v);
        }
        return v;
    };

    QStringList headerFields;
    for (const QString &h : result.headers)
        headerFields << csvField(h);
    out << headerFields.join(QLatin1Char(',')) << "\r\n";

    for (const QStringList &row : result.rows) {
        QStringList fields;
        for (const QString &cell : row)
            fields << csvField(cell);
        out << fields.join(QLatin1Char(',')) << "\r\n";
    }

    out.flush();
    if (out.status() != QTextStream::Ok) {
        file.close();
        if (errorMessage)
            *errorMessage = ErrorHandler::defaultMessage(ErrorHandler::Category::Persistence);
        return false;
    }
    file.close();
    return true;
}