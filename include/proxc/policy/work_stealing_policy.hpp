
#pragma once

#include <proxc/config.hpp>

#include <proxc/policy/policy_base.hpp>

PROXC_NAMESPACE_BEGIN

namespace policy {

template<typename T>
class WorkStealingPolicy : public PolicyBase<T>
{

};

} // namespace policy

PROXC_NAMESPACE_END

