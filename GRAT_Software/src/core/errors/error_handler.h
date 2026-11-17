// error_handler.h — central conversion of technical failures into safe user
// messages (design §10, Req 9).
//
// Technical details (SQL text, driver messages, file paths) are written to the
// Qt logging stream with qWarning()/qCritical(). The UI only ever receives a
// generic, understandable message in English. Repeated announcements of the same
// condition can be suppressed with shouldAnnounce().

#pragma once

#include <QString>
#include <QSqlError>

#include <exception>

class ErrorHandler
{
public:
    enum class Category {
        Validation,   // operator input rejected before persistence
        Connection,   // the database is unavailable / access was lost
        Persistence,  // a read or write could not be completed
        Reference,    // a foreign-key list could not be loaded
        Unexpected    // an exception or otherwise unexpected failure
    };

    struct Failure
    {
        Category category = Category::Unexpected;
        QString message;   // safe for display; never contains SQL or paths
    };

    // Builds a failure from a QSqlError, logging its full technical detail.
    static Failure sqlFailure(const QSqlError &error, Category category,
                              const QString &operation,
                              const QString &userMessage = QString());

    // Builds a failure from an unexpected exception, logging its technical text.
    static Failure exceptionFailure(const std::exception &error, const QString &operation,
                                    const QString &userMessage = QString());
    static Failure unknownExceptionFailure(const QString &operation,
                                           const QString &userMessage = QString());

    // Generic builder for failures that do not originate in QSqlError.
    static Failure failure(Category category, const QString &operation,
                           const QString &technicalDetail,
                           const QString &userMessage = QString());

    // Default, non-leaking wording for each category.
    static QString defaultMessage(Category category);

    // Returns true when a condition should be announced; repeated calls with the
    // same key inside the cooldown window return false. clearAnnouncement() is
    // called when the condition recovers.
    static bool shouldAnnounce(const QString &key, int cooldownMs = 5000);
    static void clearAnnouncement(const QString &key);
    static void resetAnnouncements();

private:
    static void logTechnical(Category category, const QString &operation,
                             const QString &detail);
};
