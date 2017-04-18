
#include <proxc/config.hpp>

#include <proxc/alt/alt.hpp>
#include <proxc/alt/choice_base.hpp>

PROXC_NAMESPACE_BEGIN
namespace alt {

ChoiceBase::ChoiceBase( Alt * alt ) noexcept
    : alt_{ alt }
{}

bool ChoiceBase::try_select() noexcept
{
    return alt_->try_select( this );
}

void ChoiceBase::maybe_wakeup() noexcept
{
    alt_->maybe_wakeup();
}

bool same_alt( Alt * alt ) const noexcept;
{
    return alt == alt_;
}

} // namespace alt
PROXC_NAMESPACE_END

