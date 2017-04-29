
#include <chrono>
#include <string>
#include <vector>

#include <proxc.hpp>

using namespace proxc;

constexpr int N = 6;

[[noreturn]]
void printer( std::vector< channel::Rx< std::string > > rxs )
{
    for ( ;; ) {
        Alt()
            .recv_for( rxs.begin(), rxs.end(),
                []( auto msg ){ std::cout << msg << std::endl; } )
            .select();
    }
}

[[noreturn]]
void a_fork( channel::Rx< int > left, channel::Rx< int > right )
{
    for ( ;; ) {
        Alt()
            .recv( left,
                [&left](auto){
                    int item{};
                    left.recv( item );
                } )
            .recv( right,
                [&right](auto){
                    int item{};
                    right.recv( item );
                } )
            .select();
    }
}

[[noreturn]]
void philosopher( channel::Tx< std::string > report, int i,
                  channel::Tx< int > left, channel::Tx< int > right,
                  channel::Tx< int > down, channel::Tx< int > up )
{
    auto dur = std::chrono::seconds( i );
    for ( ;; ) {
        report.send( std::to_string( i ) + " thinking" );
        this_proc::delay_for( dur );

        report.send( std::to_string( i ) + " hungry" );
        down.send( i );

        report.send( std::to_string( i ) + " sitting" );
        parallel(
            proc( [&left,i] { left.send( i ); } ),
            proc( [&right,i]{ right.send( i ); } )
        );

        report.send( std::to_string( i ) + " eating" );
        this_proc::delay_for( dur );

        report.send( std::to_string( i ) + " leaving" );
        parallel(
            proc( [&left,i] { left.send( i ); } ),
            proc( [&right,i]{ right.send( i ); } )
        );

        up.send( i );
    }
}

[[noreturn]]
void security( std::vector< channel::Rx< int > > down,
               std::vector< channel::Rx< int > > up )
{
    int sitting = 0;
    for ( ;; ) {
        Alt()
            .recv_for_if( sitting < N - 1,
                down.begin(), down.end(),
                [&sitting](auto){ ++sitting; } )
            .recv_for_if( sitting > 0,
                up.begin(), up.end(),
                [&sitting](auto){ --sitting; } )
            .select();
    }
}

int main()
{
    auto print = channel::create_n< std::string >( N );
    auto left  = channel::create_n< int >( N );
    auto right = channel::create_n< int >( N );
    auto up    = channel::create_n< int >( N );
    auto down  = channel::create_n< int >( N );

    std::vector< Process > philosophers;
    for ( std::size_t i = 0; i < N; ++i ) {
        philosophers.emplace_back( philosopher,
            channel::get_tx_ind( print, i ), i+1,
            channel::get_tx_ind( left, i ), channel::get_tx_ind( right, i ),
            channel::get_tx_ind( down, i ), channel::get_tx_ind( up, i ) );
    }

    std::vector< Process > forks;
    for ( std::size_t i = 0; i < N; ++i ) {
        forks.emplace_back( a_fork,
            channel::get_rx_ind( left, i ), channel::get_rx_ind( right, (i+1)%N ) );
    }

    parallel(
        proc_for( philosophers.begin(), philosophers.end() ),
        proc_for( forks.begin(), forks.end() ),
        proc( security, channel::get_rx( down ), channel::get_rx( up ) ),
        proc( printer, channel::get_rx( print ) )
    );

    return  0;
}
