#pragma once
#include <lua.hpp>
#include <stdexcept>
#include <string>
#include "cartridge.hpp"


class Lua : public Cartridge {
    static constexpr int IDX_SELF = 1;
    static constexpr int IDX_READ_PRG = 2;
    static constexpr int IDX_READ_CHR = 3;
    static constexpr int IDX_WRITE_PRG = 4;
    static constexpr int IDX_WRITE_CHR = 5;
    static constexpr int IDX_STEP = 6;

public:
    explicit Lua() : L(luaL_newstate()) { luaL_openlibs(L); }
    ~Lua() { if(L) lua_close(L); }


    void open(const std::string& path) {
        lua_pushlightuserdata(L, this);
        lua_setglobal(L, "__cpp_instance");

        regVars(L);

        if(luaL_dofile(L, path.c_str()) != LUA_OK)
            throw std::runtime_error("[LUA]: "+std::string(lua_tostring(L, -1)));

        if (!lua_istable(L, -1))
            throw std::runtime_error("[LUA]: Скрипт должен возвращать таблицу");


        /* Вызываем init */
        lua_getfield(L, -1, "init");
        if (lua_isfunction(L, -1)) {
            lua_pushvalue(L, -2);
            lua_call(L, 1, 0);
        } else {
            lua_pop(L, 1);
        }

        /* Кэшируем функции на стеке */
        cacheFunc("readPRGAddr");
        cacheFunc("readCHRAddr");
        cacheFunc("writePRGAddr");
        cacheFunc("writeCHRAddr");
        cacheFunc("step");

        hasReadPRG = !lua_isnil(L, IDX_READ_PRG);
        hasReadCHR = !lua_isnil(L, IDX_READ_CHR);
        hasWritePRG = !lua_isnil(L, IDX_WRITE_PRG);
        hasWriteCHR = !lua_isnil(L, IDX_WRITE_CHR);
        hasStep = !lua_isnil(L, IDX_STEP);
    }

public:
    inline u32 readPRGAddr(u16 addr) {
        if (!hasReadPRG) return 0xFFFFFFFF;
        lua_pushvalue(L, IDX_READ_PRG);
        lua_pushvalue(L, IDX_SELF);
        lua_pushinteger(L, addr);
        lua_call(L, 2, 1);
        u32 r = static_cast<u32>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return r;
    }

    inline u32 readCHRAddr(u16 addr) {
        if (!hasReadCHR) return 0xFFFFFFFF;
        lua_pushvalue(L, IDX_READ_CHR);
        lua_pushvalue(L, IDX_SELF);
        lua_pushinteger(L, addr);
        lua_call(L, 2, 1);
        u32 r = static_cast<u32>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return r;
    }

    inline u32 writePRGAddr(u16 addr, u8 value) {
        if (!hasWritePRG) return 0xFFFFFFFF;
        lua_pushvalue(L, IDX_WRITE_PRG);
        lua_pushvalue(L, IDX_SELF);
        lua_pushinteger(L, addr);
        lua_pushinteger(L, value);
        lua_call(L, 3, 1);
        if (lua_isnil(L, -1)) { lua_pop(L, 1); return 0xFFFFFFFF; }
        u32 r = static_cast<u32>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return r;
    }

    inline u32 writeCHRAddr(u16 addr, u8 value) {
        if (!hasWriteCHR) return 0xFFFFFFFF;
        lua_pushvalue(L, IDX_WRITE_CHR);
        lua_pushvalue(L, IDX_SELF);
        lua_pushinteger(L, addr);
        lua_pushinteger(L, value);
        lua_call(L, 3, 1);
        if (lua_isnil(L, -1)) { lua_pop(L, 1); return 0xFFFFFFFF; }
        u32 r = static_cast<u32>(lua_tointeger(L, -1));
        lua_pop(L, 1);
        return r;
    }

    inline void step() {
        if (!hasStep) return;
        lua_pushvalue(L, IDX_STEP);
        lua_pushvalue(L, IDX_SELF);
        lua_call(L, 1, 0);
    }

private:
    lua_State *L;
    bool hasReadPRG{false};
    bool hasReadCHR{false};
    bool hasWritePRG{false};
    bool hasWriteCHR{false};
    bool hasStep{false};

private:
    void regVars(lua_State* L) {
        /* mirrorMode */
        lua_pushinteger(L, mirrorMode);
        lua_setglobal(L, "mirrorMode");

        /* mapperNum */
        lua_pushinteger(L, mapperNumber);
        lua_setglobal(L, "mapperNum");

        /* prgSize */
        lua_pushinteger(L, PRG_ROM.size());
        lua_setglobal(L, "prgSize");

        /* chrSize */
        lua_pushinteger(L, CHR_ROM.size());
        lua_setglobal(L, "chrSize");


        regLuaFunc("CHRresize", call_CHRresize);
        regLuaFunc("TriggerIRQ", call_TriggerIRQ);
        regLuaFunc("ClearIRQ", call_ClearIRQ);
        regLuaFunc("SetMirror", call_SetMirror);
    }

    /* Кладёт функцию (или nil) на стек */
    void cacheFunc(const char* name) {
        lua_getfield(L, IDX_SELF, name);
    }

private:
    template<typename Func>
    static int lua_wrapper(lua_State* L) {
        lua_getglobal(L, "__cpp_instance");
        Lua* instance = static_cast<Lua*>(lua_touserdata(L, -1));
        lua_pop(L, 1);

        if (!instance)
            return luaL_error(L, "Не удалось получить экземпляр класса");

        Func func = reinterpret_cast<Func>(lua_touserdata(L, lua_upvalueindex(1)));
        return (*func)(L, instance);
    }

    template<typename Func>
    void regLuaFunc(const char* name, Func func) {
        lua_pushlightuserdata(L, reinterpret_cast<void*>(func));
        lua_pushcclosure(L, lua_wrapper<Func>, 1);
        lua_setglobal(L, name);
    }


    /* C++ api для Lua */
    static int call_CHRresize(lua_State* L, Lua* instance) {
        instance->CHR_ROM.resize(static_cast<u16>(luaL_checkinteger(L, 1)));
        return 0;
    }

    static int call_TriggerIRQ(lua_State*, Lua* instance) {
        instance->irqFlag = true;
        return 0;
    }

    static int call_ClearIRQ(lua_State*, Lua* instance) {
        instance->irqFlag = false;
        return 0;
    }

    static int call_SetMirror(lua_State* L, Lua* instance) {
        instance->mirrorMode = static_cast<u8>(luaL_checkinteger(L, 1));
        return 0;
    }
};
