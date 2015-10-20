#!/usr/bin/lua
-- This is uniq implemented with a bloom filter.
-- Run with:
-- printf "%s\n" abra cadabra abra cadabra | lua examples/uniq.lua
-- Should print each input line exactly once.


local minilibdir=".."
if package.cpath
then
	package.cpath = package.cpath .. ";" .. minilibdir .. "/lib/?.so"
else
	package.cpath = minilibdir .. "/lib/?.so"
end
if package.path
then
	package.path = package.path .. ";" .. minilibdir .. "/lua/?.lua"
else
	package.path = minilibdir .. "/lua/?.lua"
end

local minibloom = require "minibloom"
local bloomfile, filter

bloomfile = minibloom.make(",miniuniq", 1000, 0.1)

filter = minibloom.bloom(bloomfile)
for line in io.lines() do
    if (0 == minibloom.get(filter, line)) then minibloom.set(filter, line) ; print(line) ; end
end
minibloom.close(bloomfile)
