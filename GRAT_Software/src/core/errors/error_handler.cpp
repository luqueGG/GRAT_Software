#include "error_handler.h"

#include <QDateTime>
#include <QDebug>
#include <QHash>

namespace {

QHash<QString, qint64> &announcements()
{
    static QHash<QString, qint64> entries;
    return entries;
}

} // namespace

ErrorHandler::Failure ErrorHandler::sqlFailure(const QSqlError &error, Category category,
                                               const QString &operation,
                                               const QString &userMessage)
{
    const QString technical = QStringLiteral("SQL code %1; database: %2; driver: %3")
                                  .arg(error.nativeErrorCode(),
                                       error.databaseText(),
                                       error.driverText());
    return failure(category, operation, technical, userMessage);
}

ErrorHandler::Failure ErrorHandler::exceptionFailure(const std::exception &error,
                                                     const QString &operation,
                                                     const QString &userMessage)
{
    return failure(Category::Unexpected, operation, QString::fromUtf8(error.what()), userMessage);
}

ErrorHandler::Failure ErrorHandler::unknownExceptionFailure(const QString &operation,
                                                            const QString &userMessage)
{
    return failure(Category::Unexpected, operation,
                   QStringLiteral("An unknown exception was raised."), userMessage);
}

ErrorHandler::Failure ErrorHandler::failure(Category category, const QString &operation,
                                            const QString &technicalDetail,
                                            const QString &userMessage)
{
    logTechnical(category, operation, technicalDetail);

    Failure result;
    result.category = category;
    result.message = userMessage.isEmpty() ? defaultMessage(category) : userMessage;
    return result;
}

QString ErrorHandler::defaultMessage(Category category)
{
    switch (category) {
    case Category::Validation:
        return QStringLiteral("Some entered values are not valid. "
                              "Please review the highlighted fields.");
    case Category::Connection:
        return QStringLiteral("The database is not available. "
                              "Maintenance actions are temporarily disabled.");
    case Category::Persistence:
        return QStringLiteral("The operation could not be completed. No changes were saved.");
    case Category::Reference:
        return QStringLiteral("The related records could not be loaded. "
                              "The affected field remains disabled.");
    case Category::Unexpected:
        break;
    }
    return QStringLiteral("An unexpected error occurred. The operation could not be completed.");
}

bool ErrorHandler::shouldAnnounce(const QString &key, int cooldownMs)
{
    if (key.isEmpty())
        return true;

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    QHash<QString, qint64> &entries = announcements();
    const auto it = entries.constFind(key);
    if (it != entries.constEnd() && now - it.value() < cooldownMs)
        return false;

    entries.insert(key, now);
    return true;
}

void ErrorHandler::clearAnnouncement(const QString &key)
{
    announcements().remove(key);
}

void ErrorHandler::resetAnnouncements()
{
    announcements().clear();
}

void ErrorHandler::logTechnical(Category category, const QString &operation,
                                const QString &detail)
{
    const QString line = QStringLiteral("[GRAT error] %1: %2")
                             .arg(operation.isEmpty() ? QStringLiteral("operation") : operation,
                                  detail);

    if (category == Category::Unexpected || category == Category::Persistence)
        qCritical().noquote() << line;
    else
        qWarning().noquote() << line;
}
