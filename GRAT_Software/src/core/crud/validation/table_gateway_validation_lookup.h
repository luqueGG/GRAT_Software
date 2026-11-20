#pragma once

#include "i_validation_lookup.h"

class TableGateway;

class TableGatewayValidationLookup final : public IValidationLookup
{
public:
    // Borrows the gateway, which must outlive this adapter.
    explicit TableGatewayValidationLookup(const TableGateway &gateway);
    TableGatewayValidationLookup(const TableGateway &&) = delete;

    ValidationLookupResult exists(const TableSpec &spec, const QVariant &pk) const override;
    ValidationLookupResult constituentExists(const QVariant &wasteKey,
                                              const QVariant &constituentKey,
                                              const QVariant &excludePk) const override;

private:
    const TableGateway &m_gateway;
};
