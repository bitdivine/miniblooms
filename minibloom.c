#include <lua.h>
#include "lauxlib.h"
#include "lualib.h"

#include <string.h>
#include <errno.h>
#include <limits.h>
#include "minibloom.h"
#include "minibloomfile.h"

/* Lua exception */
static int lua_oops(lua_State *L, const char *s)
{
	char buffer[1024];
	if(!s) return 0;

	snprintf(buffer, sizeof(buffer), "%s: error: %s", s, strerror(errno));
	buffer[sizeof(buffer)-1] = '\0';
	lua_pushstring(L, buffer);
	return lua_error(L);
}
static int lua_oops_str(lua_State *L, const char *s, char* ss)
{
	char buffer[1024];
	if(!s) return 0;

	snprintf(buffer, sizeof(buffer), "%s: '%s': error: %s", s, ss, strerror(errno));
	buffer[sizeof(buffer)-1] = '\0';
	lua_pushstring(L, buffer);
	return lua_error(L);
}

static int luabind_minimake(lua_State *L)
{
	minibloomfile_t*	mib;
	int			err;
	const char*		filename;
	unsigned int		capacity;
	double			error_rate;

	// Args:
	filename = lua_tolstring(L, -3, NULL);
	if (!filename) {
		return lua_oops(L, "minibloom_create_filter: missing filename");
	}
	if (!lua_isnumber(L, -2)) {
		return lua_oops(L, "minibloom_create_filter: bad capacity");
	}
	capacity = lua_tointeger(L, -2);
	if (!lua_isnumber(L, -1)) {
		return lua_oops(L, "minibloom_create_filter: bad error rate");
	}
	error_rate = lua_tonumber(L, -1);

	// Create the new bloom filter:
	mib = (minibloomfile_t*) lua_newuserdata (L, sizeof(minibloomfile_t));
	err = minimake(mib, filename, capacity, error_rate);
	if (err) {
		lua_pop(L,1);
		return lua_oops(L, "minimake: error making bloom filter");
	}
	return 1;
}

static int luabind_miniload(lua_State *L)
{
	minibloomfile_t*mib;
	int		append;
	const char *	filename;

	// Arguments:
	filename = lua_tolstring(L, -1, NULL);
	if (!filename) {
		return lua_oops(L, "minibloom_open: missing filename");
	}
	append = lua_tointeger(L, -2); // If missing this should be 0 which is fine.  0 == readonly

	mib = (minibloomfile_t*) lua_newuserdata (L, sizeof(minibloomfile_t));
	if (miniload(mib, filename, append)) {
		lua_pop(L,1);
		return lua_oops_str(L, "minibloom_open: error opening mib filter", filename);
	}
	return 1;
}

static int luabind_miniclose(lua_State *L)
{
	minibloomfile_t*	mib;

	if (!lua_isuserdata(L, -1)) {
		return lua_oops(L, "minibloom_close: not a minibloom filter");
	}
	mib = (minibloomfile_t*) lua_touserdata(L, -1);
	if (minicheckfilehandle(mib)) {
		return lua_oops(L, "minibloom_close: not a minibloom filter");
	}

	miniclose(mib);
	return 0;
}

static int luabind_minigetbloom(lua_State *L)
{
	minibloomfile_t*	mib;
	if (!lua_isuserdata(L,-1)){
		return lua_oops(L, "minibloom_getbloom: not a minibloom filter: wrong type");
	}
	mib = lua_touserdata(L, -1);
	if (minicheckfilehandle(mib)) {
		return lua_oops(L, "minibloom_getbloom: not a minibloom filter: bad content");
	}
	
	lua_pushlightuserdata(L, mib->bloom);
	return 1;
}

static int luabind_miniget(lua_State *L)
{
	minibloom_t* mib;
	const char *key;
	size_t keylen;

	if (!lua_isuserdata(L, -2)) {
		return lua_oops(L, "minibloom_get: not minibloom filter");
	}
	mib = lua_touserdata(L, -2);
	key = lua_tolstring(L, -1, &keylen);
	if (!key) {
		return lua_oops(L, "minibloom_get: missing key");
	}

	int v = miniget(mib, key, keylen);
	lua_pushinteger(L, v);
	return 1;
}

static int luabind_miniset(lua_State *L)
{
	minibloom_t* mib;
	const char *key;
	size_t keylen;

	if (!lua_islightuserdata(L, -2)) {
		return lua_oops(L, "minibloom_set: not a minibloom filter");
	}
	mib = lua_touserdata(L, -2);
	key = lua_tolstring(L, -1, &keylen);
	if (!key) {
		return lua_oops(L, "minibloom_set: missing key");
	}
	miniset(mib, key, keylen);
	return 0;
}

static const luaL_Reg api[] = {
	{"make" , luabind_minimake},
	{"open" , luabind_miniload},
	{"close", luabind_miniclose},
	{"bloom", luabind_minigetbloom},
	{"get"  , luabind_miniget},
	{"set"  , luabind_miniset},
	{NULL, NULL}
};

int luaopen_minibloom (lua_State *L) {
#if LUA_VERSION_NUM <= 501
	luaL_register(L, "minibloom", api); /* 5.1 */
#else
	luaL_newlib(L, api); /* 5.2 */
#endif
	return 1;
}
