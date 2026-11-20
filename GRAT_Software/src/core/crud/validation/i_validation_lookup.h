#pragma once

class QVariant;
struct TableSpec;

enum class ValidationLookupResult
{
    Found,
    NotFound,
    QueryError
};

// Read-only queries needed by Validator. A failed query is not a negative match.
class IValidationLookup
{
public:
    virtual ~IValidationLookup() = default;

    virtual ValidationLookupResult exists(const TableSpec &spec, const QVariant &pk) const = 0;
    virtual ValidationLookupResult constituentExists(const QVariant &wasteKey,
                                                      const QVariant &constituentKey,
                                                      const QVariant &excludePk) const = 0;
};
