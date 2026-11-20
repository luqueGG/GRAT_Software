// test_permissions.cpp — PermissionChecker (unit) and the effective permission
// set computed by AuthService over the real schema and seed (integration) (P3).

#include <QtTest>

#include "core/auth/permission_checker.h"
#include "test_support.h"

using grat_test::TestDatabase;

namespace {

AuthSession sessionWith(const QStringList &permissions, const QStringList &roles = {},
                        bool superuser = false, qint64 userId = 1)
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

} // namespace

class TestPermissions : public QObject
{
    Q_OBJECT

private slots:
    // PermissionChecker (unit)
    void mapsActionsToCodenames_data();
    void mapsActionsToCodenames();
    void grantsOnlyPresentPermissions();
    void roleAssignmentUsesFixedCodename();
    void manageRolesByRoleOrSuperuser_data();
    void manageRolesByRoleOrSuperuser();

    // AuthService over the real seed (integration)
    void seededRolesHaveExpectedPermissionCounts_data();
    void seededRolesHaveExpectedPermissionCounts();
    void operatorPermissionsMatchTheMatrix();
    void wrongPasswordIsRejected();
    void unknownUserIsRejected();
    void inactiveOrDeletedUserCannotLogIn();
    void loginUpdatesLastLogin();
    void effectivePermissionsIncludeUserPermissions();
    void revokedRolePermissionAppliesOnNextLogin();
};

void TestPermissions::mapsActionsToCodenames_data()
{
    QTest::addColumn<int>("action");
    QTest::addColumn<QString>("expected");
    QTest::newRow("view") << int(PermissionAction::View) << QStringLiteral("view_waste");
    QTest::newRow("add") << int(PermissionAction::Add) << QStringLiteral("add_waste");
    QTest::newRow("change") << int(PermissionAction::Change) << QStringLiteral("change_waste");
}

void TestPermissions::mapsActionsToCodenames()
{
    QFETCH(int, action);
    QFETCH(QString, expected);
    QCOMPARE(PermissionChecker::codename(static_cast<PermissionAction>(action),
                                         QStringLiteral("waste"), QStringLiteral("waste")),
             expected);
}

void TestPermissions::grantsOnlyPresentPermissions()
{
    const AuthSession session = sessionWith({QStringLiteral("view_waste")});
    QVERIFY(PermissionChecker::can(session, PermissionAction::View, QStringLiteral("waste"),
                                   QStringLiteral("waste")));
    QVERIFY(!PermissionChecker::can(session, PermissionAction::Add, QStringLiteral("waste"),
                                    QStringLiteral("waste")));
    QVERIFY(!PermissionChecker::can(session, PermissionAction::View, QStringLiteral("transfer"),
                                    QStringLiteral("transfer")));
    QVERIFY(!PermissionChecker::can(AuthSession{}, PermissionAction::View, QStringLiteral("waste"),
                                    QStringLiteral("waste")));
}

void TestPermissions::roleAssignmentUsesFixedCodename()
{
    QCOMPARE(PermissionChecker::codename(PermissionAction::AssignRole, QStringLiteral("waste"),
                                         QStringLiteral("waste")),
             QStringLiteral("assign_role_user"));
    const AuthSession session = sessionWith({QStringLiteral("assign_role_user")});
    QVERIFY(PermissionChecker::can(session, PermissionAction::AssignRole, QStringLiteral("user"),
                                   QStringLiteral("user")));
    QVERIFY(!PermissionChecker::can(sessionWith({QStringLiteral("change_user")}),
                                    PermissionAction::AssignRole, QStringLiteral("user"),
                                    QStringLiteral("user")));
}

void TestPermissions::manageRolesByRoleOrSuperuser_data()
{
    QTest::addColumn<QStringList>("roles");
    QTest::addColumn<bool>("superuser");
    QTest::addColumn<qint64>("userId");
    QTest::addColumn<bool>("allowed");
    QTest::newRow("administrator") << QStringList{QStringLiteral("Administrator")} << false << qint64(1) << true;
    QTest::newRow("superuser") << QStringList{} << true << qint64(1) << true;
    QTest::newRow("supervisor") << QStringList{QStringLiteral("Supervisor")} << false << qint64(2) << false;
    QTest::newRow("operator") << QStringList{QStringLiteral("Operator")} << false << qint64(3) << false;
    QTest::newRow("invalid-session-superuser") << QStringList{QStringLiteral("Administrator")} << true
                                               << qint64(-1) << false;
}

void TestPermissions::manageRolesByRoleOrSuperuser()
{
    QFETCH(QStringList, roles);
    QFETCH(bool, superuser);
    QFETCH(qint64, userId);
    QFETCH(bool, allowed);
    QCOMPARE(PermissionChecker::canManageRoles(sessionWith({}, roles, superuser, userId)), allowed);
}

void TestPermissions::seededRolesHaveExpectedPermissionCounts_data()
{
    QTest::addColumn<QString>("username");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QString>("role");
    QTest::addColumn<int>("permissions");
    QTest::addColumn<bool>("superuser");
    QTest::newRow("admin") << QStringLiteral("admin") << QStringLiteral("Admin123!")
                           << QStringLiteral("Administrator") << 43 << true;
    QTest::newRow("supervisor") << QStringLiteral("supervisor") << QStringLiteral("Supervisor123!")
                                << QStringLiteral("Supervisor") << 36 << false;
    QTest::newRow("operator") << QStringLiteral("operator") << QStringLiteral("Operator123!")
                              << QStringLiteral("Operator") << 19 << false;
}

void TestPermissions::seededRolesHaveExpectedPermissionCounts()
{
    QFETCH(QString, username);
    QFETCH(QString, password);
    QFETCH(QString, role);
    QFETCH(int, permissions);
    QFETCH(bool, superuser);

    TestDatabase db;
    QVERIFY2(db.isOpen(), qPrintable(db.lastError()));
    QVERIFY(db.seedUsers());
    const auto session = db.login(username, password);
    QVERIFY(session.has_value());
    QVERIFY(session->isValid());
    QCOMPARE(session->username, username);
    QCOMPARE(session->roles, QStringList{role});
    QCOMPARE(session->permissions.size(), permissions);
    QCOMPARE(session->isSuperuser, superuser);
}

void TestPermissions::operatorPermissionsMatchTheMatrix()
{
    TestDatabase db;
    QVERIFY(db.isOpen());
    QVERIFY(db.seedUsers());
    const auto op = db.login(QStringLiteral("operator"), QStringLiteral("Operator123!"));
    QVERIFY(op.has_value());
    QVERIFY(PermissionChecker::can(*op, PermissionAction::Add, QStringLiteral("waste"), QStringLiteral("waste")));
    QVERIFY(PermissionChecker::can(*op, PermissionAction::View, QStringLiteral("location"),
                                   QStringLiteral("country")));
    QVERIFY(!PermissionChecker::can(*op, PermissionAction::Add, QStringLiteral("location"),
                                    QStringLiteral("country")));
    QVERIFY(!PermissionChecker::can(*op, PermissionAction::View, QStringLiteral("user"), QStringLiteral("user")));
    QVERIFY(!PermissionChecker::can(*op, PermissionAction::AssignRole, QStringLiteral("user"),
                                    QStringLiteral("user")));
    QVERIFY(!PermissionChecker::canManageRoles(*op));
}

void TestPermissions::wrongPasswordIsRejected()
{
    TestDatabase db;
    QVERIFY(db.isOpen());
    QVERIFY(db.seedUsers());
    const AuthService auth(db.connection());
    QString message;
    QVERIFY(!auth.login(QStringLiteral("admin"), QStringLiteral("admin123!"), &message).has_value());
    QVERIFY(!message.isEmpty());
    // The message must not reveal SQL or schema details.
    QVERIFY(!message.contains(QStringLiteral("user_user")));
    QVERIFY(!message.contains(QStringLiteral("SELECT"), Qt::CaseInsensitive));
}

void TestPermissions::unknownUserIsRejected()
{
    TestDatabase db;
    QVERIFY(db.isOpen());
    QVERIFY(db.seedUsers());
    QVERIFY(!db.login(QStringLiteral("nadie"), QStringLiteral("Admin123!")).has_value());
}

void TestPermissions::inactiveOrDeletedUserCannotLogIn()
{
    TestDatabase db;
    QVERIFY(db.isOpen());
    QVERIFY(db.seedUsers());
    QVERIFY(db.exec(QStringLiteral("UPDATE user_user SET is_active = 'I' WHERE username = 'operator'")));
    QVERIFY(!db.login(QStringLiteral("operator"), QStringLiteral("Operator123!")).has_value());
    QVERIFY(db.exec(QStringLiteral("UPDATE user_user SET is_active = '*' WHERE username = 'operator'")));
    QVERIFY(!db.login(QStringLiteral("operator"), QStringLiteral("Operator123!")).has_value());
    QVERIFY(db.exec(QStringLiteral("UPDATE user_user SET is_active = 'A' WHERE username = 'operator'")));
    QVERIFY(db.login(QStringLiteral("operator"), QStringLiteral("Operator123!")).has_value());
}

void TestPermissions::loginUpdatesLastLogin()
{
    TestDatabase db;
    QVERIFY(db.isOpen());
    QVERIFY(db.seedUsers());
    const QString lastLoginSql = QStringLiteral("SELECT last_login FROM user_user WHERE username = 'supervisor'");
    QVERIFY(db.scalar(lastLoginSql).toString().isEmpty());
    QVERIFY(db.login(QStringLiteral("supervisor"), QStringLiteral("Supervisor123!")).has_value());
    QVERIFY(!db.scalar(lastLoginSql).toString().isEmpty());
}

void TestPermissions::effectivePermissionsIncludeUserPermissions()
{
    TestDatabase db;
    QVERIFY(db.isOpen());
    QVERIFY(db.seedUsers());
    // add_country is not in the Operator role; grant it directly to the user.
    QVERIFY(db.exec(QStringLiteral(
        "INSERT INTO user_user_user_permissions (user_id, permission_id) "
        "SELECT u.id, p.id FROM user_user u, auth_permission p "
        "WHERE u.username = 'operator' AND p.codename = 'add_country'")));
    // view_waste is already granted by the role: the union must not double count.
    QVERIFY(db.exec(QStringLiteral(
        "INSERT INTO user_user_user_permissions (user_id, permission_id) "
        "SELECT u.id, p.id FROM user_user u, auth_permission p "
        "WHERE u.username = 'operator' AND p.codename = 'view_waste'")));
    const auto op = db.login(QStringLiteral("operator"), QStringLiteral("Operator123!"));
    QVERIFY(op.has_value());
    QCOMPARE(op->permissions.size(), 20);
    QVERIFY(op->permissions.contains(QStringLiteral("add_country")));
}

void TestPermissions::revokedRolePermissionAppliesOnNextLogin()
{
    TestDatabase db;
    QVERIFY(db.isOpen());
    QVERIFY(db.seedUsers());
    const auto before = db.login(QStringLiteral("operator"), QStringLiteral("Operator123!"));
    QVERIFY(before.has_value());
    QVERIFY(before->permissions.contains(QStringLiteral("add_waste")));

    QVERIFY(db.exec(QStringLiteral(
        "DELETE FROM auth_group_permissions WHERE group_id = (SELECT id FROM auth_group WHERE name = 'Operator') "
        "AND permission_id = (SELECT id FROM auth_permission WHERE codename = 'add_waste')")));

    // An existing session keeps its snapshot; the next login sees the change.
    QVERIFY(before->permissions.contains(QStringLiteral("add_waste")));
    const auto after = db.login(QStringLiteral("operator"), QStringLiteral("Operator123!"));
    QVERIFY(after.has_value());
    QCOMPARE(after->permissions.size(), 18);
    QVERIFY(!after->permissions.contains(QStringLiteral("add_waste")));
}

QTEST_GUILESS_MAIN(TestPermissions)
#include "test_permissions.moc"
