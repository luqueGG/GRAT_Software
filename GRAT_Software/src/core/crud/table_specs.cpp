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
                ColumnSpec{.dbColumn = "alpha_2", .label = "Código (Alfa-2)", .type = ColumnType::Text, .maxLength = 2},
                ColumnSpec{.dbColumn = "alpha_3", .label = "Código (Alfa-3)", .type = ColumnType::Text, .maxLength = 3},
                ColumnSpec{.dbColumn = "numerical_code", .label = "Código numérico", .type = ColumnType::Text, .maxLength = 3},
                ColumnSpec{.dbColumn = "name", .label = "Nombre", .type = ColumnType::Text, .maxLength = 100},
                ColumnSpec{.dbColumn = "is_active", .label = "Estado", .type = ColumnType::State, .editable = false, .isState = true},
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
                ColumnSpec{.dbColumn = "subdiv_code", .label = "Código de subdivisión", .type = ColumnType::Text, .maxLength = 5},
                ColumnSpec{.dbColumn = "country_alpha_2_id", .label = "País", .type = ColumnType::Fk,
                           .refTable = "location_country", .refPk = "alpha_2", .refDisplay = "name"},
                ColumnSpec{.dbColumn = "type_subdivision", .label = "Tipo", .type = ColumnType::Text, .maxLength = 100},
                ColumnSpec{.dbColumn = "name", .label = "Nombre", .type = ColumnType::Text, .maxLength = 100},
                ColumnSpec{.dbColumn = "is_active", .label = "Estado", .type = ColumnType::State, .editable = false, .isState = true},
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
                ColumnSpec{.dbColumn = "name", .label = "Nombre", .type = ColumnType::Text, .maxLength = 50},
                ColumnSpec{.dbColumn = "is_active", .label = "Estado", .type = ColumnType::State, .editable = false, .isState = true},
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
                ColumnSpec{.dbColumn = "name", .label = "Nombre", .type = ColumnType::Text, .maxLength = 50},
                ColumnSpec{.dbColumn = "description", .label = "Descripción", .type = ColumnType::Text, .maxLength = 200},
                ColumnSpec{.dbColumn = "is_active", .label = "Estado", .type = ColumnType::State, .editable = false, .isState = true},
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
                ColumnSpec{.dbColumn = "name", .label = "Nombre", .type = ColumnType::Text, .maxLength = 50},
                ColumnSpec{.dbColumn = "description", .label = "Descripción", .type = ColumnType::Text, .maxLength = 200},
                ColumnSpec{.dbColumn = "is_active", .label = "Estado", .type = ColumnType::State, .editable = false, .isState = true},
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
                ColumnSpec{.dbColumn = "name", .label = "Nombre", .type = ColumnType::Text, .maxLength = 50},
                ColumnSpec{.dbColumn = "description", .label = "Descripción", .type = ColumnType::Text, .maxLength = 200},
                ColumnSpec{.dbColumn = "is_active", .label = "Estado", .type = ColumnType::State, .editable = false, .isState = true},
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
                ColumnSpec{.dbColumn = "password", .label = "Contraseña", .type = ColumnType::Password, .maxLength = 128},
                ColumnSpec{.dbColumn = "last_login", .label = "Último acceso", .type = ColumnType::DateTime,
                           .required = false, .editable = false, .autoManaged = true},
                ColumnSpec{.dbColumn = "is_superuser", .label = "Superusuario", .type = ColumnType::Boolean},
                ColumnSpec{.dbColumn = "username", .label = "Usuario", .type = ColumnType::Text, .maxLength = 150},
                ColumnSpec{.dbColumn = "first_name", .label = "Nombre", .type = ColumnType::Text, .maxLength = 150, .required = false},
                ColumnSpec{.dbColumn = "last_name", .label = "Apellido", .type = ColumnType::Text, .maxLength = 150, .required = false},
                ColumnSpec{.dbColumn = "email", .label = "Correo electrónico", .type = ColumnType::Text, .maxLength = 254},
                ColumnSpec{.dbColumn = "is_active", .label = "Estado", .type = ColumnType::State, .editable = false, .isState = true},
                ColumnSpec{.dbColumn = "is_staff", .label = "Personal", .type = ColumnType::Boolean},
                ColumnSpec{.dbColumn = "created_at", .label = "Creado el", .type = ColumnType::DateTime, .editable = false, .autoManaged = true},
                ColumnSpec{.dbColumn = "updated_at", .label = "Actualizado el", .type = ColumnType::DateTime, .editable = false, .autoManaged = true},
            },
            .children = {
                ChildTableSpec{
                    .table = "user_user_groups",
                    .fkToParent = "user_id",
                    .appLabel = "user",
                    .model = "user",
                    .hasState = false,
                    .columns = {
                        ColumnSpec{.dbColumn = "user_id", .label = "Usuario", .type = ColumnType::Integer, .editable = false, .autoManaged = true},
                        ColumnSpec{.dbColumn = "group_id", .label = "Rol", .type = ColumnType::Fk,
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
                        ColumnSpec{.dbColumn = "user_id", .label = "Usuario", .type = ColumnType::Integer, .editable = false, .autoManaged = true},
                        ColumnSpec{.dbColumn = "permission_id", .label = "Permiso", .type = ColumnType::Fk,
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
                ColumnSpec{.dbColumn = "id_gen_company", .label = "ID del generador", .type = ColumnType::Text, .maxLength = 20},
                ColumnSpec{.dbColumn = "name_company", .label = "Nombre de la empresa", .type = ColumnType::Text, .maxLength = 50},
                ColumnSpec{.dbColumn = "city_id", .label = "Ciudad", .type = ColumnType::Fk,
                           .refTable = "location_subdivision", .refPk = "subdiv_code", .refDisplay = "name"},
                ColumnSpec{.dbColumn = "activity", .label = "Actividad", .type = ColumnType::Text, .maxLength = 200},
                ColumnSpec{.dbColumn = "is_active", .label = "Estado", .type = ColumnType::State, .editable = false, .isState = true},
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
                ColumnSpec{.dbColumn = "id_waste", .label = "ID del residuo", .type = ColumnType::Text, .maxLength = 8},
                ColumnSpec{.dbColumn = "id_gen_company_id", .label = "Empresa generadora", .type = ColumnType::Fk,
                           .refTable = "waste_generatorcompany", .refPk = "id_gen_company", .refDisplay = "name_company"},
                ColumnSpec{.dbColumn = "amount_waste", .label = "Cantidad", .type = ColumnType::Decimal},
                ColumnSpec{.dbColumn = "id_toxicity_id", .label = "Nivel de toxicidad", .type = ColumnType::Fk,
                           .refTable = "catalog_toxicitylevel", .refPk = "id", .refDisplay = "name"},
                ColumnSpec{.dbColumn = "created_by_id", .label = "Creado por", .type = ColumnType::Fk,
                           .editable = false, .autoManaged = true,
                           .refTable = "user_user", .refPk = "id", .refDisplay = "username"},
                ColumnSpec{.dbColumn = "created_at", .label = "Creado el", .type = ColumnType::DateTime, .editable = false, .autoManaged = true},
                ColumnSpec{.dbColumn = "updated_at", .label = "Actualizado el", .type = ColumnType::DateTime, .editable = false, .autoManaged = true},
                ColumnSpec{.dbColumn = "is_active", .label = "Estado", .type = ColumnType::State, .editable = false, .isState = true},
            },
            .children = {
                ChildTableSpec{
                    .table = "waste_wasteconstituent",
                    .fkToParent = "id_waste_id",
                    .appLabel = "waste",
                    .model = "wasteconstituent",
                    .hasState = true,
                    .columns = {
                        ColumnSpec{.dbColumn = "id_waste_id", .label = "Residuo", .type = ColumnType::Text, .editable = false, .autoManaged = true},
                        ColumnSpec{.dbColumn = "id_constituent_id", .label = "Componente", .type = ColumnType::Fk,
                                   .refTable = "catalog_constituent", .refPk = "id", .refDisplay = "name"},
                        ColumnSpec{.dbColumn = "amount_constituent", .label = "Cantidad", .type = ColumnType::Decimal},
                        ColumnSpec{.dbColumn = "is_active", .label = "Estado", .type = ColumnType::State, .editable = false, .isState = true},
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
                ColumnSpec{.dbColumn = "name", .label = "Nombre", .type = ColumnType::Text, .maxLength = 50},
                ColumnSpec{.dbColumn = "city_id", .label = "Ciudad", .type = ColumnType::Fk,
                           .refTable = "location_subdivision", .refPk = "subdiv_code", .refDisplay = "name"},
                ColumnSpec{.dbColumn = "is_active", .label = "Estado", .type = ColumnType::State, .editable = false, .isState = true},
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
                ColumnSpec{.dbColumn = "id_travel_company", .label = "ID de la empresa", .type = ColumnType::Text, .maxLength = 20},
                ColumnSpec{.dbColumn = "name", .label = "Nombre", .type = ColumnType::Text, .maxLength = 50},
                ColumnSpec{.dbColumn = "city_id", .label = "Ciudad", .type = ColumnType::Fk,
                           .refTable = "location_subdivision", .refPk = "subdiv_code", .refDisplay = "name"},
                ColumnSpec{.dbColumn = "is_active", .label = "Estado", .type = ColumnType::State, .editable = false, .isState = true},
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
                ColumnSpec{.dbColumn = "id_transfer", .label = "ID del traslado", .type = ColumnType::Text, .maxLength = 20},
                ColumnSpec{.dbColumn = "id_travel_company_id", .label = "Empresa transportista", .type = ColumnType::Fk,
                           .refTable = "transfer_travelcompany", .refPk = "id_travel_company", .refDisplay = "name"},
                ColumnSpec{.dbColumn = "id_destination_id", .label = "Destino", .type = ColumnType::Fk,
                           .refTable = "transfer_destination", .refPk = "id", .refDisplay = "name"},
                ColumnSpec{.dbColumn = "id_waste_id", .label = "Residuo", .type = ColumnType::Fk,
                           .refTable = "waste_waste", .refPk = "id_waste"}, // no name → show PK only
                ColumnSpec{.dbColumn = "id_container_type_id", .label = "Tipo de contenedor", .type = ColumnType::Fk,
                           .refTable = "catalog_containertype", .refPk = "id", .refDisplay = "name"},
                ColumnSpec{.dbColumn = "amount_waste", .label = "Cantidad", .type = ColumnType::Decimal},
                ColumnSpec{.dbColumn = "delivery_date", .label = "Fecha de entrega", .type = ColumnType::Date},
                ColumnSpec{.dbColumn = "created_by_id", .label = "Creado por", .type = ColumnType::Fk,
                           .editable = false, .autoManaged = true,
                           .refTable = "user_user", .refPk = "id", .refDisplay = "username"},
                ColumnSpec{.dbColumn = "created_at", .label = "Creado el", .type = ColumnType::DateTime, .editable = false, .autoManaged = true},
                ColumnSpec{.dbColumn = "updated_at", .label = "Actualizado el", .type = ColumnType::DateTime, .editable = false, .autoManaged = true},
                ColumnSpec{.dbColumn = "is_active", .label = "Estado", .type = ColumnType::State, .editable = false, .isState = true},
            },
            .children = {
                ChildTableSpec{
                    .table = "transfer_transfertravelcompany",
                    .fkToParent = "id_transfer_id",
                    .appLabel = "transfer",
                    .model = "transfertravelcompany",
                    .hasState = false,
                    .columns = {
                        ColumnSpec{.dbColumn = "id_transfer_id", .label = "Traslado", .type = ColumnType::Text, .editable = false, .autoManaged = true},
                        ColumnSpec{.dbColumn = "id_travel_company_id", .label = "Empresa transportista", .type = ColumnType::Fk,
                                   .refTable = "transfer_travelcompany", .refPk = "id_travel_company", .refDisplay = "name"},
                        ColumnSpec{.dbColumn = "id_type_transport_id", .label = "Tipo de transporte", .type = ColumnType::Fk,
                                   .refTable = "catalog_transporttype", .refPk = "id", .refDisplay = "name"},
                        ColumnSpec{.dbColumn = "cost", .label = "Costo", .type = ColumnType::Decimal},
                        ColumnSpec{.dbColumn = "kms", .label = "Km", .type = ColumnType::Decimal},
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
                ColumnSpec{.dbColumn = "name", .label = "Nombre del rol", .type = ColumnType::Text, .maxLength = 150},
            },
            .children = {
                ChildTableSpec{
                    .table = "auth_group_permissions",
                    .fkToParent = "group_id",
                    .appLabel = "admin",
                    .model = "group",
                    .hasState = false,
                    .columns = {
                        ColumnSpec{.dbColumn = "group_id", .label = "Rol", .type = ColumnType::Integer, .editable = false, .autoManaged = true},
                        ColumnSpec{.dbColumn = "permission_id", .label = "Permiso", .type = ColumnType::Fk,
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
