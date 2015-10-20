#!/usr/bin/lua
if package.cpath
then
	package.cpath = "./?.so;" .. package.cpath
else
	package.cpath = "./?.so"
end
if package.path
then
	package.path = "./?.lua;" .. package.path
else
	package.path = "./?.lua"
end

local minibloom = require "minibloom"
local bloomfile, filter

if arg[1] then
    bloomfile = minibloom.open(arg[1])
else
    error("Usage: lua bloomlookup.lua <bloomfilter> < ids.txt")
end

print(bloomfile)

filter = minibloom.bloom(bloomfile)
for line in io.lines() do
    -- local k = gsub(line, "\n$", "")
    local v = minibloom.get(filter, line);
    print(line .. "," .. v)
end
minibloom.close(bloomfile)
