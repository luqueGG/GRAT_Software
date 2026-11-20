#include <QtTest>
#include <QFile>
#include <QTemporaryDir>

#include "core/reports/report_service.h"
#include "app/session.h"
#include "core/db/database.h"

class TestReportes : public QObject
{
    Q_OBJECT

private slots:
    void csvWritesHeadersAndRows();
    void csvCancelledPathDoesNothing();
    void permissionDeniedWithoutView();
};

void TestReportes::csvWritesHeadersAndRows()
{
    ReportResult r;
    r.ok = true;
    r.headers = {QStringLiteral("A"), QStringLiteral("B")};
    r.rows = {{QStringLiteral("1"), QStringLiteral("x")}, {QStringLiteral("2"), QStringLiteral("y,z")}};
    r.count = 2;

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + QStringLiteral("/out.csv");

    QString err;
    QVERIFY(ReportService::writeCsv(path, r, &err));
    QVERIFY(err.isEmpty());

    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString content = QString::fromUtf8(f.readAll());
    QVERIFY(content.contains(QStringLiteral("A,B")));
    QVERIFY(content.contains(QStringLiteral("1,x")));
    QVERIFY(content.contains(QStringLiteral("\"2,y,z\"")) || content.contains(QStringLiteral("2,\"y,z\"")));
}

void TestReportes::csvCancelledPathDoesNothing()
{
    ReportResult r;
    r.ok = true;
    r.headers = {QStringLiteral("A")};
    r.rows = {{QStringLiteral("1")}};
    QString err;
    QVERIFY(!ReportService::writeCsv(QString(), r, &err));
    QVERIFY(!err.isEmpty());
}

void TestReportes::permissionDeniedWithoutView()
{
    Database db;
    AuthSession session;
    session.userId = 1;
    session.username = QStringLiteral("operator");
    // no permissions granted
    ReportService service(db, session);
    const auto res = service.wasteReport(QStringLiteral(""));
    QVERIFY(!res.ok);
    QVERIFY(res.permissionDenied);
}

QTEST_MAIN(TestReportes)
#include "test_reportes.moc"