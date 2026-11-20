// test_gateway.cpp — TableGateway and SqlRecordRepository over a temporary
// SQLite file with the real schema: persistence, lifecycle states, parent/child
// isolation, foreign keys, cascades and rollback (P3, integration).

#include <QtTest>

#include "core/crud/repositories/sql_record_repository.h"
#include "core/crud/table_gateway.h"
#include "test_doubles.h"
#include "test_support.h"

using grat_test::TestDatabase;

namespace {

const TableSpec &spec(const char *table)
{
    const TableSpec *found = findTableSpec(QString::fromLatin1(table));
    Q_ASSERT(found);
    return *found;
}

const ChildTableSpec &child(const char *parent, const char *table)
{
    const ChildTableSpec *found = grat_test::findChildSpec(spec(parent), table);
    Q_ASSERT(found);
    return *found;
}

QVariantMap country(const QString &a2, const QString &a3, const QString &name)
{
    return {{QStringLiteral("alpha_2"), a2},
            {QStringLiteral("alpha_3"), a3},
            {QStringLiteral("numerical_code"), QStringLiteral("000")},
            {QStringLiteral("name"), name}};
}

QVariantMap constituentLink(int constituent, double amount)
{
    return {{QStringLiteral("id_constituent_id"), constituent},
            {QStringLiteral("amount_constituent"), amount}};
}

AuthSession administrator()
{
    AuthSession session;
    session.userId = 1;
    session.username = QStringLiteral("admin");
    session.roles = {QStringLiteral("Administrator")};
    session.isSuperuser = true;
    return session;
}

// The visible error text must never expose SQL or schema details (Req 9.2).
bool leaksInternals(const QString &message)
{
    static const QStringList markers{QStringLiteral("SQL"), QStringLiteral("UNIQUE"),
                                     QStringLiteral("constraint"), QStringLiteral("FOREIGN"),
                                     QStringLiteral("INSERT"), QStringLiteral("UPDATE"),
                                     QStringLiteral("location_"), QStringLiteral("waste_")};
    for (const QString &marker : markers) {
        if (message.contains(marker, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

} // namespace

class TestGateway : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<TestDatabase> m_db;

private slots:
    void init();
    void cleanup();

    // Standalone tables
    void insertSelectUpdateRoundTrip();
    void selectAllReturnsEveryState();
    void stateTransitions();
    void updateNeverChangesKeyOrState();
    void missingRecordIsReported();
    void duplicateKeyRollsBackWithSafeMessage();
    void foreignKeyViolationIsRejected();
    void negativeAmountViolatesCheck();
    void managedTablesAreNeverPhysicallyDeleted();

    // Child rows
    void childrenAreIsolatedByParent();
    void childCannotBeReassignedToAnotherParent();
    void childWriteWithWrongParentChangesNothing();
    void statefulChildRemovalIsLogical();
    void linkChildRemovalIsPhysical();
    void duplicateLinkRollsBack();
    void parentDeletionCascades();

    // Role permissions
    void rolePermissionWritesRequireAdministration();

    // Repository adapter (P1)
    void repositoryDistinguishesEmptyFromError();
    void repositoryScopesChildCollections();
    void repositoryRefusesInvalidOperations();
};

void TestGateway::init()
{
    m_db = std::make_unique<TestDatabase>();
    QVERIFY2(m_db->isOpen(), qPrintable(m_db->lastError()));
    QVERIFY(m_db->seedUsers());
}

void TestGateway::cleanup()
{
    m_db.reset();
}

void TestGateway::insertSelectUpdateRoundTrip()
{
    const TableGateway gateway(m_db->connection());
    const TableSpec &countries = spec("location_country");

    const DbResult inserted = gateway.insert(countries, country(QStringLiteral("PE"), QStringLiteral("PER"),
                                                                QStringLiteral("Peru")));
    QVERIFY2(inserted.success, qPrintable(inserted.error));
    QCOMPARE(inserted.pk.toString(), QStringLiteral("PE"));

    QList<QVariantMap> rows = gateway.selectAll(countries);
    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows.first().value(QStringLiteral("name")).toString(), QStringLiteral("Peru"));
    QCOMPARE(rows.first().value(QStringLiteral("is_active")).toString(), QStringLiteral("A")); // DB default

    QVERIFY(gateway.update(countries, QStringLiteral("PE"),
                           {{QStringLiteral("name"), QStringLiteral("Republica del Peru")}}).success);
    rows = gateway.selectAll(countries);
    QCOMPARE(rows.first().value(QStringLiteral("name")).toString(), QStringLiteral("Republica del Peru"));
    QCOMPARE(rows.first().value(QStringLiteral("alpha_3")).toString(), QStringLiteral("PER"));

    // Identity tables return the generated key.
    const DbResult generated = gateway.insert(spec("catalog_constituent"),
                                              {{QStringLiteral("name"), QStringLiteral("Plomo")}});
    QVERIFY(generated.success);
    QVERIFY(generated.pk.toLongLong() > 0);
}

void TestGateway::selectAllReturnsEveryState()
{
    const TableGateway gateway(m_db->connection());
    const TableSpec &countries = spec("location_country");
    QVERIFY(gateway.insert(countries, country(QStringLiteral("AA"), QStringLiteral("AAA"), QStringLiteral("A"))).success);
    QVERIFY(gateway.insert(countries, country(QStringLiteral("BB"), QStringLiteral("BBB"), QStringLiteral("B"))).success);
    QVERIFY(gateway.insert(countries, country(QStringLiteral("CC"), QStringLiteral("CCC"), QStringLiteral("C"))).success);
    QVERIFY(gateway.setState(countries, QStringLiteral("BB"), QStringLiteral("I")).success);
    QVERIFY(gateway.setState(countries, QStringLiteral("CC"), QStringLiteral("*")).success);

    QString error;
    const QList<QVariantMap> rows = gateway.selectAll(countries, &error);
    QVERIFY(error.isEmpty());
    QCOMPARE(rows.size(), 3);
    QStringList states;
    for (const QVariantMap &row : rows)
        states << row.value(QStringLiteral("is_active")).toString();
    QCOMPARE(states, (QStringList{QStringLiteral("A"), QStringLiteral("I"), QStringLiteral("*")}));
}

void TestGateway::stateTransitions()
{
    const TableGateway gateway(m_db->connection());
    const TableSpec &countries = spec("location_country");
    QVERIFY(gateway.insert(countries, country(QStringLiteral("PE"), QStringLiteral("PER"), QStringLiteral("Peru"))).success);
    const QString stateSql = QStringLiteral("SELECT is_active FROM location_country WHERE alpha_2 = 'PE'");

    QVERIFY(gateway.setState(countries, QStringLiteral("PE"), QStringLiteral("I")).success);
    QCOMPARE(m_db->scalar(stateSql).toString(), QStringLiteral("I"));
    QVERIFY(gateway.setState(countries, QStringLiteral("PE"), QStringLiteral("A")).success);
    QCOMPARE(m_db->scalar(stateSql).toString(), QStringLiteral("A"));
    QVERIFY(gateway.setState(countries, QStringLiteral("PE"), QStringLiteral("*")).success);
    QCOMPARE(m_db->scalar(stateSql).toString(), QStringLiteral("*"));
    QCOMPARE(m_db->count(QStringLiteral("SELECT COUNT(*) FROM location_country")), 1);
}

void TestGateway::updateNeverChangesKeyOrState()
{
    const TableGateway gateway(m_db->connection());
    const TableSpec &countries = spec("location_country");
    QVERIFY(gateway.insert(countries, country(QStringLiteral("PE"), QStringLiteral("PER"), QStringLiteral("Peru"))).success);
    QVERIFY(gateway.update(countries, QStringLiteral("PE"),
                           {{QStringLiteral("alpha_2"), QStringLiteral("XX")},
                            {QStringLiteral("is_active"), QStringLiteral("*")},
                            {QStringLiteral("name"), QStringLiteral("Peru 2")}}).success);
    QCOMPARE(m_db->count(QStringLiteral("SELECT COUNT(*) FROM location_country WHERE alpha_2 = 'PE' "
                                        "AND is_active = 'A' AND name = 'Peru 2'")), 1);
}

void TestGateway::missingRecordIsReported()
{
    const TableGateway gateway(m_db->connection());
    const TableSpec &countries = spec("location_country");
    QVERIFY(!gateway.update(countries, QStringLiteral("ZZ"), {{QStringLiteral("name"), QStringLiteral("X")}}).success);
    QVERIFY(!gateway.setState(countries, QStringLiteral("ZZ"), QStringLiteral("I")).success);
}

void TestGateway::duplicateKeyRollsBackWithSafeMessage()
{
    const TableGateway gateway(m_db->connection());
    const TableSpec &countries = spec("location_country");
    QVERIFY(gateway.insert(countries, country(QStringLiteral("PE"), QStringLiteral("PER"), QStringLiteral("Peru"))).success);

    const DbResult duplicate = gateway.insert(countries, country(QStringLiteral("PE"), QStringLiteral("XXX"),
                                                                 QStringLiteral("Otro")));
    QVERIFY(!duplicate.success);
    QVERIFY(!duplicate.error.isEmpty());
    QVERIFY2(!leaksInternals(duplicate.error), qPrintable(duplicate.error));
    QCOMPARE(m_db->count(QStringLiteral("SELECT COUNT(*) FROM location_country")), 1);

    // The connection is not left inside an open transaction: later writes work.
    QVERIFY(gateway.insert(countries, country(QStringLiteral("CL"), QStringLiteral("CHL"), QStringLiteral("Chile"))).success);
    QCOMPARE(m_db->count(QStringLiteral("SELECT COUNT(*) FROM location_country")), 2);
}

void TestGateway::foreignKeyViolationIsRejected()
{
    const TableGateway gateway(m_db->connection());
    const DbResult result = gateway.insert(spec("location_subdivision"),
                                           {{QStringLiteral("subdiv_code"), QStringLiteral("XX-01")},
                                            {QStringLiteral("country_alpha_2_id"), QStringLiteral("XX")},
                                            {QStringLiteral("type_subdivision"), QStringLiteral("Region")},
                                            {QStringLiteral("name"), QStringLiteral("Sin pais")}});
    QVERIFY(!result.success);
    QVERIFY2(!leaksInternals(result.error), qPrintable(result.error));
    QCOMPARE(m_db->count(QStringLiteral("SELECT COUNT(*) FROM location_subdivision")), 0);
}

void TestGateway::negativeAmountViolatesCheck()
{
    QString error;
    QVERIFY2(m_db->seedWasteData(&error), qPrintable(error));
    const TableGateway gateway(m_db->connection());
    const DbResult result = gateway.insert(spec("waste_waste"),
                                           {{QStringLiteral("id_waste"), QStringLiteral("W9")},
                                            {QStringLiteral("id_gen_company_id"), QStringLiteral("G1")},
                                            {QStringLiteral("amount_waste"), -1.0},
                                            {QStringLiteral("id_toxicity_id"), 1},
                                            {QStringLiteral("created_by_id"), 1}});
    QVERIFY(!result.success);
    QCOMPARE(m_db->count(QStringLiteral("SELECT COUNT(*) FROM waste_waste WHERE id_waste = 'W9'")), 0);
}

void TestGateway::managedTablesAreNeverPhysicallyDeleted()
{
    const TableGateway gateway(m_db->connection());
    const TableSpec &countries = spec("location_country");
    QVERIFY(gateway.insert(countries, country(QStringLiteral("PE"), QStringLiteral("PER"), QStringLiteral("Peru"))).success);
    QVERIFY(!gateway.remove(countries, QStringLiteral("PE")).success);
    QCOMPARE(m_db->count(QStringLiteral("SELECT COUNT(*) FROM location_country")), 1);
}

void TestGateway::childrenAreIsolatedByParent()
{
    QString error;
    QVERIFY2(m_db->seedWasteData(&error), qPrintable(error));
    const TableGateway gateway(m_db->connection());
    const ChildTableSpec &links = child("waste_waste", "waste_wasteconstituent");

    QVERIFY(gateway.insertChild(links, QStringLiteral("W1"), constituentLink(1, 2.0)).success);
    QVERIFY(gateway.insertChild(links, QStringLiteral("W1"), constituentLink(2, 3.0)).success);
    QVERIFY(gateway.insertChild(links, QStringLiteral("W2"), constituentLink(1, 4.0)).success);

    const QList<QVariantMap> w1 = gateway.selectChildren(links, QStringLiteral("W1"));
    QCOMPARE(w1.size(), 2);
    for (const QVariantMap &row : w1)
        QCOMPARE(row.value(QStringLiteral("id_waste_id")).toString(), QStringLiteral("W1"));
    QCOMPARE(gateway.selectChildren(links, QStringLiteral("W2")).size(), 1);
    QCOMPARE(gateway.selectChildren(links, QStringLiteral("W3")).size(), 0);

    // A child insert needs a saved parent.
    QVERIFY(!gateway.insertChild(links, QVariant(), constituentLink(1, 1.0)).success);
}

void TestGateway::childCannotBeReassignedToAnotherParent()
{
    QString error;
    QVERIFY2(m_db->seedWasteData(&error), qPrintable(error));
    const TableGateway gateway(m_db->connection());
    const ChildTableSpec &links = child("waste_waste", "waste_wasteconstituent");
    const DbResult inserted = gateway.insertChild(links, QStringLiteral("W1"), constituentLink(1, 2.0));
    QVERIFY(inserted.success);

    QVariantMap values = constituentLink(1, 9.0);
    values.insert(QStringLiteral("id_waste_id"), QStringLiteral("W2"));
    QVERIFY(gateway.updateChild(links, QStringLiteral("W1"), inserted.pk, values).success);
    QCOMPARE(m_db->scalar(QStringLiteral("SELECT id_waste_id FROM waste_wasteconstituent WHERE id = %1")
                              .arg(inserted.pk.toLongLong())).toString(),
             QStringLiteral("W1"));
    QCOMPARE(m_db->scalar(QStringLiteral("SELECT amount_constituent FROM waste_wasteconstituent WHERE id = %1")
                              .arg(inserted.pk.toLongLong())).toDouble(),
             9.0);
}

void TestGateway::childWriteWithWrongParentChangesNothing()
{
    QString error;
    QVERIFY2(m_db->seedWasteData(&error), qPrintable(error));
    const TableGateway gateway(m_db->connection());
    const ChildTableSpec &links = child("waste_waste", "waste_wasteconstituent");
    const DbResult inserted = gateway.insertChild(links, QStringLiteral("W1"), constituentLink(1, 2.0));
    QVERIFY(inserted.success);

    QVERIFY(!gateway.updateChild(links, QStringLiteral("W2"), inserted.pk, constituentLink(1, 7.0)).success);
    QVERIFY(!gateway.setChildState(links, QStringLiteral("W2"), inserted.pk, QStringLiteral("*")).success);
    QCOMPARE(m_db->count(QStringLiteral("SELECT COUNT(*) FROM waste_wasteconstituent "
                                        "WHERE amount_constituent = 2 AND is_active = 'A'")), 1);
}

void TestGateway::statefulChildRemovalIsLogical()
{
    QString error;
    QVERIFY2(m_db->seedWasteData(&error), qPrintable(error));
    const TableGateway gateway(m_db->connection());
    const ChildTableSpec &links = child("waste_waste", "waste_wasteconstituent");
    const DbResult inserted = gateway.insertChild(links, QStringLiteral("W1"), constituentLink(1, 2.0));
    QVERIFY(inserted.success);

    QVERIFY(gateway.setChildState(links, QStringLiteral("W1"), inserted.pk, QStringLiteral("I")).success);
    QVERIFY(!gateway.setChildState(links, QStringLiteral("W1"), inserted.pk, QStringLiteral("X")).success);
    QVERIFY(gateway.removeChild(links, QStringLiteral("W1"), inserted.pk).success);
    QCOMPARE(m_db->scalar(QStringLiteral("SELECT is_active FROM waste_wasteconstituent WHERE id = %1")
                              .arg(inserted.pk.toLongLong())).toString(),
             QStringLiteral("*"));
    QCOMPARE(gateway.selectChildren(links, QStringLiteral("W1")).size(), 1);
}

void TestGateway::linkChildRemovalIsPhysical()
{
    const TableGateway gateway(m_db->connection());
    const ChildTableSpec &groups = child("user_user", "user_user_groups");
    const qint64 operatorId = m_db->scalar(QStringLiteral("SELECT id FROM user_user WHERE username = 'operator'")).toLongLong();
    const QList<QVariantMap> before = gateway.selectChildren(groups, operatorId);
    QCOMPARE(before.size(), 1);

    const DbResult added = gateway.insertChild(groups, operatorId, {{QStringLiteral("group_id"), 2}});
    QVERIFY2(added.success, qPrintable(added.error));
    QCOMPARE(gateway.selectChildren(groups, operatorId).size(), 2);
    QVERIFY(gateway.removeChild(groups, operatorId, added.pk).success);
    QCOMPARE(gateway.selectChildren(groups, operatorId).size(), 1);
    QCOMPARE(m_db->count(QStringLiteral("SELECT COUNT(*) FROM user_user_groups WHERE id = %1")
                             .arg(added.pk.toLongLong())), 0);
}

void TestGateway::duplicateLinkRollsBack()
{
    const TableGateway gateway(m_db->connection());
    const ChildTableSpec &groups = child("user_user", "user_user_groups");
    const qint64 operatorId = m_db->scalar(QStringLiteral("SELECT id FROM user_user WHERE username = 'operator'")).toLongLong();
    const qint64 operatorGroup = m_db->scalar(QStringLiteral("SELECT id FROM auth_group WHERE name = 'Operator'")).toLongLong();
    const int before = m_db->count(QStringLiteral("SELECT COUNT(*) FROM user_user_groups"));

    const DbResult duplicate = gateway.insertChild(groups, operatorId, {{QStringLiteral("group_id"), operatorGroup}});
    QVERIFY(!duplicate.success);
    QVERIFY2(!leaksInternals(duplicate.error), qPrintable(duplicate.error));
    QCOMPARE(m_db->count(QStringLiteral("SELECT COUNT(*) FROM user_user_groups")), before);
}

void TestGateway::parentDeletionCascades()
{
    QString error;
    QVERIFY2(m_db->seedWasteData(&error), qPrintable(error));
    const TableGateway gateway(m_db->connection());
    const ChildTableSpec &links = child("waste_waste", "waste_wasteconstituent");
    QVERIFY(gateway.insertChild(links, QStringLiteral("W1"), constituentLink(1, 2.0)).success);
    QVERIFY(gateway.insertChild(links, QStringLiteral("W2"), constituentLink(1, 2.0)).success);

    // ON DELETE CASCADE from waste to its constituents.
    QVERIFY(m_db->exec(QStringLiteral("DELETE FROM waste_waste WHERE id_waste = 'W1'")));
    QCOMPARE(m_db->count(QStringLiteral("SELECT COUNT(*) FROM waste_wasteconstituent WHERE id_waste_id = 'W1'")), 0);
    QCOMPARE(m_db->count(QStringLiteral("SELECT COUNT(*) FROM waste_wasteconstituent WHERE id_waste_id = 'W2'")), 1);

    // And transitively: country → subdivision → generator → waste → constituents.
    QVERIFY(m_db->exec(QStringLiteral("DELETE FROM location_country WHERE alpha_2 = 'PE'")));
    QCOMPARE(m_db->count(QStringLiteral("SELECT COUNT(*) FROM location_subdivision")), 0);
    QCOMPARE(m_db->count(QStringLiteral("SELECT COUNT(*) FROM waste_generatorcompany")), 0);
    QCOMPARE(m_db->count(QStringLiteral("SELECT COUNT(*) FROM waste_waste")), 0);
    QCOMPARE(m_db->count(QStringLiteral("SELECT COUNT(*) FROM waste_wasteconstituent")), 0);
}

void TestGateway::rolePermissionWritesRequireAdministration()
{
    const TableGateway gateway(m_db->connection());
    const qint64 operatorGroup = m_db->scalar(QStringLiteral("SELECT id FROM auth_group WHERE name = 'Operator'")).toLongLong();
    const qint64 addCountry = m_db->scalar(QStringLiteral("SELECT id FROM auth_permission WHERE codename = 'add_country'")).toLongLong();
    const QString countSql = QStringLiteral("SELECT COUNT(*) FROM auth_group_permissions WHERE group_id = %1").arg(operatorGroup);
    QCOMPARE(m_db->count(countSql), 19);

    AuthSession supervisor;
    supervisor.userId = 2;
    supervisor.roles = {QStringLiteral("Supervisor")};
    QVERIFY(!gateway.grantRolePermission(supervisor, operatorGroup, addCountry).success);
    QCOMPARE(m_db->count(countSql), 19);

    const DbResult granted = gateway.grantRolePermission(administrator(), operatorGroup, addCountry);
    QVERIFY2(granted.success, qPrintable(granted.error));
    QCOMPARE(m_db->count(countSql), 20);
    // The permission catalog itself is never modified.
    QCOMPARE(m_db->count(QStringLiteral("SELECT COUNT(*) FROM auth_permission")), 43);

    const qint64 link = m_db->scalar(QStringLiteral("SELECT id FROM auth_group_permissions WHERE group_id = %1 "
                                                    "AND permission_id = %2").arg(operatorGroup).arg(addCountry)).toLongLong();
    QVERIFY(!gateway.revokeRolePermission(supervisor, operatorGroup, link).success);
    QVERIFY(gateway.revokeRolePermission(administrator(), operatorGroup, link).success);
    QCOMPARE(m_db->count(countSql), 19);
}

void TestGateway::repositoryDistinguishesEmptyFromError()
{
    TableGateway gateway(m_db->connection());
    const SqlRecordRepository repository(gateway, spec("location_country"));

    const RecordListResult empty = repository.load();
    QVERIFY(empty.success);
    QVERIFY(empty.records.isEmpty());

    m_db->database().close();
    const RecordListResult failed = repository.load();
    QVERIFY(!failed.success);
    QVERIFY(!failed.error.isEmpty());
    QVERIFY2(!leaksInternals(failed.error), qPrintable(failed.error));
}

void TestGateway::repositoryScopesChildCollections()
{
    QString error;
    QVERIFY2(m_db->seedWasteData(&error), qPrintable(error));
    TableGateway gateway(m_db->connection());
    const ChildTableSpec &links = child("waste_waste", "waste_wasteconstituent");
    const SqlRecordRepository repository(gateway, grat_test::childAsTable(links), links);

    const RecordWriteResult added = repository.add(constituentLink(1, 2.0), QStringLiteral("W1"));
    QVERIFY2(added.success, qPrintable(added.error));
    QVERIFY(repository.add(constituentLink(2, 1.0), QStringLiteral("W2")).success);

    const RecordListResult w1 = repository.load(QStringLiteral("W1"));
    QVERIFY(w1.success);
    QCOMPARE(w1.records.size(), 1);

    QVERIFY(repository.change(added.pk, constituentLink(1, 6.0), QStringLiteral("W1")).success);
    QVERIFY(!repository.change(added.pk, constituentLink(1, 7.0), QStringLiteral("W2")).success);
    QVERIFY(repository.remove(added.pk, QStringLiteral("W1")).success); // logical for this child
    const RecordListResult after = repository.load(QStringLiteral("W1"));
    QCOMPARE(after.records.first().value(QStringLiteral("is_active")).toString(), QStringLiteral("*"));
    QCOMPARE(after.records.first().value(QStringLiteral("amount_constituent")).toDouble(), 6.0);
}

void TestGateway::repositoryRefusesInvalidOperations()
{
    TableGateway gateway(m_db->connection());
    const SqlRecordRepository countries(gateway, spec("location_country"));
    QVERIFY(gateway.insert(spec("location_country"),
                           country(QStringLiteral("PE"), QStringLiteral("PER"), QStringLiteral("Peru"))).success);
    // Standalone records keep their lifecycle: no physical removal.
    QVERIFY(!countries.remove(QStringLiteral("PE")).success);
    QVERIFY(!countries.grantPermission(administrator(), 1, 1).success);
    QCOMPARE(m_db->count(QStringLiteral("SELECT COUNT(*) FROM location_country")), 1);

    // Role permissions are granted/revoked only through the authorized path.
    const ChildTableSpec &perms = child("auth_group", "auth_group_permissions");
    const SqlRecordRepository rolePerms(gateway, grat_test::childAsTable(perms), perms);
    const int before = m_db->count(QStringLiteral("SELECT COUNT(*) FROM auth_group_permissions"));
    QVERIFY(!rolePerms.add({{QStringLiteral("permission_id"), 13}}, 3).success);
    QVERIFY(!rolePerms.remove(1, 1).success);
    QCOMPARE(m_db->count(QStringLiteral("SELECT COUNT(*) FROM auth_group_permissions")), before);
}

QTEST_GUILESS_MAIN(TestGateway)
#include "test_gateway.moc"
