// test_controllers.cpp — CrudController / ChildCrudController (P1 contract)
// tested with an in-memory IRecordRepository: permissions, validation and the
// operation state machine decide whether anything is written (P3).

#include <QtTest>

#include "core/auth/auth_service.h"
#include "core/crud/controllers/child_crud_controller.h"
#include "core/crud/controllers/crud_controller.h"
#include "core/crud/validator.h"
#include "test_doubles.h"

using grat_test::FakeRecordRepository;
using grat_test::FakeValidationLookup;

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

AuthSession sessionWith(const QStringList &permissions, const QStringList &roles = {},
                        bool superuser = false, qint64 userId = 7)
{
    AuthSession session;
    session.userId = userId;
    session.username = QStringLiteral("prueba");
    session.roles = roles;
    session.isSuperuser = superuser;
    for (const QString &permission : permissions)
        session.permissions.insert(permission);
    return session;
}

// Real validation rules over an in-memory lookup: no SQLite involved.
struct ValidationHarness
{
    FakeValidationLookup lookup;
    Validator validator{lookup};
    RecordValidator function()
    {
        return [this](const TableSpec &s, const QVariantMap &v, bool insert, const QVariantMap &current) {
            return validator.validate(s, v, insert, current);
        };
    }
};

QVariantMap validCountry()
{
    return {{QStringLiteral("alpha_2"), QStringLiteral("PE")},
            {QStringLiteral("alpha_3"), QStringLiteral("PER")},
            {QStringLiteral("numerical_code"), QStringLiteral("604")},
            {QStringLiteral("name"), QStringLiteral("Peru")}};
}

QVariantMap countryRecord()
{
    QVariantMap record = validCountry();
    record.insert(QStringLiteral("is_active"), QStringLiteral("A"));
    return record;
}

QVariantMap validWaste()
{
    return {{QStringLiteral("id_waste"), QStringLiteral("W9")},
            {QStringLiteral("id_gen_company_id"), QStringLiteral("G1")},
            {QStringLiteral("amount_waste"), 3.5},
            {QStringLiteral("id_toxicity_id"), 1}};
}

const QStringList kCountryAll{QStringLiteral("view_country"), QStringLiteral("add_country"),
                              QStringLiteral("change_country")};

} // namespace

class TestControllers : public QObject
{
    Q_OBJECT

private slots:
    // Standalone CRUD
    void readingRequiresViewPermission();
    void addWithoutPermissionIsRejected();
    void saveWithoutPendingOperationWritesNothing();
    void saveRechecksPermissionsAtWriteTime();
    void failedValidationWritesNothing();
    void validAddPreparesSystemFields();
    void modifyUpdatesSelectedRecordOnly();
    void passwordsAreHashedBeforePersisting();
    void persistenceErrorIsPropagated();
    void cancelDiscardsPendingOperation();
    void stateChangesRespectRulesAndPermissions();

    // Child CRUD
    void childWithoutParentCannotReadOrWrite();
    void childWritesAreScopedToSelectedParent();
    void childRejectsRowsOfAnotherParent();
    void statefulChildRemovalIsLogical();
    void linkChildRemovalIsPhysical();
    void userAssignmentsRequireAssignRole();
    void rolePermissionsRequireAdministration();
};

void TestControllers::readingRequiresViewPermission()
{
    FakeRecordRepository repo;
    ValidationHarness harness;
    CrudController denied(spec("location_country"), sessionWith({}), repo, harness.function());
    QVERIFY(!denied.records().success);
    QVERIFY(repo.calls.isEmpty());

    repo.rows = {countryRecord()};
    CrudController allowed(spec("location_country"), sessionWith({QStringLiteral("view_country")}), repo,
                           harness.function());
    const RecordListResult result = allowed.records();
    QVERIFY(result.success);
    QCOMPARE(result.records.size(), 1);
}

void TestControllers::addWithoutPermissionIsRejected()
{
    FakeRecordRepository repo;
    ValidationHarness harness;
    CrudController controller(spec("location_country"), sessionWith({QStringLiteral("view_country")}), repo,
                              harness.function());
    QVERIFY(!controller.startAdd().success);
    QCOMPARE(controller.operation(), CrudController::Operation::Idle);
    QVERIFY(!controller.save(validCountry()).success);
    QCOMPARE(repo.writeCount(), 0);
}

void TestControllers::saveWithoutPendingOperationWritesNothing()
{
    FakeRecordRepository repo;
    ValidationHarness harness;
    CrudController controller(spec("location_country"), sessionWith(kCountryAll), repo, harness.function());
    const ControllerResult result = controller.save(validCountry());
    QVERIFY(!result.success);
    QVERIFY(result.error.contains(QStringLiteral("No hay una operacion pendiente")));
    QCOMPARE(repo.writeCount(), 0);
}

void TestControllers::saveRechecksPermissionsAtWriteTime()
{
    FakeRecordRepository repo;
    ValidationHarness harness;
    // A session that may add but not change: Modify cannot persist.
    CrudController controller(spec("location_country"),
                              sessionWith({QStringLiteral("view_country"), QStringLiteral("add_country")}),
                              repo, harness.function());
    controller.selectRecord(countryRecord());
    QVERIFY(!controller.startModify().success);
    QVERIFY(!controller.save({{QStringLiteral("name"), QStringLiteral("Otro")}}).success);
    QVERIFY(!controller.changeState(QStringLiteral("I")).success);
    QCOMPARE(repo.writeCount(), 0);
}

void TestControllers::failedValidationWritesNothing()
{
    FakeRecordRepository repo;
    ValidationHarness harness;
    CrudController controller(spec("location_country"), sessionWith(kCountryAll), repo, harness.function());
    QVERIFY(controller.startAdd().success);
    QVariantMap values = validCountry();
    values[QStringLiteral("alpha_2")] = QStringLiteral("PER");
    values[QStringLiteral("name")] = QString();
    const ControllerResult result = controller.save(values);
    QVERIFY(!result.success);
    QCOMPARE(result.validationErrors.size(), 2);
    QCOMPARE(repo.writeCount(), 0);
    // The operator can correct the form: the operation remains pending.
    QCOMPARE(controller.operation(), CrudController::Operation::Adding);
}

void TestControllers::validAddPreparesSystemFields()
{
    FakeRecordRepository repo;
    repo.nextPk = QStringLiteral("W9");
    ValidationHarness harness;
    const AuthSession session = sessionWith({QStringLiteral("view_waste"), QStringLiteral("add_waste")});
    CrudController controller(spec("waste_waste"), session, repo, harness.function());
    QVERIFY(controller.startAdd().success);

    QVariantMap values = validWaste();
    // Values the form must not be able to impose.
    values.insert(QStringLiteral("is_active"), QStringLiteral("*"));
    values.insert(QStringLiteral("created_by_id"), 999);
    values.insert(QStringLiteral("created_at"), QStringLiteral("2000-01-01T00:00:00Z"));
    const ControllerResult result = controller.save(values);
    QVERIFY2(result.success, qPrintable(result.error));
    QCOMPARE(result.pk.toString(), QStringLiteral("W9"));

    QCOMPARE(repo.writeCount(), 1);
    const auto &call = repo.calls.last();
    QCOMPARE(call.operation, QStringLiteral("add"));
    QCOMPARE(call.values.value(QStringLiteral("created_by_id")).toLongLong(), session.userId);
    QVERIFY(!call.values.contains(QStringLiteral("is_active")));
    QVERIFY(!call.values.contains(QStringLiteral("created_at")));
    QCOMPARE(call.values.value(QStringLiteral("id_waste")).toString(), QStringLiteral("W9"));
    QCOMPARE(controller.operation(), CrudController::Operation::Idle);
}

void TestControllers::modifyUpdatesSelectedRecordOnly()
{
    FakeRecordRepository repo;
    ValidationHarness harness;
    CrudController controller(spec("waste_waste"),
                              sessionWith({QStringLiteral("view_waste"), QStringLiteral("change_waste")}),
                              repo, harness.function());
    QVERIFY(!controller.startModify().success); // nothing selected

    QVariantMap record = validWaste();
    record.insert(QStringLiteral("is_active"), QStringLiteral("A"));
    controller.selectRecord(record);
    QVERIFY(controller.startModify().success);

    QVariantMap values = validWaste();
    values[QStringLiteral("id_waste")] = QStringLiteral("OTRO");   // PK is locked on modify
    values[QStringLiteral("amount_waste")] = 8.0;
    QVERIFY(controller.save(values).success);

    const auto &call = repo.calls.last();
    QCOMPARE(call.operation, QStringLiteral("change"));
    QCOMPARE(call.pk.toString(), QStringLiteral("W9"));
    QVERIFY(!call.values.contains(QStringLiteral("id_waste")));
    QCOMPARE(call.values.value(QStringLiteral("amount_waste")).toDouble(), 8.0);
    QVERIFY(call.values.contains(QStringLiteral("updated_at")));
    QVERIFY(!call.values.contains(QStringLiteral("created_by_id")));
}

void TestControllers::passwordsAreHashedBeforePersisting()
{
    FakeRecordRepository repo;
    ValidationHarness harness;
    CrudController controller(spec("user_user"),
                              sessionWith({QStringLiteral("view_user"), QStringLiteral("add_user"),
                                           QStringLiteral("change_user")}),
                              repo, harness.function());
    QVERIFY(controller.startAdd().success);
    const QVariantMap values{{QStringLiteral("password"), QStringLiteral("Clave123!")},
                             {QStringLiteral("is_superuser"), 0},
                             {QStringLiteral("username"), QStringLiteral("nuevo")},
                             {QStringLiteral("email"), QStringLiteral("nuevo@grat.local")},
                             {QStringLiteral("is_staff"), 0}};
    QVERIFY(controller.save(values).success);
    const QString stored = repo.calls.last().values.value(QStringLiteral("password")).toString();
    QVERIFY(stored != QStringLiteral("Clave123!"));
    QVERIFY(AuthService::verifyPassword(QStringLiteral("Clave123!"), stored));

    // On modify, an empty password keeps the current hash (not sent at all).
    controller.selectRecord({{QStringLiteral("id"), 4}, {QStringLiteral("username"), QStringLiteral("nuevo")}});
    QVERIFY(controller.startModify().success);
    QVERIFY(controller.save({{QStringLiteral("password"), QString()},
                             {QStringLiteral("first_name"), QStringLiteral("Ana")}}).success);
    QVERIFY(!repo.calls.last().values.contains(QStringLiteral("password")));
}

void TestControllers::persistenceErrorIsPropagated()
{
    FakeRecordRepository repo;
    repo.failWrites = true;
    repo.failMessage = QStringLiteral("No se pudo guardar el registro.");
    ValidationHarness harness;
    CrudController controller(spec("location_country"), sessionWith(kCountryAll), repo, harness.function());
    QVERIFY(controller.startAdd().success);
    const ControllerResult result = controller.save(validCountry());
    QVERIFY(!result.success);
    QCOMPARE(result.error, QStringLiteral("No se pudo guardar el registro."));
    QVERIFY(result.validationErrors.isEmpty());
    QCOMPARE(controller.operation(), CrudController::Operation::Adding);
}

void TestControllers::cancelDiscardsPendingOperation()
{
    FakeRecordRepository repo;
    ValidationHarness harness;
    CrudController controller(spec("location_country"), sessionWith(kCountryAll), repo, harness.function());
    controller.selectRecord(countryRecord());
    QVERIFY(controller.startModify().success);
    controller.cancel();
    QCOMPARE(controller.operation(), CrudController::Operation::Idle);
    QVERIFY(controller.currentRecord().isEmpty());
    QVERIFY(!controller.save({{QStringLiteral("name"), QStringLiteral("X")}}).success);
    QCOMPARE(repo.writeCount(), 0);
}

void TestControllers::stateChangesRespectRulesAndPermissions()
{
    FakeRecordRepository repo;
    ValidationHarness harness;
    CrudController controller(spec("location_country"), sessionWith(kCountryAll), repo, harness.function());
    QVERIFY(!controller.changeState(QStringLiteral("I")).success); // no selection

    controller.selectRecord(countryRecord());
    QVERIFY(!controller.changeState(QStringLiteral("X")).success);  // invalid state
    QVERIFY(controller.startModify().success);
    QVERIFY(!controller.changeState(QStringLiteral("I")).success);  // pending operation
    controller.cancel();
    QCOMPARE(repo.writeCount(), 0);

    for (const QString &state : {QStringLiteral("I"), QStringLiteral("A"), QStringLiteral("*")}) {
        controller.selectRecord(countryRecord());
        QVERIFY(controller.changeState(state).success);
        QCOMPARE(repo.calls.last().operation, QStringLiteral("setState"));
        QCOMPARE(repo.calls.last().state, state);
        QCOMPARE(repo.calls.last().pk.toString(), QStringLiteral("PE"));
    }

    // Tables without a lifecycle cannot change state.
    CrudController roles(spec("auth_group"), sessionWith({}, {QStringLiteral("Administrator")}, true), repo,
                         harness.function());
    roles.selectRecord({{QStringLiteral("id"), 1}, {QStringLiteral("name"), QStringLiteral("Operator")}});
    QVERIFY(!roles.changeState(QStringLiteral("I")).success);
}

void TestControllers::childWithoutParentCannotReadOrWrite()
{
    FakeRecordRepository repo;
    ValidationHarness harness;
    const ChildTableSpec &links = child("waste_waste", "waste_wasteconstituent");
    ChildCrudController controller(grat_test::childAsTable(links), links,
                                   sessionWith({QStringLiteral("view_wasteconstituent"),
                                                QStringLiteral("add_wasteconstituent"),
                                                QStringLiteral("change_wasteconstituent")}),
                                   repo, harness.function());
    QVERIFY(!controller.can(PermissionAction::Add));
    const RecordListResult rows = controller.records();
    QVERIFY(rows.success);
    QVERIFY(rows.records.isEmpty());
    QVERIFY(!controller.startAdd().success);
    QVERIFY(repo.calls.isEmpty());
}

void TestControllers::childWritesAreScopedToSelectedParent()
{
    FakeRecordRepository repo;
    ValidationHarness harness;
    const ChildTableSpec &links = child("waste_waste", "waste_wasteconstituent");
    ChildCrudController controller(grat_test::childAsTable(links), links,
                                   sessionWith({QStringLiteral("view_wasteconstituent"),
                                                QStringLiteral("add_wasteconstituent"),
                                                QStringLiteral("change_wasteconstituent")}),
                                   repo, harness.function());
    controller.setParentKey(QStringLiteral("W1"));
    QVERIFY(controller.records().success);
    QCOMPARE(repo.calls.last().parentKey.toString(), QStringLiteral("W1"));

    QVERIFY(controller.startAdd().success);
    // A forged parent in the form is replaced by the selected parent.
    const QVariantMap values{{QStringLiteral("id_waste_id"), QStringLiteral("W2")},
                             {QStringLiteral("id_constituent_id"), 1},
                             {QStringLiteral("amount_constituent"), 2.0}};
    QVERIFY(controller.save(values).success);
    const auto &call = repo.calls.last();
    QCOMPARE(call.operation, QStringLiteral("add"));
    QCOMPARE(call.parentKey.toString(), QStringLiteral("W1"));
    QCOMPARE(call.values.value(QStringLiteral("id_waste_id")).toString(), QStringLiteral("W1"));

    // Duplicate pair under the same parent is rejected before writing.
    harness.lookup.pairs.append({QStringLiteral("W1"), QStringLiteral("1"), QStringLiteral("100")});
    const int writes = repo.writeCount();
    QVERIFY(controller.startAdd().success);
    const ControllerResult duplicate = controller.save(values);
    QVERIFY(!duplicate.success);
    QCOMPARE(duplicate.validationErrors.size(), 1);
    QCOMPARE(repo.writeCount(), writes);
}

void TestControllers::childRejectsRowsOfAnotherParent()
{
    FakeRecordRepository repo;
    ValidationHarness harness;
    const ChildTableSpec &links = child("waste_waste", "waste_wasteconstituent");
    ChildCrudController controller(grat_test::childAsTable(links), links,
                                   sessionWith({QStringLiteral("view_wasteconstituent"),
                                                QStringLiteral("change_wasteconstituent")}),
                                   repo, harness.function());
    controller.setParentKey(QStringLiteral("W1"));
    controller.selectRecord({{QStringLiteral("id"), 5}, {QStringLiteral("id_waste_id"), QStringLiteral("W2")}});
    QVERIFY(controller.currentRecord().isEmpty());
    QVERIFY(!controller.startModify().success);
    QVERIFY(!controller.removeSelected().success);
    QCOMPARE(repo.writeCount(), 0);
}

void TestControllers::statefulChildRemovalIsLogical()
{
    FakeRecordRepository repo;
    ValidationHarness harness;
    const ChildTableSpec &links = child("waste_waste", "waste_wasteconstituent");
    ChildCrudController controller(grat_test::childAsTable(links), links,
                                   sessionWith({QStringLiteral("view_wasteconstituent"),
                                                QStringLiteral("change_wasteconstituent")}),
                                   repo, harness.function());
    controller.setParentKey(QStringLiteral("W1"));
    controller.selectRecord({{QStringLiteral("id"), 5}, {QStringLiteral("id_waste_id"), QStringLiteral("W1")},
                             {QStringLiteral("is_active"), QStringLiteral("A")}});
    QVERIFY(controller.removeSelected().success);
    QCOMPARE(repo.calls.last().operation, QStringLiteral("setState"));
    QCOMPARE(repo.calls.last().state, QStringLiteral("*"));
    QCOMPARE(repo.calls.last().parentKey.toString(), QStringLiteral("W1"));
}

void TestControllers::linkChildRemovalIsPhysical()
{
    FakeRecordRepository repo;
    ValidationHarness harness;
    const ChildTableSpec &legs = child("transfer_transfer", "transfer_transfertravelcompany");
    ChildCrudController controller(grat_test::childAsTable(legs), legs,
                                   sessionWith({QStringLiteral("view_transfertravelcompany"),
                                                QStringLiteral("change_transfertravelcompany")}),
                                   repo, harness.function());
    controller.setParentKey(QStringLiteral("T1"));
    controller.selectRecord({{QStringLiteral("id"), 3}, {QStringLiteral("id_transfer_id"), QStringLiteral("T1")}});
    QVERIFY(controller.removeSelected().success);
    QCOMPARE(repo.calls.last().operation, QStringLiteral("remove"));
    QCOMPARE(repo.calls.last().pk.toInt(), 3);
    QCOMPARE(repo.calls.last().parentKey.toString(), QStringLiteral("T1"));
}

void TestControllers::userAssignmentsRequireAssignRole()
{
    FakeRecordRepository repo;
    ValidationHarness harness;
    const ChildTableSpec &groups = child("user_user", "user_user_groups");

    // Supervisor: may view users (read-only assignments) but not assign roles.
    ChildCrudController supervisor(grat_test::childAsTable(groups), groups,
                                   sessionWith({QStringLiteral("view_user")}), repo, harness.function());
    supervisor.setParentKey(1);
    QVERIFY(supervisor.records().success);
    QVERIFY(!supervisor.can(PermissionAction::Add));
    QVERIFY(!supervisor.startAdd().success);
    QCOMPARE(repo.writeCount(), 0);

    // change_user alone is not enough either.
    ChildCrudController changeOnly(grat_test::childAsTable(groups), groups,
                                   sessionWith({QStringLiteral("view_user"), QStringLiteral("change_user")}),
                                   repo, harness.function());
    changeOnly.setParentKey(1);
    QVERIFY(!changeOnly.startAdd().success);

    ChildCrudController admin(grat_test::childAsTable(groups), groups,
                              sessionWith({QStringLiteral("view_user"), QStringLiteral("assign_role_user")}),
                              repo, harness.function());
    admin.setParentKey(1);
    QVERIFY(admin.startAdd().success);
    QVERIFY(admin.save({{QStringLiteral("group_id"), 2}}).success);
    QCOMPARE(repo.calls.last().values.value(QStringLiteral("user_id")).toInt(), 1);
}

void TestControllers::rolePermissionsRequireAdministration()
{
    FakeRecordRepository repo;
    ValidationHarness harness;
    const ChildTableSpec &perms = child("auth_group", "auth_group_permissions");

    // A non-administrator cannot even bind the role as parent.
    ChildCrudController operatorSide(grat_test::childAsTable(perms), perms,
                                     sessionWith({}, {QStringLiteral("Operator")}), repo, harness.function());
    operatorSide.setParentKey(3);
    QVERIFY(operatorSide.parentKey().isNull());
    QVERIFY(!operatorSide.startAdd().success);

    ChildCrudController admin(grat_test::childAsTable(perms), perms,
                              sessionWith({}, {QStringLiteral("Administrator")}, true, 1), repo,
                              harness.function());
    admin.setParentKey(3);
    // Writes are enabled only after the role's current permissions were read.
    QVERIFY(!admin.can(PermissionAction::Add));
    QVERIFY(admin.records().success);
    QVERIFY(admin.can(PermissionAction::Add));
    QVERIFY(!admin.canEdit());

    QVERIFY(admin.startAdd().success);
    QVERIFY(admin.save({{QStringLiteral("permission_id"), 13}}).success);
    QCOMPARE(repo.calls.last().operation, QStringLiteral("grant"));
    QCOMPARE(repo.calls.last().parentKey.toInt(), 3);
    QCOMPARE(repo.calls.last().pk.toInt(), 13);

    admin.selectRecord({{QStringLiteral("id"), 40}, {QStringLiteral("group_id"), 3}});
    QVERIFY(admin.removeSelected().success);
    QCOMPARE(repo.calls.last().operation, QStringLiteral("revoke"));
    QCOMPARE(repo.calls.last().pk.toInt(), 40);
}

QTEST_GUILESS_MAIN(TestControllers)
#include "test_controllers.moc"
