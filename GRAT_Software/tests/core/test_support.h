// test_support.h — shared fixtures for the core test suite (P3).
//
// TestDatabase opens a fresh SQLite file with the real schema and RBAC seed
// inside a QTemporaryDir, so every test starts from a known state and the
// developer database (db/grat.db) is never touched.

#pragma once

#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTemporaryDir>
#include <QVariant>

#include <memory>
#include <optional>

#include "app/session.h"
#include "core/auth/auth_service.h"
#include "core/db/database.h"

namespace grat_test {

class TestDatabase
{
public:
    TestDatabase()
        : m_database(std::make_unique<Database>())
    {
        m_ok = m_dir.isValid() && m_database->open(m_dir.path());
    }

    TestDatabase(const TestDatabase &) = delete;
    TestDatabase &operator=(const TestDatabase &) = delete;

    bool isOpen() const { return m_ok && m_database->isConnected(); }
    QString lastError() const { return m_database->lastError(); }
    QString filePath() const { return QDir(m_dir.path()).filePath(QStringLiteral("grat.db")); }

    Database &database() { return *m_database; }
    QSqlDatabase connection() const { return m_database->connection(); }

    // Runs a statement; returns false and keeps the driver message on failure.
    bool exec(const QString &sql, QString *error = nullptr) const
    {
        QSqlQuery q(connection());
        if (q.exec(sql))
            return true;
        if (error)
            *error = q.lastError().text();
        return false;
    }

    // First column of the first row, or an invalid QVariant.
    QVariant scalar(const QString &sql) const
    {
        QSqlQuery q(connection());
        if (q.exec(sql) && q.next())
            return q.value(0);
        return {};
    }

    int count(const QString &sql) const { return scalar(sql).toInt(); }

    // The three demonstration accounts (admin / supervisor / operator).
    bool seedUsers() const
    {
        const AuthService auth(connection());
        return auth.ensureDefaultUsers();
    }

    std::optional<AuthSession> login(const QString &username, const QString &password) const
    {
        const AuthService auth(connection());
        return auth.login(username, password);
    }

    // Minimal fictitious reference data for waste/transfer relations:
    // country PE, subdivision PE-LIM, generator G1, toxicity 1, constituents 1-2,
    // wastes W1 and W2 (created by user 1). Requires seedUsers() first.
    bool seedWasteData(QString *error = nullptr) const
    {
        static const char *const statements[] = {
            "INSERT INTO location_country (alpha_2, alpha_3, numerical_code, name) "
            "VALUES ('PE', 'PER', '604', 'Peru')",
            "INSERT INTO location_subdivision (subdiv_code, country_alpha_2_id, type_subdivision, name) "
            "VALUES ('PE-LI', 'PE', 'Region', 'Lima')",
            "INSERT INTO waste_generatorcompany (id_gen_company, name_company, city_id, activity) "
            "VALUES ('G1', 'Generadora Uno', 'PE-LI', 'Mineria')",
            "INSERT INTO catalog_toxicitylevel (name, description) VALUES ('Alta', 'Toxicidad alta')",
            "INSERT INTO catalog_constituent (name) VALUES ('Plomo')",
            "INSERT INTO catalog_constituent (name) VALUES ('Mercurio')",
            "INSERT INTO waste_waste (id_waste, id_gen_company_id, amount_waste, id_toxicity_id, created_by_id) "
            "VALUES ('W1', 'G1', 10.5, 1, 1)",
            "INSERT INTO waste_waste (id_waste, id_gen_company_id, amount_waste, id_toxicity_id, created_by_id) "
            "VALUES ('W2', 'G1', 4, 1, 1)",
        };
        for (const char *sql : statements) {
            if (!exec(QString::fromLatin1(sql), error))
                return false;
        }
        return true;
    }

private:
    QTemporaryDir m_dir;
    std::unique_ptr<Database> m_database;
    bool m_ok = false;
};

} // namespace grat_test
