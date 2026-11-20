// test_validator.cpp — Validator and its strategy rules (P2 contract), tested
// with an in-memory IValidationLookup (no SQLite) plus a few integration cases
// through the real TableGateway lookup (P3).

#include <QtTest>

#include "core/crud/table_gateway.h"
#include "core/crud/validation/table_gateway_validation_lookup.h"
#include "core/crud/validator.h"
#include "test_doubles.h"
#include "test_support.h"

using grat_test::FakeValidationLookup;
using grat_test::TestDatabase;

namespace {

const TableSpec &spec(const char *table)
{
    const TableSpec *found = findTableSpec(QString::fromLatin1(table));
    Q_ASSERT(found);
    return *found;
}

TableSpec constituentSpec()
{
    const ChildTableSpec *child = grat_test::findChildSpec(spec("waste_waste"), "waste_wasteconstituent");
    Q_ASSERT(child);
    return grat_test::childAsTable(*child);
}

QStringList columns(const QList<ValidationError> &errors)
{
    QStringList names;
    for (const ValidationError &error : errors)
        names << error.column;
    return names;
}

QVariantMap validCountry()
{
    return {{QStringLiteral("alpha_2"), QStringLiteral("PE")},
            {QStringLiteral("alpha_3"), QStringLiteral("PER")},
            {QStringLiteral("numerical_code"), QStringLiteral("604")},
            {QStringLiteral("name"), QStringLiteral("Peru")}};
}

QVariantMap validUser()
{
    return {{QStringLiteral("password"), QStringLiteral("Clave123!")},
            {QStringLiteral("is_superuser"), 0},
            {QStringLiteral("username"), QStringLiteral("nuevo")},
            {QStringLiteral("email"), QStringLiteral("nuevo@grat.local")},
            {QStringLiteral("is_staff"), 0}};
}

// A strategy injected from outside to prove Validator runs rules polymorphically.
class RejectNameRule final : public IValidationRule
{
public:
    mutable int calls = 0;
    void validate(const ValidationContext &context, QList<ValidationError> &errors) const override
    {
        if (!context.field || context.field->columnName != QLatin1String("name"))
            return;
        ++calls;
        if (context.field->text == QLatin1String("Prohibido"))
            errors.append({context.field->columnName, QStringLiteral("Nombre rechazado.")});
    }
};

} // namespace

class TestValidator : public QObject
{
    Q_OBJECT

private slots:
    // Field rules
    void acceptsValidRecord();
    void reportsEveryMissingRequiredFieldTogether();
    void maxLengthAcceptsExactLimit();
    void maxLengthRejectsExcess();
    void decimalAcceptsZeroRejectsNegative();
    void emailFormat_data();
    void emailFormat();
    void reportsSimultaneousErrorsOfDifferentKinds();
    void partialEditValidatesOnlySubmittedFields();
    void partialEditRejectsClearedRequiredField();
    void passwordRequiredOnInsertOptionalOnEdit();
    void skipsSystemManagedFields();

    // Uniqueness through IValidationLookup (fake)
    void naturalKeyDuplicateIsRejected();
    void naturalKeyCheckedOnlyOnInsert();
    void identityTablesSkipKeyLookup();
    void lookupFailureIsNotTreatedAsUnique();
    void duplicateConstituentPairIsRejected();
    void constituentAllowedInAnotherWaste();
    void editExcludesOwnRowFromDuplicates();
    void partialChildEditUsesOriginalValues();

    // Strategy composition
    void injectedRuleRunsThroughInterface();
    void emptyRuleSetAcceptsAnything();

    // Integration with the real gateway lookup
    void gatewayLookupDetectsExistingNaturalKey();
    void gatewayLookupMatchesPairsInEveryState();
};

void TestValidator::acceptsValidRecord()
{
    FakeValidationLookup lookup;
    const Validator validator(lookup);
    QVERIFY(validator.validate(spec("location_country"), validCountry(), true).isEmpty());
    QVERIFY(validator.validate(spec("user_user"), validUser(), true).isEmpty());
}

void TestValidator::reportsEveryMissingRequiredFieldTogether()
{
    FakeValidationLookup lookup;
    const Validator validator(lookup);
    const auto errors = validator.validate(spec("location_country"), {}, true);
    QCOMPARE(columns(errors), (QStringList{QStringLiteral("alpha_2"), QStringLiteral("alpha_3"),
                                          QStringLiteral("numerical_code"), QStringLiteral("name")}));
    QVERIFY(errors.first().reason.contains(QStringLiteral("obligatorio")));
}

void TestValidator::maxLengthAcceptsExactLimit()
{
    FakeValidationLookup lookup;
    const Validator validator(lookup);
    QVariantMap values = validCountry();
    values[QStringLiteral("name")] = QString(100, QLatin1Char('a'));
    QVERIFY(validator.validate(spec("location_country"), values, true).isEmpty());
}

void TestValidator::maxLengthRejectsExcess()
{
    FakeValidationLookup lookup;
    const Validator validator(lookup);
    QVariantMap values = validCountry();
    values[QStringLiteral("alpha_2")] = QStringLiteral("PER");
    values[QStringLiteral("name")] = QString(101, QLatin1Char('a'));
    const auto errors = validator.validate(spec("location_country"), values, true);
    QCOMPARE(columns(errors), (QStringList{QStringLiteral("alpha_2"), QStringLiteral("name")}));
    QVERIFY(errors.first().reason.contains(QStringLiteral("longitud maxima de 2")));
}

void TestValidator::decimalAcceptsZeroRejectsNegative()
{
    FakeValidationLookup lookup;
    const Validator validator(lookup);
    const TableSpec &waste = spec("waste_waste");
    QVariantMap values{{QStringLiteral("id_waste"), QStringLiteral("W9")},
                       {QStringLiteral("id_gen_company_id"), QStringLiteral("G1")},
                       {QStringLiteral("amount_waste"), 0.0},
                       {QStringLiteral("id_toxicity_id"), 1}};
    QVERIFY(validator.validate(waste, values, true).isEmpty());

    values[QStringLiteral("amount_waste")] = -0.01;
    const auto errors = validator.validate(waste, values, true);
    QCOMPARE(columns(errors), QStringList{QStringLiteral("amount_waste")});
    QVERIFY(errors.first().reason.contains(QStringLiteral("negativo")));
}

void TestValidator::emailFormat_data()
{
    QTest::addColumn<QString>("email");
    QTest::addColumn<bool>("valid");
    QTest::newRow("valid") << QStringLiteral("ana@grat.local") << true;
    QTest::newRow("subdomain") << QStringLiteral("a.b@mail.grat.pe") << true;
    QTest::newRow("no-at") << QStringLiteral("ana.grat.local") << false;
    QTest::newRow("no-domain-dot") << QStringLiteral("ana@grat") << false;
    QTest::newRow("spaces") << QStringLiteral("ana @grat.local") << false;
    QTest::newRow("two-at") << QStringLiteral("ana@@grat.local") << false;
}

void TestValidator::emailFormat()
{
    QFETCH(QString, email);
    QFETCH(bool, valid);
    FakeValidationLookup lookup;
    const Validator validator(lookup);
    QVariantMap values = validUser();
    values[QStringLiteral("email")] = email;
    const auto errors = validator.validate(spec("user_user"), values, true);
    QCOMPARE(errors.isEmpty(), valid);
    if (!valid)
        QCOMPARE(columns(errors), QStringList{QStringLiteral("email")});
}

void TestValidator::reportsSimultaneousErrorsOfDifferentKinds()
{
    FakeValidationLookup lookup;
    const Validator validator(lookup);
    QVariantMap values = validUser();
    values[QStringLiteral("username")] = QString();                    // required
    values[QStringLiteral("email")] = QStringLiteral("correo-invalido"); // format
    values[QStringLiteral("first_name")] = QString(151, QLatin1Char('x')); // length
    const auto errors = validator.validate(spec("user_user"), values, true);
    QCOMPARE(errors.size(), 3);
    QVERIFY(columns(errors).contains(QStringLiteral("username")));
    QVERIFY(columns(errors).contains(QStringLiteral("email")));
    QVERIFY(columns(errors).contains(QStringLiteral("first_name")));
}

void TestValidator::partialEditValidatesOnlySubmittedFields()
{
    FakeValidationLookup lookup;
    const Validator validator(lookup);
    const QVariantMap values{{QStringLiteral("name"), QStringLiteral("Republica del Peru")}};
    QVERIFY(validator.validate(spec("location_country"), values, false).isEmpty());
}

void TestValidator::partialEditRejectsClearedRequiredField()
{
    FakeValidationLookup lookup;
    const Validator validator(lookup);
    const QVariantMap values{{QStringLiteral("name"), QString()}};
    QCOMPARE(columns(validator.validate(spec("location_country"), values, false)),
             QStringList{QStringLiteral("name")});
}

void TestValidator::passwordRequiredOnInsertOptionalOnEdit()
{
    FakeValidationLookup lookup;
    const Validator validator(lookup);
    QVariantMap values = validUser();
    values[QStringLiteral("password")] = QString();
    QCOMPARE(columns(validator.validate(spec("user_user"), values, true)),
             QStringList{QStringLiteral("password")});
    // Empty password on edit means "keep the current one".
    QVERIFY(validator.validate(spec("user_user"), values, false).isEmpty());
}

void TestValidator::skipsSystemManagedFields()
{
    FakeValidationLookup lookup;
    const Validator validator(lookup);
    // created_by_id/created_at/updated_at (auto), is_active (state) are absent.
    const QVariantMap values{{QStringLiteral("id_waste"), QStringLiteral("W9")},
                             {QStringLiteral("id_gen_company_id"), QStringLiteral("G1")},
                             {QStringLiteral("amount_waste"), 1.0},
                             {QStringLiteral("id_toxicity_id"), 1}};
    QVERIFY(validator.validate(spec("waste_waste"), values, true).isEmpty());
    // Identity PK is generated: not required for catalog tables.
    QVERIFY(validator.validate(spec("catalog_constituent"),
                               {{QStringLiteral("name"), QStringLiteral("Plomo")}}, true).isEmpty());
}

void TestValidator::naturalKeyDuplicateIsRejected()
{
    FakeValidationLookup lookup;
    lookup.existingKeys.insert(QStringLiteral("location_country|PE"));
    const Validator validator(lookup);
    const auto errors = validator.validate(spec("location_country"), validCountry(), true);
    QCOMPARE(columns(errors), QStringList{QStringLiteral("alpha_2")});
    QVERIFY(errors.first().reason.contains(QStringLiteral("Ya existe")));
}

void TestValidator::naturalKeyCheckedOnlyOnInsert()
{
    FakeValidationLookup lookup;
    lookup.existingKeys.insert(QStringLiteral("location_country|PE"));
    const Validator validator(lookup);
    QVERIFY(validator.validate(spec("location_country"), validCountry(), false).isEmpty());
    QCOMPARE(lookup.existsCalls, 0);
}

void TestValidator::identityTablesSkipKeyLookup()
{
    FakeValidationLookup lookup;
    const Validator validator(lookup);
    QVERIFY(validator.validate(spec("catalog_constituent"),
                               {{QStringLiteral("name"), QStringLiteral("Plomo")}}, true).isEmpty());
    QCOMPARE(lookup.existsCalls, 0);
}

void TestValidator::lookupFailureIsNotTreatedAsUnique()
{
    FakeValidationLookup lookup;
    lookup.failQueries = true;
    const Validator validator(lookup);
    const auto errors = validator.validate(spec("location_country"), validCountry(), true);
    QCOMPARE(columns(errors), QStringList{QStringLiteral("alpha_2")});
    QVERIFY(errors.first().reason.contains(QStringLiteral("No se pudo verificar")));
}

void TestValidator::duplicateConstituentPairIsRejected()
{
    FakeValidationLookup lookup;
    lookup.pairs.append({QStringLiteral("W1"), QStringLiteral("1"), QStringLiteral("7")});
    const Validator validator(lookup);
    const QVariantMap values{{QStringLiteral("id_waste_id"), QStringLiteral("W1")},
                             {QStringLiteral("id_constituent_id"), 1},
                             {QStringLiteral("amount_constituent"), 2.0}};
    const auto errors = validator.validate(constituentSpec(), values, true);
    QCOMPARE(columns(errors), QStringList{QStringLiteral("id_constituent_id")});
    QCOMPARE(lookup.pairCalls, 1);
    QVERIFY(lookup.lastExcluded.isNull());
}

void TestValidator::constituentAllowedInAnotherWaste()
{
    FakeValidationLookup lookup;
    lookup.pairs.append({QStringLiteral("W1"), QStringLiteral("1"), QStringLiteral("7")});
    const Validator validator(lookup);
    const QVariantMap values{{QStringLiteral("id_waste_id"), QStringLiteral("W2")},
                             {QStringLiteral("id_constituent_id"), 1},
                             {QStringLiteral("amount_constituent"), 2.0}};
    QVERIFY(validator.validate(constituentSpec(), values, true).isEmpty());
}

void TestValidator::editExcludesOwnRowFromDuplicates()
{
    FakeValidationLookup lookup;
    lookup.pairs.append({QStringLiteral("W1"), QStringLiteral("1"), QStringLiteral("7")});
    const Validator validator(lookup);
    const QVariantMap current{{QStringLiteral("id"), 7},
                              {QStringLiteral("id_waste_id"), QStringLiteral("W1")},
                              {QStringLiteral("id_constituent_id"), 1},
                              {QStringLiteral("amount_constituent"), 2.0}};
    const QVariantMap values{{QStringLiteral("id_waste_id"), QStringLiteral("W1")},
                             {QStringLiteral("id_constituent_id"), 1},
                             {QStringLiteral("amount_constituent"), 5.0}};
    QVERIFY(validator.validate(constituentSpec(), values, false, current).isEmpty());
    QCOMPARE(lookup.lastExcluded.toInt(), 7);
}

void TestValidator::partialChildEditUsesOriginalValues()
{
    FakeValidationLookup lookup;
    // Row 8 already links W1–2; editing row 7 to constituent 2 must collide.
    lookup.pairs.append({QStringLiteral("W1"), QStringLiteral("2"), QStringLiteral("8")});
    const Validator validator(lookup);
    const QVariantMap current{{QStringLiteral("id"), 7},
                              {QStringLiteral("id_waste_id"), QStringLiteral("W1")},
                              {QStringLiteral("id_constituent_id"), 1}};
    const QVariantMap values{{QStringLiteral("id_constituent_id"), 2}}; // waste not submitted
    QCOMPARE(columns(validator.validate(constituentSpec(), values, false, current)),
             QStringList{QStringLiteral("id_constituent_id")});
}

void TestValidator::injectedRuleRunsThroughInterface()
{
    FakeValidationLookup lookup;
    ValidationRuleSet rules = Validator::defaultRules();
    auto custom = std::make_unique<RejectNameRule>();
    const RejectNameRule *observer = custom.get();
    rules.fieldRules.push_back(std::move(custom));
    const Validator validator(lookup, std::move(rules));

    QVariantMap values = validCountry();
    QVERIFY(validator.validate(spec("location_country"), values, true).isEmpty());
    values[QStringLiteral("name")] = QStringLiteral("Prohibido");
    const auto errors = validator.validate(spec("location_country"), values, true);
    QCOMPARE(columns(errors), QStringList{QStringLiteral("name")});
    QCOMPARE(errors.first().reason, QStringLiteral("Nombre rechazado."));
    QCOMPARE(observer->calls, 2);
}

void TestValidator::emptyRuleSetAcceptsAnything()
{
    FakeValidationLookup lookup;
    const Validator validator(lookup, ValidationRuleSet{});
    QVERIFY(validator.validate(spec("location_country"), {}, true).isEmpty());
    QCOMPARE(lookup.existsCalls, 0);
}

void TestValidator::gatewayLookupDetectsExistingNaturalKey()
{
    TestDatabase db;
    QVERIFY2(db.isOpen(), qPrintable(db.lastError()));
    QVERIFY(db.exec(QStringLiteral("INSERT INTO location_country (alpha_2, alpha_3, numerical_code, name) "
                                   "VALUES ('PE', 'PER', '604', 'Peru')")));
    const TableGateway gateway(db.connection());
    const Validator validator(gateway);
    QCOMPARE(columns(validator.validate(spec("location_country"), validCountry(), true)),
             QStringList{QStringLiteral("alpha_2")});
    QVariantMap other = validCountry();
    other[QStringLiteral("alpha_2")] = QStringLiteral("CL");
    QVERIFY(validator.validate(spec("location_country"), other, true).isEmpty());
}

void TestValidator::gatewayLookupMatchesPairsInEveryState()
{
    TestDatabase db;
    QVERIFY(db.isOpen());
    QVERIFY(db.seedUsers());
    QString error;
    QVERIFY2(db.seedWasteData(&error), qPrintable(error));
    QVERIFY(db.exec(QStringLiteral("INSERT INTO waste_wasteconstituent "
                                   "(id_waste_id, id_constituent_id, amount_constituent, is_active) "
                                   "VALUES ('W1', 1, 2.0, '*')")));
    const TableGateway gateway(db.connection());
    const Validator validator(gateway);
    QVariantMap values{{QStringLiteral("id_waste_id"), QStringLiteral("W1")},
                       {QStringLiteral("id_constituent_id"), 1},
                       {QStringLiteral("amount_constituent"), 1.0}};
    // A logically deleted link still blocks the pair.
    QCOMPARE(columns(validator.validate(constituentSpec(), values, true)),
             QStringList{QStringLiteral("id_constituent_id")});
    values[QStringLiteral("id_waste_id")] = QStringLiteral("W2");
    QVERIFY(validator.validate(constituentSpec(), values, true).isEmpty());
}

QTEST_GUILESS_MAIN(TestValidator)
#include "test_validator.moc"
