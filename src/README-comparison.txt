
Miniblooms are set to use 1 bit rather 1 nybble per cell, so the bloom filters are smaller.

In addition I have changed the definition of "optimal" to take into account cache misses when computing the parameters of the bloom filters.
As you see, the number of cache misses is halved and could be smaller yet, at the expense of larger filters, were that to suit our operating context.  We could, for example, keep the bloom filters the same size as with dablooms but have a much smaller number of memory lookups.



== C A C H E G R I N D ==

Minibloom vs dabloom:

mmurphy@violet:~/minibloom/src$ time seq 1000000 | valgrind --tool=cachegrind ./minibloomcat -c1000000 -e0.0001 -u ,mb >/dev/null
==25653== Cachegrind, a cache and branch-prediction profiler
==25653== Copyright (C) 2002-2011, and GNU GPL'd, by Nicholas Nethercote et al.
==25653== Using Valgrind-3.7.0 and LibVEX; rerun with -h for copyright info
==25653== Command: ./minibloomcat -c1000000 -e0.0001 -u ,mb
==25653== 
--25653-- warning: L3 cache found, using its data for the LL simulation.
==25653== 
==25653== I   refs:      940,512,172
==25653== I1  misses:          1,353
==25653== LLi misses:          1,308
==25653== I1  miss rate:        0.00%
==25653== LLi miss rate:        0.00%
==25653== 
==25653== D   refs:      428,154,172  (322,022,649 rd   + 106,131,523 wr)
==25653== D1  misses:      7,093,598  (  7,045,398 rd   +      48,200 wr)
==25653== LLd misses:         50,186  (      2,112 rd   +      48,074 wr)
==25653== D1  miss rate:         1.6% (        2.1%     +         0.0%  )
==25653== LLd miss rate:         0.0% (        0.0%     +         0.0%  )
==25653== 
==25653== LL refs:         7,094,951  (  7,046,751 rd   +      48,200 wr)
==25653== LL misses:          51,494  (      3,420 rd   +      48,074 wr)
==25653== LL miss rate:          0.0% (        0.0%     +         0.0%  )

real	0m12.017s
user	0m12.513s
sys	0m0.108s
mmurphy@violet:~/dabloom/src$ time seq 1000000 | valgrind --tool=cachegrind ./bloomcat -c1000000 -e0.0001 -u ,db >/dev/null
==25871== Cachegrind, a cache and branch-prediction profiler
==25871== Copyright (C) 2002-2011, and GNU GPL'd, by Nicholas Nethercote et al.
==25871== Using Valgrind-3.7.0 and LibVEX; rerun with -h for copyright info
==25871== Command: ./bloomcat -c1000000 -e0.0001 -u ,db
==25871== 
--25871-- warning: L3 cache found, using its data for the LL simulation.
==25871== 
==25871== I   refs:      4,390,019,775
==25871== I1  misses:            1,392
==25871== LLi misses:            1,363
==25871== I1  miss rate:          0.00%
==25871== LLi miss rate:          0.00%
==25871== 
==25871== D   refs:      2,336,663,104  (1,706,073,055 rd   + 630,590,049 wr)
==25871== D1  misses:       14,367,483  (   14,076,755 rd   +     290,728 wr)
==25871== LLd misses:        7,906,918  (    7,906,237 rd   +         681 wr)
==25871== D1  miss rate:           0.6% (          0.8%     +         0.0%  )
==25871== LLd miss rate:           0.3% (          0.4%     +         0.0%  )
==25871== 
==25871== LL refs:          14,368,875  (   14,078,147 rd   +     290,728 wr)
==25871== LL misses:         7,908,281  (    7,907,600 rd   +         681 wr)
==25871== LL miss rate:            0.1% (          0.1%     +         0.0%  )

real	0m53.562s
user	0m53.915s
sys	0m0.220s
mmurphy@violet:~/minibloom/src$ 


== R U N T I M E ==

The times are inflated by the use of cachegrind.  Here are the real times for making a 10M table.  Repeated runs give similar results.  As I don't have a grepper written in C for dablooms I am making use of the fact that setting an entry is similar in cost to the worst case scenario for checking one.

mmurphy@violet:~/minibloom/src$ time seq 10000000 | ./minibloomcat -c10000000 -e0.0001 -u ,mb >/dev/null

real	0m6.298s
user	0m11.625s
sys	0m0.236s
mmurphy@violet:~/minibloom/src$ ls -lh ,mb 
-rw-r--r-- 1 mmurphy mmurphy 30M Mar  2 00:22 ,mb
mmurphy@violet:~/minibloom/src$ 
mmurphy@violet:~/dabloom/src$ time seq 10000000 | ./bloomcat -c10000000 -e0.0001 -u ,db >/dev/null

real	1m8.217s
user	0m33.706s
sys	0m16.561s
mmurphy@violet:~/dabloom/src$ ls -lh ,db
-rw------- 1 mmurphy mmurphy 92M Mar  2 00:23 ,db
mmurphy@violet:~/dabloom/src$ 


