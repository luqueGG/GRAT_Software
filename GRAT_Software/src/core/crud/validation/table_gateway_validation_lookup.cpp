#include "table_gateway_validation_lookup.h"

#include "core/crud/table_gateway.h"

TableGatewayValidationLookup::TableGatewayValidationLookup(const TableGateway &gateway)
    : m_gateway(gateway)
{
}

ValidationLookupResult TableGatewayValidationLookup::exists(const TableSpec &spec,
                                                            const QVariant &pk) const
{
    return m_gateway.exists(spec, pk);
}

ValidationLookupResult TableGatewayValidationLookup::constituentExists(
    const QVariant &wasteKey, const QVariant &constituentKey, const QVariant &excludePk) const
{
    return m_gateway.constituentExists(wasteKey, constituentKey, excludePk);
}
