
#include <proxc/config.hpp>

#include <proxc/alt/alt.hpp>

PROXC_NAMESPACE_BEGIN
namespace alt {

ChoiceBase::ChoiceBase( Alt * alt ) noexcept
    : alt_{ alt }
{}

bool ChoiceBase::same_alt( Alt * alt ) noexcept
{
    return alt == alt_;
}

bool ChoiceBase::try_select() noexcept
{
    return alt_->try_select( this );
}

void ChoiceBase::maybe_wakeup() noexcept
{
    return alt_->maybe_wakeup();
}

} // namespace alt
PROXC_NAMESPACE_END

