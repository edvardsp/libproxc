
#include <chrono>
#include <iostream>
#include <vector>

#include <proxc.hpp>

using namespace proxc;

void writer( channel::Tx< int > tx )
{
    int n = 0;
    while ( ! tx.is_closed() ) {
        tx.send( n++ );
    }
}

void reader( std::vector< channel::Rx< int > > rxs )
{
    constexpr std::size_t num_runs = 100;
    constexpr std::size_t num_iter = 1000;
    double total_us = 0.;

    timer::Egg egg{ std::chrono::milliseconds( 1 ) };
    for ( std::size_t run = 0; run < num_runs; ++run ) {
        auto start = std::chrono::steady_clock::now();
        for ( std::size_t iter = 0; iter < num_iter; ++iter ) {
            Alt()
                .recv_for( rxs.begin(), rxs.end(),
                    []( int ){} )
                .select();
        }
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration< double, std::micro > diff = end - start;
        total_us += diff.count();
    }
    std::cout << "Avg. us per alt: " << total_us / num_runs / num_iter << "us" << std::endl;
}

int main()
{
    constexpr std::size_t num = 200;
    auto chs = channel::create_n< int >( num );
    auto txs = channel::get_tx( chs );

    parallel(
        proc_for( std::size_t{ 0 }, num,
            [&txs]( auto i ){ writer( std::move( txs[i] ) ); } ),
        proc( reader, channel::get_rx( chs ) )
    );

    return 0;
}
