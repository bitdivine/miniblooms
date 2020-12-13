MiniBlooms
==========

This is a small, simple and fast implementation of bloom filters.  The default bloom filter implementation in Lua seems to be dablooms, which were good to get me started but they are too large and slow for my purposes, so miniblooms were born.  Their speed stems from a good understanding of the mathematics underlying bloom filters and how to optimise it in a cache friendly way.

## Build

    make

Hopefully something like this will eventually work, however I'm unfamiliar with luarocks so will have to take some time sometime to figure out how:

    sudo luarocks install miniblooms

## API

There are bindings for C, Lua and shell:

## Lua API:

	make(filename,capacity,error) -> bloomfile
	open(filename) -> bloomfile
	close(minibloom_handle)
	bloom(bloomfile) -> filter
	get(filter,key) -> bool
	set(filter,key)

Where:

* `string:filename` is the relative or absolute path to the bloom filter.
* `int:capacity` is the number of unique entries the bloom filter is expected to cope with.
* `float:error` is the probability of a false positive, i.e. the probability of the filter recognising something that it has in fact not seen before.
* `string:key` is a value to be marked or looked up in the bloom filter.


### Lua Example:

This is the UNIX command line "uniq" without any sorting.  It reads lines from stdin and writes them to stdout if they have not been seen before.

	local minibloom = require "minibloom"
	local bloomfile, filter

	bloomfile = minibloom.make(",miniuniq", 10000, 0.01)

	filter = minibloom.bloom(bloomfile)
	for line in io.lines() do
	    if (0 == minibloom.get(filter, line)) then minibloom.set(filter, line) ; print(line) ; end
	end
	minibloom.close(bloomfile)

## Availabile from

* IPFS
  * ipfs://bafybeifwkid24jw3ll2up3lo6vifr6eahuce3ag7jkerm3g3nfr5ebus7q (CIDv2)
  * https://gateway.ipfs.io/ipfs/bafybeifwkid24jw3ll2up3lo6vifr6eahuce3ag7jkerm3g3nfr5ebus7q (CIDv2)
  * CIDv1 (old):
    * ipfs://QmacMTXuR61uSz1AcqZdsRk5PjqTQRn66YeHL5du4K9c4b (CIDv1)
    * https://gateway.ipfs.io/ipfs/QmacMTXuR61uSz1AcqZdsRk5PjqTQRn66YeHL5du4K9c4b (CIDv1)
* Github: https://github.com/bitdivine/miniblooms
