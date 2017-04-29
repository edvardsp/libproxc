
examples = [
    ['concurrent_sieve',     ['concurrent_sieve.cpp']],
    ['concurrent_fibonacci', ['concurrent_fibonacci.cpp']],
    ['commstime',            ['commstime.cpp']],
    ['stressed_alt',         ['stressed_alt.cpp']],
    ['dining_philosophers',  ['dining_philosophers.cpp']]
]

foreach e : examples
    exe = executable(e[0], e[1],
        include_directories : [inc, src, exm],
        dependencies : [boost_dep, thread_dep],
        link_with : proxc)
endforeach
