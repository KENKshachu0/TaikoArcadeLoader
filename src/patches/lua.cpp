#include "constants.h"
#include "helpers.h"
#include "patches.h"

namespace patches::Lua {
    #define LUA_GLOBALSINDEX -10002
    typedef void * (*lua_Alloc) (void *ud, void *ptr, size_t osize, size_t nsize);

    LuaState globalLuaState = 0;

    FUNCTION_PTR (lua_CFunction, lua_atpanic, PROC_ADDRESS ("lua51.dll", "lua_atpanic"), LuaState, lua_CFunction);
    FUNCTION_PTR (void, lua_call, PROC_ADDRESS ("lua51.dll", "lua_call"), LuaState, int, int);
    FUNCTION_PTR (int, lua_checkstack, PROC_ADDRESS ("lua51.dll", "lua_checkstack"), LuaState, int);
    FUNCTION_PTR (void, lua_concat, PROC_ADDRESS ("lua51.dll", "lua_concat"), LuaState, int);
    FUNCTION_PTR (int, lua_cpcall, PROC_ADDRESS ("lua51.dll", "lua_cpcall"), LuaState, lua_CFunction, void *);
    FUNCTION_PTR (void, lua_createtable, PROC_ADDRESS ("lua51.dll", "lua_createtable"), LuaState, int, int);
    FUNCTION_PTR (int, lua_equal, PROC_ADDRESS ("lua51.dll", "lua_equal"), LuaState, int, int);
    FUNCTION_PTR (int, lua_error, PROC_ADDRESS ("lua51.dll", "lua_error"), LuaState);
    FUNCTION_PTR (void, lua_getfenv, PROC_ADDRESS ("lua51.dll", "lua_getfenv"), LuaState, int);
    FUNCTION_PTR (void, lua_getfield, PROC_ADDRESS ("lua51.dll", "lua_getfield"), LuaState, int, const char *);
    #define lua_getglobal(L,s)  lua_getfield(L, LUA_GLOBALSINDEX, s)
    // FUNCTION_PTR (void, lua_getglobal, PROC_ADDRESS ("lua51.dll", "lua_getglobal"), LuaState, const char *);
    FUNCTION_PTR (void, lua_getlocal, PROC_ADDRESS ("lua51.dll", "lua_getlocal"), LuaState, const char *);
    FUNCTION_PTR (int, lua_getmetatable, PROC_ADDRESS ("lua51.dll", "lua_getmetatable"), LuaState, int);
    FUNCTION_PTR (void, lua_gettable, PROC_ADDRESS ("lua51.dll", "lua_gettable"), LuaState, int);
    FUNCTION_PTR (int, lua_gettop, PROC_ADDRESS ("lua51.dll", "lua_gettop"), LuaState);
    FUNCTION_PTR (void, lua_insert, PROC_ADDRESS ("lua51.dll", "lua_gettop"), LuaState, int);
    FUNCTION_PTR (int, lua_isboolean, PROC_ADDRESS ("lua51.dll", "lua_isboolean"), LuaState, int);
    FUNCTION_PTR (int, lua_iscfunction, PROC_ADDRESS ("lua51.dll", "lua_iscfunction"), LuaState, int);
    FUNCTION_PTR (int, lua_isfunction, PROC_ADDRESS ("lua51.dll", "lua_isfunction"), LuaState, int);
    FUNCTION_PTR (int, lua_islightuserdata, PROC_ADDRESS ("lua51.dll", "lua_islightuserdata"), LuaState, int);
    FUNCTION_PTR (int, lua_isnil, PROC_ADDRESS ("lua51.dll", "lua_isnil"), LuaState, int);
    FUNCTION_PTR (int, lua_isnone, PROC_ADDRESS ("lua51.dll", "lua_isnone"), LuaState, int);
    FUNCTION_PTR (int, lua_isnoneornil, PROC_ADDRESS ("lua51.dll", "lua_isnoneornil"), LuaState, int);
    FUNCTION_PTR (int, lua_isnumber, PROC_ADDRESS ("lua51.dll", "lua_isnumber"), LuaState, int);
    FUNCTION_PTR (int, lua_isstring, PROC_ADDRESS ("lua51.dll", "lua_isstring"), LuaState, int);
    FUNCTION_PTR (int, lua_istable, PROC_ADDRESS ("lua51.dll", "lua_istable"), LuaState, int);
    FUNCTION_PTR (int, lua_isthread, PROC_ADDRESS ("lua51.dll", "lua_isthread"), LuaState, int);
    FUNCTION_PTR (int, lua_isuserdata, PROC_ADDRESS ("lua51.dll", "lua_isuserdata"), LuaState, int);
    FUNCTION_PTR (int, lua_lessthan, PROC_ADDRESS ("lua51.dll", "lua_lessthan"), LuaState, int, int);
    FUNCTION_PTR (void, lua_newtable, PROC_ADDRESS ("lua51.dll", "lua_newtable"), LuaState);
    FUNCTION_PTR (LuaState, lua_newthread, PROC_ADDRESS ("lua51.dll", "lua_newthread"), LuaState);
    FUNCTION_PTR (void *, lua_newuserdata, PROC_ADDRESS ("lua51.dll", "lua_newuserdata"), LuaState, size_t);
    FUNCTION_PTR (int, lua_next, PROC_ADDRESS ("lua51.dll", "lua_next"), LuaState, int);
    FUNCTION_PTR (size_t, lua_objlen, PROC_ADDRESS ("lua51.dll", "lua_objlen"), LuaState, int);
    FUNCTION_PTR (int, lua_pcall, PROC_ADDRESS ("lua51.dll", "lua_pcall"), LuaState, int, int, int);
    FUNCTION_PTR (void, lua_pop, PROC_ADDRESS ("lua51.dll", "lua_pop"), LuaState, int);
    FUNCTION_PTR (void, lua_pushboolean, PROC_ADDRESS ("lua51.dll", "lua_pushboolean"), LuaState, int);
    FUNCTION_PTR (void, lua_pushcclosure, PROC_ADDRESS ("lua51.dll", "lua_pushcclosure"), LuaState, lua_CFunction, int);
    FUNCTION_PTR (void, lua_pushcfunction, PROC_ADDRESS ("lua51.dll", "lua_pushcfunction"), LuaState, lua_CFunction);
    FUNCTION_PTR (const char *, lua_pushfstring, PROC_ADDRESS ("lua51.dll", "lua_pushfstring"), LuaState, const char *, ...);
    FUNCTION_PTR (void, lua_pushinteger, PROC_ADDRESS ("lua51.dll", "lua_pushinteger"), LuaState, i64);
    FUNCTION_PTR (void, lua_pushlightuserdata, PROC_ADDRESS ("lua51.dll", "lua_pushlightuserdata"), LuaState, void *);
    FUNCTION_PTR (void, lua_pushliteral, PROC_ADDRESS ("lua51.dll", "lua_pushliteral"), LuaState, const char *);
    FUNCTION_PTR (void, lua_pushlstring, PROC_ADDRESS ("lua51.dll", "lua_pushlstring"), LuaState, const char *, size_t);
    FUNCTION_PTR (void, lua_pushnil, PROC_ADDRESS ("lua51.dll", "lua_pushnil"), LuaState);
    FUNCTION_PTR (void, lua_pushnumber, PROC_ADDRESS ("lua51.dll", "lua_pushnumber"), LuaState, double);
    FUNCTION_PTR (void, lua_pushstring, PROC_ADDRESS ("lua51.dll", "lua_pushstring"), LuaState, const char *);
    FUNCTION_PTR (int, lua_pushthread, PROC_ADDRESS ("lua51.dll", "lua_pushthread"), LuaState);
    FUNCTION_PTR (void, lua_pushvalue, PROC_ADDRESS ("lua51.dll", "lua_pushvalue"), LuaState, int);
    FUNCTION_PTR (void, lua_pushvfstring, PROC_ADDRESS ("lua51.dll", "lua_pushvfstring"), LuaState, const char *, va_list);
    FUNCTION_PTR (void, lua_rawget, PROC_ADDRESS ("lua51.dll", "lua_rawget"), LuaState, int);
    FUNCTION_PTR (void, lua_rawgeti, PROC_ADDRESS ("lua51.dll", "lua_rawgeti"), LuaState, int, int);
    FUNCTION_PTR (void, lua_rawset, PROC_ADDRESS ("lua51.dll", "lua_rawset"), LuaState, int);
    FUNCTION_PTR (void, lua_rawseti, PROC_ADDRESS ("lua51.dll", "lua_rawseti"), LuaState, int, int);
    FUNCTION_PTR (void, lua_remove, PROC_ADDRESS ("lua51.dll", "lua_remove"), LuaState, int);
    FUNCTION_PTR (void, lua_replace, PROC_ADDRESS ("lua51.dll", "lua_replace"), LuaState, int);
    FUNCTION_PTR (int, lua_resume, PROC_ADDRESS ("lua51.dll", "lua_resume"), LuaState, int);
    FUNCTION_PTR (int, lua_setfenv, PROC_ADDRESS ("lua51.dll", "lua_setfenv"), LuaState, int);
    FUNCTION_PTR (void, lua_setfield, PROC_ADDRESS ("lua51.dll", "lua_setfield"), LuaState, int, const char *);
    FUNCTION_PTR (void, lua_setglobal, PROC_ADDRESS ("lua51.dll", "lua_setglobal"), LuaState, const char *);
    FUNCTION_PTR (int, lua_setmetatable, PROC_ADDRESS ("lua51.dll", "lua_setmetatable"), LuaState, int);
    FUNCTION_PTR (void, lua_settable, PROC_ADDRESS ("lua51.dll", "lua_settable"), LuaState, int);
    FUNCTION_PTR (void, lua_settop, PROC_ADDRESS ("lua51.dll", "lua_settop"), LuaState, int);
    FUNCTION_PTR (int, lua_status, PROC_ADDRESS ("lua51.dll", "lua_status"), LuaState);
    FUNCTION_PTR (int, lua_toboolean, PROC_ADDRESS ("lua51.dll", "lua_toboolean"), LuaState, int);
    FUNCTION_PTR (lua_CFunction, lua_tocfunction, PROC_ADDRESS ("lua51.dll", "lua_tocfunction"), LuaState, int);
    FUNCTION_PTR (i64, lua_tointeger, PROC_ADDRESS ("lua51.dll", "lua_tointeger"), LuaState, int);
    FUNCTION_PTR (const char *, lua_tolstring, PROC_ADDRESS ("lua51.dll", "lua_tolstring"), LuaState, int, size_t *);
    FUNCTION_PTR (double, lua_tonumber, PROC_ADDRESS ("lua51.dll", "lua_tonumber"), LuaState, int);
    FUNCTION_PTR (void *, lua_topointer, PROC_ADDRESS ("lua51.dll", "lua_topointer"), LuaState, int);
    FUNCTION_PTR (const char *, lua_tostring, PROC_ADDRESS ("lua51.dll", "lua_tostring"), LuaState, int);
    FUNCTION_PTR (LuaState, lua_tothread, PROC_ADDRESS ("lua51.dll", "lua_tothread"), LuaState, int);
    FUNCTION_PTR (void *, lua_touserdata, PROC_ADDRESS ("lua51.dll", "lua_touserdata"), LuaState, int);
    FUNCTION_PTR (int, lua_type, PROC_ADDRESS ("lua51.dll", "lua_type"), LuaState, int);
    FUNCTION_PTR (const char *, lua_typename, PROC_ADDRESS ("lua51.dll", "lua_typename"), LuaState, int);
    FUNCTION_PTR (i32, luaL_loadstring, PROC_ADDRESS ("lua51.dll", "luaL_loadstring"), i64, const char *);

    FAST_HOOK (i64, luaL_newstate, PROC_ADDRESS ("lua51.dll", "luaL_newstate")) {
        // LogMessage (LogLevel::INFO, "Lua::NewState");
        globalLuaState = originalluaL_newstate.call<i64> ();

        // LogMessage (LogLevel::INFO, "1");
        // lua_getglobal (globalLuaState, "package");
        // LogMessage (LogLevel::INFO, "2");
        // lua_getfield (globalLuaState, -1, "path");
        // LogMessage (LogLevel::INFO, "3");
        // const char *path = lua_tostring(globalLuaState, -1);
        // LogMessage (LogLevel::INFO, "4");
        // lua_pop (globalLuaState, 2);

        // LogMessage (LogLevel::INFO, "Lua package.path={}", path);

        return globalLuaState;
    }
    #define LUA_MULTRET         (-1)
    #define luaL_dostring(L, s) (luaL_loadstring (L, s) || lua_pcall (L, 0, LUA_MULTRET, 0))

    void
    Init () {
        INSTALL_FAST_HOOK (luaL_newstate);
    }

    void
    Execute (std::string code) {
        luaL_dostring (globalLuaState, code.c_str ());
    }

    void
    RegisterMethod (std::string base, std::string methodName, lua_CFunction method) {
        return;     // WAIT IMPLEMENT
    }

    void
    SetTop (i64 lua_State, i32 index) {
        lua_settop (lua_State, index);
    }

    void
    Replace (i64 lua_State, i32 index) {
        lua_replace (lua_State, index);
    }

    void
    PushCClosure (i64 lua_State, lua_CFunction func, i32 index) {
        lua_pushcclosure (lua_State, func, index);
    }

    void
    PushBoolean (i64 lua_State, bool value) {
        lua_pushboolean (lua_State, value);
    }

    void
    PushString (i64 lua_State, std::string value) {
        lua_pushstring (lua_State, value.c_str());
    }

    bool
    ToBoolean (i64 lua_State, i32 index) {
        return lua_toboolean (lua_State, index);
    }

    const char *
    ToLString (i64 lua_State, i32 index, size_t *size) {
        return lua_tolstring(lua_State, index, size);
    }
}