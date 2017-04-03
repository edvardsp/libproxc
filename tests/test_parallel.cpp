
#include <proxc/config.hpp>

#include <proxc/parallel.hpp>

#include "setup.hpp"

using namespace proxc;

int main()
{
    parallel(
        make_proc( 1 ),
        make_proc( 2 ),
        make_proc( 3 ),
        make_proc_list(
            make_proc( 4 ), 
            make_proc( 5 ), 
            make_proc( 6 ) 
        ),
        make_proc_list(
            make_proc( 7 ), 
            make_proc_list(
                make_proc( 8 ),
                make_proc( 9 )
            ), 
            make_proc( 10 ) 
        ),
        make_proc( 11 ),
        make_proc_list_for< 3 >(
            make_proc( 12 ),
            make_proc( 13 )
        )
    );
}
