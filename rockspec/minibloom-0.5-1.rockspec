package = "minibloom"
version = "0.5-1"
source = {
	url = "..." -- We don't have one yet
}
description = {
	summary = "An example for the LuaRocks tutorial.",
	detailed = [[
		This is an example for the LuaRocks tutorial.
		Here we would put a detailed, typically
		paragraph-long description.
	]],
	homepage = "https://github.com/bitdivine/minibloom",
	license = "MIT"
}
dependencies = {
	"lua ~> 5.1"
}
build = {
	type = "make",
	install_pass = false,
}
