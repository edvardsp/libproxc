
#include <tuple>
#include <vector>

#include <proxc/config.hpp>

#include <proxc/scheduler.hpp>
#include <proxc/channel/sync.hpp>

#include "setup.hpp"

using namespace proxc;

void test_channel_sync_works()
{
    std::vector< int > answer = { 1, 7, 42, 1337, -100 };
    std::vector< int > ints;

    auto ch = channel::sync::create< int >();
    auto p1 = Scheduler::make_work(
        [&answer](auto tx){
            for (auto& i : answer) {
                tx.send( i );
            }
        },
        std::move( std::get<0>( ch ) )
    );
    auto p2 = Scheduler::make_work(
        [&ints](auto rx){
            while ( ! rx.is_closed() ) {
                int item;
                auto res = rx.recv( item );
                if ( res == channel::OpResult::Ok ) {
                    ints.push_back( item );
                }
            }
        },
        std::move( std::get<1>( ch ) )
    );

    Scheduler::self()->commit( p1.get() );
    Scheduler::self()->commit( p2.get() );

    Scheduler::self()->join( p2.get() );

    throw_assert_equ( ints.size(), answer.size(), "both vectors should have equal size" );
    for (std::size_t i = 0; i < ints.size(); ++i) {
        throw_assert_equ( ints[i], answer[i], "item shoud match" );
    }
}

int main()
{
    test_channel_sync_works();

    return 0;
}
