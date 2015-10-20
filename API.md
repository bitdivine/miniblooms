
# Minibloom API

## Lua procedures

### make(str:filename, int:capacity, real:error_rate) -> bloomfile:
Create a new bloomfile.

### open(str:filename) -> bloomfile:
Open an existing bloomfile.

### close(bloomfile:) -> -
Close a bloomfile.  This releases the memory and closes the file handle.

### bloom(bloomfile:) -> bloom:
Get the actual filter out from inside the bloomfile.

### get(bloom:filter, str:key) -> int:found
Returns 1 if "key" is in the filter and 0 otherwise.

### set(bloom:filter, str:key)
Sets the key.

## Available to C and may be added to Lua:
* Get the stats such as the size, number of set bits, number of insertions etc from a bloom filter.

You can see what is available by creating a bloomfile:

    seq 100 | while read line ; do echo $((i % 66)) ; done | minibloomcat -c100 -e0.1 -S ,toy
    # Or looking at an existing file:
    minibloomcat -s ,toy
    # Or as JSON:
    minibloomcat -j ,toy

* A function to create a filter without a file.

