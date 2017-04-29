/* 
 * MIT License
 * 
 * Copyright (c) [2017] [Edvard S. Pettersen] <edvard.pettersen@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <proxc.h>

void prefix(void) { 
    Chan *a = ARGN(0), *b = ARGN(1);
    int value = 0;
    for(;;) {
        CHWRITE(a, &value, int);
        CHREAD(b, &value, int);
    }
}
void delta(void) { 
    Chan *a = ARGN(0), *c = ARGN(1), *d = ARGN(2);
    int value;
    for (;;) {
        CHREAD(a, &value, int);
        CHWRITE(d, &value, int);
        CHWRITE(c, &value, int);
    }
}

void successor(void) {
    Chan *b = ARGN(0), *c = ARGN(1);
    int value;
    for (;;) {
        CHREAD(c, &value, int);
        value++;
        CHWRITE(b, &value, int);
    }
}

void consumer(void) {
    Chan *d = ARGN(0);
    int repeat = 20000;
    int runs = 100;
    clock_t start, stop;
    long time_diff = 0;

    printf("Repeat = %d, runs = %d\n", repeat, runs);
    for (int j = 0; j < runs; j++) {
        start = clock();

        int x;
        for (int i = 0; i < repeat; i++ ) {
            CHREAD(d, &x, int);
        }
        stop = clock();
        time_diff += (long)(stop - start);
        //printf("\t%d: %fms\n", j, time_spent);
    }
    long sum_ns = time_diff * 1000000000L / CLOCKS_PER_SEC;

    printf("Average = %.2fms / %d loops\n", (double)(sum_ns) / (1000.0 * 1000.0) / (runs), repeat);
    printf("Average = %.2fns / loop\n", (double)(sum_ns) / (runs * repeat));
    printf("Average = %.2fns / chan op\n", (double)(sum_ns) / (runs * repeat * 4));
}

void foofunc(void)
{
    Chan *a = CHOPEN(int);
    Chan *b = CHOPEN(int);
    Chan *c = CHOPEN(int);
    Chan *d = CHOPEN(int);

    GO(PAR(
           PROC(prefix, a, b),
           PROC(delta, a, c, d),
           PROC(successor, b, c)
       )
    );

    RUN( PROC(consumer, d) );

    CHCLOSE(a);
    CHCLOSE(b);
    CHCLOSE(c);
    CHCLOSE(d);
}

int main(void)
{
    proxc_start(foofunc);
    return 0;
}

