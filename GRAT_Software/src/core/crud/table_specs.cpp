// table_specs.cpp — concrete TableSpec definitions for every managed table.
//
// Metadata for: 12 standalone CRUD tables (design §5.1) plus the admin
// 'auth_group' role screen (design §5.3), and the link/association tables as
// master-detail children (design §5.2). Field metadata (required, maxLength, FK
// references) mirrors schema.sql; VARCHAR lengths are enforced app-side (Req 7.3)
// and FK columns drive ComboBox rendering (Req 10).

#include "table_spec.h"

const std::vector<TableSpec> &allTableSpecs()
{
    static const std::vector<TableSpec> specs = {
        // =====================================================================
        // LOCATION MODULE
        // =====================================================================
        TableSpec{
            .table = "location_country",
            .module = "location",
            .pkColumn = "alpha_2",
            .appLabel = "location",
            .model = "country",
            .identityPk = false,
            .columns = {
                ColumnSpec{.dbColumn = "alpha_2", .label = "Code (Alpha-2)", .type = ColumnType::Text, .maxLength = 2},
                ColumnSpec{.dbColumn = "alpha_3", .label = "Code (Alpha-3)", .type = ColumnType::Text, .maxLength = 3},
                ColumnSpec{.dbColumn = "numerical_code", .label = "Numerical Code", .type = ColumnType::Text, .maxLength = 3},
                ColumnSpec{.dbColumn = "name", .label = "Name", .type = ColumnType::Text, .maxLength = 100},
                ColumnSpec{.dbColumn = "is_active", .label = "State", .type = ColumnType::State, .editable = false, .isState = true},
            },
        },
        TableSpec{
            .table = "location_subdivision",
            .module = "location",
            .pkColumn = "subdiv_code",
            .appLabel = "location",
            .model = "subdivision",
            .identityPk = false,
            .columns = {
                ColumnSpec{.dbColumn = "subdiv_code", .label = "Subdivision Code", .type = ColumnType::Text, .maxLength = 5},
                ColumnSpec{.dbColumn = "country_alpha_2_id", .label = "Country", .type = ColumnType::Fk,
                           .refTable = "location_country", .refPk = "alpha_2", .refDisplay = "name"},
                ColumnSpec{.dbColumn = "type_subdivision", .label = "Type", .type = ColumnType::Text, .maxLength = 100},
                ColumnSpec{.dbColumn = "name", .label = "Name", .type = ColumnType::Text, .maxLength = 100},
                ColumnSpec{.dbColumn = "is_active", .label = "State", .type = ColumnType::State, .editable = false, .isState = true},
            },
        },

        // =====================================================================
        // CATALOG MODULE
        // =====================================================================
        TableSpec{
            .table = "catalog_constituent",
            .module = "catalog",
            .pkColumn = "id",
            .appLabel = "catalog",
            .model = "constituent",
            .identityPk = true,
            .columns = {
                ColumnSpec{.dbColumn = "id", .label = "ID", .type = ColumnType::Integer, .editable = false},
                ColumnSpec{.dbColumn = "name", .label = "Name", .type = ColumnType::Text, .maxLength = 50},
                ColumnSpec{.dbColumn = "is_active", .label = "State", .type = ColumnType::State, .editable = false, .isState = true},
            },
        },
        TableSpec{
            .table = "catalog_containertype",
            .module = "catalog",
            .pkColumn = "id",
            .appLabel = "catalog",
            .model = "containertype",
            .identityPk = true,
            .columns = {
                ColumnSpec{.dbColumn = "id", .label = "ID", .type = ColumnType::Integer, .editable = false},
                ColumnSpec{.dbColumn = "name", .label = "Name", .type = ColumnType::Text, .maxLength = 50},
                ColumnSpec{.dbColumn = "description", .label = "Description", .type = ColumnType::Text, .maxLength = 200},
                ColumnSpec{.dbColumn = "is_active", .label = "State", .type = ColumnType::State, .editable = false, .isState = true},
            },
        },
        TableSpec{
            .table = "catalog_toxicitylevel",
            .module = "catalog",
            .pkColumn = "id",
            .appLabel = "catalog",
            .model = "toxicitylevel",
            .identityPk = true,
            .columns = {
                ColumnSpec{.dbColumn = "id", .label = "ID", .type = ColumnType::Integer, .editable = false},
                ColumnSpec{.dbColumn = "name", .label = "Name", .type = ColumnType::Text, .maxLength = 50},
                ColumnSpec{.dbColumn = "description", .label = "Description", .type = ColumnType::Text, .maxLength = 200},
                ColumnSpec{.dbColumn = "is_active", .label = "State", .type = ColumnType::State, .editable = false, .isState = true},
            },
        },
        TableSpec{
            .table = "catalog_transporttype",
            .module = "catalog",
            .pkColumn = "id",
            .appLabel = "catalog",
            .model = "transporttype",
            .identityPk = true,
            .columns = {
                ColumnSpec{.dbColumn = "id", .label = "ID", .type = ColumnType::Integer, .editable = false},
                ColumnSpec{.dbColumn = "name", .label = "Name", .type = ColumnType::Text, .maxLength = 50},
                ColumnSpec{.dbColumn = "description", .label = "Description", .type = ColumnType::Text, .maxLength = 200},
                ColumnSpec{.dbColumn = "is_active", .label = "State", .type = ColumnType::State, .editable = false, .isState = true},
            },
        },

        // =====================================================================
        // USER MODULE
        // =====================================================================
        TableSpec{
            .table = "user_user",
            .module = "user",
            .pkColumn = "id",
            .appLabel = "user",
            .model = "user",
            .identityPk = true,
            .columns = {
                ColumnSpec{.dbColumn = "id", .label = "ID", .type = ColumnType::Integer, .editable = false},
                ColumnSpec{.dbColumn = "password", .label = "Password", .type = ColumnType::Password, .maxLength = 128},
                ColumnSpec{.dbColumn = "last_login", .label = "Last Login", .type = ColumnType::DateTime,
                           .required = false, .editable = false, .autoManaged = true},
                ColumnSpec{.dbColumn = "is_superuser", .label = "Superuser", .type = ColumnType::Boolean},
                ColumnSpec{.dbColumn = "username", .label = "Username", .type = ColumnType::Text, .maxLength = 150},
                ColumnSpec{.dbColumn = "first_name", .label = "First Name", .type = ColumnType::Text, .maxLength = 150, .required = false},
                ColumnSpec{.dbColumn = "last_name", .label = "Last Name", .type = ColumnType::Text, .maxLength = 150, .required = false},
                ColumnSpec{.dbColumn = "email", .label = "Email", .type = ColumnType::Text, .maxLength = 254},
                ColumnSpec{.dbColumn = "is_active", .label = "State", .type = ColumnType::State, .editable = false, .isState = true},
                ColumnSpec{.dbColumn = "is_staff", .label = "Staff", .type = ColumnType::Boolean},
                ColumnSpec{.dbColumn = "created_at", .label = "Created At", .type = ColumnType::DateTime, .editable = false, .autoManaged = true},
                ColumnSpec{.dbColumn = "updated_at", .label = "Updated At", .type = ColumnType::DateTime, .editable = false, .autoManaged = true},
            },
            .children = {
                ChildTableSpec{
                    .table = "user_user_groups",
                    .fkToParent = "user_id",
                    .appLabel = "user",
                    .model = "user",
                    .hasState = false,
                    .columns = {
                        ColumnSpec{.dbColumn = "user_id", .label = "User", .type = ColumnType::Integer, .editable = false, .autoManaged = true},
                        ColumnSpec{.dbColumn = "group_id", .label = "Role", .type = ColumnType::Fk,
                                   .refTable = "auth_group", .refPk = "id", .refDisplay = "name"},
                    },
                },
                ChildTableSpec{
                    .table = "user_user_user_permissions",
                    .fkToParent = "user_id",
                    .appLabel = "user",
                    .model = "user",
                    .hasState = false,
                    .columns = {
                        ColumnSpec{.dbColumn = "user_id", .label = "User", .type = ColumnType::Integer, .editable = false, .autoManaged = true},
                        ColumnSpec{.dbColumn = "permission_id", .label = "Permission", .type = ColumnType::Fk,
                                   .refTable = "auth_permission", .refPk = "id", .refDisplay = "name"},
                    },
                },
            },
        },

        // =====================================================================
        // WASTE MODULE
        // =====================================================================
        TableSpec{
            .table = "waste_generatorcompany",
            .module = "waste",
            .pkColumn = "id_gen_company",
            .appLabel = "waste",
            .model = "generatorcompany",
            .identityPk = false,
            .columns = {
                ColumnSpec{.dbColumn = "id_gen_company", .label = "Generator ID", .type = ColumnType::Text, .maxLength = 20},
                ColumnSpec{.dbColumn = "name_company", .label = "Company Name", .type = ColumnType::Text, .maxLength = 50},
                ColumnSpec{.dbColumn = "city_id", .label = "City", .type = ColumnType::Fk,
                           .refTable = "location_subdivision", .refPk = "subdiv_code", .refDisplay = "name"},
                ColumnSpec{.dbColumn = "activity", .label = "Activity", .type = ColumnType::Text, .maxLength = 200},
                ColumnSpec{.dbColumn = "is_active", .label = "State", .type = ColumnType::State, .editable = false, .isState = true},
            },
        },
        TableSpec{
            .table = "waste_waste",
            .module = "waste",
            .pkColumn = "id_waste",
            .appLabel = "waste",
            .model = "waste",
            .identityPk = false,
            .columns = {
                ColumnSpec{.dbColumn = "id_waste", .label = "Waste ID", .type = ColumnType::Text, .maxLength = 8},
                ColumnSpec{.dbColumn = "id_gen_company_id", .label = "Generator Company", .type = ColumnType::Fk,
                           .refTable = "waste_generatorcompany", .refPk = "id_gen_company", .refDisplay = "name_company"},
                ColumnSpec{.dbColumn = "amount_waste", .label = "Amount", .type = ColumnType::Decimal},
                ColumnSpec{.dbColumn = "id_toxicity_id", .label = "Toxicity Level", .type = ColumnType::Fk,
                           .refTable = "catalog_toxicitylevel", .refPk = "id", .refDisplay = "name"},
                ColumnSpec{.dbColumn = "created_by_id", .label = "Created By", .type = ColumnType::Fk,
                           .editable = false, .autoManaged = true,
                           .refTable = "user_user", .refPk = "id", .refDisplay = "username"},
                ColumnSpec{.dbColumn = "created_at", .label = "Created At", .type = ColumnType::DateTime, .editable = false, .autoManaged = true},
                ColumnSpec{.dbColumn = "updated_at", .label = "Updated At", .type = ColumnType::DateTime, .editable = false, .autoManaged = true},
                ColumnSpec{.dbColumn = "is_active", .label = "State", .type = ColumnType::State, .editable = false, .isState = true},
            },
            .children = {
                ChildTableSpec{
                    .table = "waste_wasteconstituent",
                    .fkToParent = "id_waste_id",
                    .appLabel = "waste",
                    .model = "wasteconstituent",
                    .hasState = true,
                    .columns = {
                        ColumnSpec{.dbColumn = "id_waste_id", .label = "Waste", .type = ColumnType::Text, .editable = false, .autoManaged = true},
                        ColumnSpec{.dbColumn = "id_constituent_id", .label = "Constituent", .type = ColumnType::Fk,
                                   .refTable = "catalog_constituent", .refPk = "id", .refDisplay = "name"},
                        ColumnSpec{.dbColumn = "amount_constituent", .label = "Amount", .type = ColumnType::Decimal},
                        ColumnSpec{.dbColumn = "is_active", .label = "State", .type = ColumnType::State, .editable = false, .isState = true},
                    },
                },
            },
        },

        // =====================================================================
        // TRANSFER MODULE
        // =====================================================================
        TableSpec{
            .table = "transfer_destination",
            .module = "transfer",
            .pkColumn = "id",
            .appLabel = "transfer",
            .model = "destination",
            .identityPk = true,
            .columns = {
                ColumnSpec{.dbColumn = "id", .label = "ID", .type = ColumnType::Integer, .editable = false},
                ColumnSpec{.dbColumn = "name", .label = "Name", .type = ColumnType::Text, .maxLength = 50},
                ColumnSpec{.dbColumn = "city_id", .label = "City", .type = ColumnType::Fk,
                           .refTable = "location_subdivision", .refPk = "subdiv_code", .refDisplay = "name"},
                ColumnSpec{.dbColumn = "is_active", .label = "State", .type = ColumnType::State, .editable = false, .isState = true},
            },
        },
        TableSpec{
            .table = "transfer_travelcompany",
            .module = "transfer",
            .pkColumn = "id_travel_company",
            .appLabel = "transfer",
            .model = "travelcompany",
            .identityPk = false,
            .columns = {
                ColumnSpec{.dbColumn = "id_travel_company", .label = "Company ID", .type = ColumnType::Text, .maxLength = 20},
                ColumnSpec{.dbColumn = "name", .label = "Name", .type = ColumnType::Text, .maxLength = 50},
                ColumnSpec{.dbColumn = "city_id", .label = "City", .type = ColumnType::Fk,
                           .refTable = "location_subdivision", .refPk = "subdiv_code", .refDisplay = "name"},
                ColumnSpec{.dbColumn = "is_active", .label = "State", .type = ColumnType::State, .editable = false, .isState = true},
            },
        },
        TableSpec{
            .table = "transfer_transfer",
            .module = "transfer",
            .pkColumn = "id_transfer",
            .appLabel = "transfer",
            .model = "transfer",
            .identityPk = false,
            .columns = {
                ColumnSpec{.dbColumn = "id_transfer", .label = "Transfer ID", .type = ColumnType::Text, .maxLength = 20},
                ColumnSpec{.dbColumn = "id_travel_company_id", .label = "Travel Company", .type = ColumnType::Fk,
                           .refTable = "transfer_travelcompany", .refPk = "id_travel_company", .refDisplay = "name"},
                ColumnSpec{.dbColumn = "id_destination_id", .label = "Destination", .type = ColumnType::Fk,
                           .refTable = "transfer_destination", .refPk = "id", .refDisplay = "name"},
                ColumnSpec{.dbColumn = "id_waste_id", .label = "Waste", .type = ColumnType::Fk,
                           .refTable = "waste_waste", .refPk = "id_waste"}, // no name → show PK only
                ColumnSpec{.dbColumn = "id_container_type_id", .label = "Container Type", .type = ColumnType::Fk,
                           .refTable = "catalog_containertype", .refPk = "id", .refDisplay = "name"},
                ColumnSpec{.dbColumn = "amount_waste", .label = "Amount", .type = ColumnType::Decimal},
                ColumnSpec{.dbColumn = "delivery_date", .label = "Delivery Date", .type = ColumnType::Date},
                ColumnSpec{.dbColumn = "created_by_id", .label = "Created By", .type = ColumnType::Fk,
                           .editable = false, .autoManaged = true,
                           .refTable = "user_user", .refPk = "id", .refDisplay = "username"},
                ColumnSpec{.dbColumn = "created_at", .label = "Created At", .type = ColumnType::DateTime, .editable = false, .autoManaged = true},
                ColumnSpec{.dbColumn = "updated_at", .label = "Updated At", .type = ColumnType::DateTime, .editable = false, .autoManaged = true},
                ColumnSpec{.dbColumn = "is_active", .label = "State", .type = ColumnType::State, .editable = false, .isState = true},
            },
            .children = {
                ChildTableSpec{
                    .table = "transfer_transfertravelcompany",
                    .fkToParent = "id_transfer_id",
                    .appLabel = "transfer",
                    .model = "transfertravelcompany",
                    .hasState = false,
                    .columns = {
                        ColumnSpec{.dbColumn = "id_transfer_id", .label = "Transfer", .type = ColumnType::Text, .editable = false, .autoManaged = true},
                        ColumnSpec{.dbColumn = "id_travel_company_id", .label = "Travel Company", .type = ColumnType::Fk,
                                   .refTable = "transfer_travelcompany", .refPk = "id_travel_company", .refDisplay = "name"},
                        ColumnSpec{.dbColumn = "id_type_transport_id", .label = "Transport Type", .type = ColumnType::Fk,
                                   .refTable = "catalog_transporttype", .refPk = "id", .refDisplay = "name"},
                        ColumnSpec{.dbColumn = "cost", .label = "Cost", .type = ColumnType::Decimal},
                        ColumnSpec{.dbColumn = "kms", .label = "Kms", .type = ColumnType::Decimal},
                    },
                },
            },
        },

        // =====================================================================
        // ADMIN MODULE — Roles & Permissions (design §5.3)
        // auth_group is the only table outside the 43-permission CRUD matrix; it
        // is managed by Administrators via this master-detail screen. auth_permission
        // is a fixed system catalog (read-only) referenced by the children below.
        // =====================================================================
        TableSpec{
            .table = "auth_group",
            .module = "admin",
            .pkColumn = "id",
            .appLabel = "admin",
            .model = "group",
            .identityPk = true,
            .columns = {
                ColumnSpec{.dbColumn = "id", .label = "ID", .type = ColumnType::Integer, .editable = false},
                ColumnSpec{.dbColumn = "name", .label = "Role Name", .type = ColumnType::Text, .maxLength = 150},
            },
            .children = {
                ChildTableSpec{
                    .table = "auth_group_permissions",
                    .fkToParent = "group_id",
                    .appLabel = "admin",
                    .model = "group",
                    .hasState = false,
                    .columns = {
                        ColumnSpec{.dbColumn = "group_id", .label = "Role", .type = ColumnType::Integer, .editable = false, .autoManaged = true},
                        ColumnSpec{.dbColumn = "permission_id", .label = "Permission", .type = ColumnType::Fk,
                                   .refTable = "auth_permission", .refPk = "id", .refDisplay = "name"},
                    },
                },
            },
        },
    };
    return specs;
}

const TableSpec *findTableSpec(const QString &tableName)
{
    const auto &specs = allTableSpecs();
    for (const auto &s : specs)
        if (tableName == QLatin1String(s.table))
            return &s;
    return nullptr;
}
