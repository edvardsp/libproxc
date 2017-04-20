
examples = [
    ['concurrent_sieve',     ['concurrent_sieve.cpp']],
    ['concurrent_fibonacci', ['concurrent_fibonacci.cpp']]
]

foreach e : examples
    exe = executable(e[0], e[1],
        include_directories : [inc, src, exm],
        dependencies : [boost_dep, thread_dep],
        link_with : proxc)
endforeach
