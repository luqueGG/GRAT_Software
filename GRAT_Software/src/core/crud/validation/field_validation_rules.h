#pragma once

#include <memory>
#include <vector>

#include "i_validation_rule.h"

std::vector<std::unique_ptr<IValidationRule>> makeFieldValidationRules();
