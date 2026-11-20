// test_core_fixtures.cpp — the shared fixture builds an isolated database with
// the real schema and seed (P3).

#include <QFileInfo>
#include <QtTest>

#include "test_support.h"

using grat_test::TestDatabase;

class TestCoreFixtures : public QObject
{
    Q_OBJECT

private slots:
    void opensTemporaryDatabaseOutsideProject();
    void appliesRealSchemaAndSeed();
    void enablesForeignKeys();
    void seedsDemoUsersAndReferenceData();
    void eachFixtureStartsEmpty();
};

void TestCoreFixtures::opensTemporaryDatabaseOutsideProject()
{
    TestDatabase db;
    QVERIFY2(db.isOpen(), qPrintable(db.lastError()));
    QVERIFY(QFileInfo::exists(db.filePath()));
    // Never the developer database under the project's db/ folder.
    QVERIFY(!QDir::fromNativeSeparators(db.filePath()).contains(QStringLiteral("/GRAT_Software/db/")));
}

void TestCoreFixtures::appliesRealSchemaAndSeed()
{
    TestDatabase db;
    QVERIFY(db.isOpen());
    QCOMPARE(db.count(QStringLiteral("SELECT COUNT(*) FROM auth_group")), 3);
    QCOMPARE(db.count(QStringLiteral("SELECT COUNT(*) FROM auth_permission")), 43);
    QCOMPARE(db.count(QStringLiteral(
                 "SELECT COUNT(*) FROM auth_group_permissions gp JOIN auth_group g ON g.id = gp.group_id "
                 "WHERE g.name = 'Administrator'")), 43);
    QCOMPARE(db.count(QStringLiteral(
                 "SELECT COUNT(*) FROM auth_group_permissions gp JOIN auth_group g ON g.id = gp.group_id "
                 "WHERE g.name = 'Supervisor'")), 36);
    QCOMPARE(db.count(QStringLiteral(
                 "SELECT COUNT(*) FROM auth_group_permissions gp JOIN auth_group g ON g.id = gp.group_id "
                 "WHERE g.name = 'Operator'")), 19);
}

void TestCoreFixtures::enablesForeignKeys()
{
    TestDatabase db;
    QVERIFY(db.isOpen());
    QCOMPARE(db.scalar(QStringLiteral("PRAGMA foreign_keys")).toInt(), 1);
    QVERIFY(!db.exec(QStringLiteral(
        "INSERT INTO location_subdivision (subdiv_code, country_alpha_2_id, type_subdivision, name) "
        "VALUES ('XX-01', 'XX', 'Region', 'Sin pais')")));
}

void TestCoreFixtures::seedsDemoUsersAndReferenceData()
{
    TestDatabase db;
    QVERIFY(db.isOpen());
    QVERIFY(db.seedUsers());
    QCOMPARE(db.count(QStringLiteral("SELECT COUNT(*) FROM user_user")), 3);
    QString error;
    QVERIFY2(db.seedWasteData(&error), qPrintable(error));
    QCOMPARE(db.count(QStringLiteral("SELECT COUNT(*) FROM waste_waste")), 2);
    QCOMPARE(db.count(QStringLiteral("SELECT COUNT(*) FROM catalog_constituent")), 2);
}

void TestCoreFixtures::eachFixtureStartsEmpty()
{
    {
        TestDatabase first;
        QVERIFY(first.isOpen());
        QVERIFY(first.seedUsers());
        QCOMPARE(first.count(QStringLiteral("SELECT COUNT(*) FROM user_user")), 3);
    }
    TestDatabase second;
    QVERIFY(second.isOpen());
    QCOMPARE(second.count(QStringLiteral("SELECT COUNT(*) FROM user_user")), 0);
}

QTEST_GUILESS_MAIN(TestCoreFixtures)
#include "test_core_fixtures.moc"
