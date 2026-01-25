#pragma once
extern "C" {
    #include <luajit-2.1/luajit.h>
    #include <luajit-2.1/lualib.h>
    #include <luajit-2.1/lauxlib.h>
}
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
    explicit Lua() : L(luaL_newstate()) {
        luaL_openlibs(L);
        luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_ON);
    }
    ~Lua() { if(L) lua_close(L); }


    void open(const std::string& path) {
        lua_pushlightuserdata(L, this);
        lua_setglobal(L, "__instance");

        luaL_dostring(L, "package.path = 'mappers/?.lua;' .. package.path");

        lua_pushinteger(L, PRG_ROM.size());
        lua_setglobal(L, "prgSize");

        lua_pushinteger(L, PRG_RAM.size());
        lua_setglobal(L, "prgRamSize");

        lua_pushinteger(L, CHR_ROM.size());
        lua_setglobal(L, "chrSize");


        /* Загружаем файл */
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

        /* Кэшируем функции */
        cacheFunc("readPRGAddr");
        cacheFunc("readCHRAddr");
        cacheFunc("writePRGAddr");
        cacheFunc("writeCHRAddr");
        cacheFunc("step");

        hasReadPRG  = !lua_isnil(L, IDX_READ_PRG);
        hasReadCHR  = !lua_isnil(L, IDX_READ_CHR);
        hasWritePRG = !lua_isnil(L, IDX_WRITE_PRG);
        hasWriteCHR = !lua_isnil(L, IDX_WRITE_CHR);
        hasStep     = !lua_isnil(L, IDX_STEP);
    }

public:
    inline u32 readPRGAddr(u16 addr) {
        if (!hasReadPRG) return 0xFFFFFFFF;
        lua_pushvalue(L, IDX_READ_PRG);
        lua_pushvalue(L, IDX_SELF);
        lua_pushinteger(L, addr);
        lua_call(L, 2, 1);
        u32 r = lua_tointeger(L, -1);
        lua_settop(L, IDX_STEP);
        return r;
    }

    inline u32 readCHRAddr(u16 addr) {
        if (!hasReadCHR) return 0xFFFFFFFF;
        lua_pushvalue(L, IDX_READ_CHR);
        lua_pushvalue(L, IDX_SELF);
        lua_pushinteger(L, addr);
        lua_call(L, 2, 1);
        u32 r = lua_tointeger(L, -1);
        lua_settop(L, IDX_STEP);
        return r;
    }

    inline u32 writePRGAddr(u16 addr, u8 value) {
        if (!hasWritePRG) return 0xFFFFFFFF;
        lua_pushvalue(L, IDX_WRITE_PRG);
        lua_pushvalue(L, IDX_SELF);
        lua_pushinteger(L, addr);
        lua_pushinteger(L, value);
        lua_call(L, 3, 1);
        if (lua_isnil(L, -1)) {
            lua_settop(L, IDX_STEP);
            return 0xFFFFFFFF;
        }
        u32 r = lua_tointeger(L, -1);
        lua_settop(L, IDX_STEP);
        return r;
    }

    inline u32 writeCHRAddr(u16 addr, u8 value) {
        if (!hasWriteCHR) return 0xFFFFFFFF;
        lua_pushvalue(L, IDX_WRITE_CHR);
        lua_pushvalue(L, IDX_SELF);
        lua_pushinteger(L, addr);
        lua_pushinteger(L, value);
        lua_call(L, 3, 1);
        if (lua_isnil(L, -1)) {
            lua_settop(L, IDX_STEP);
            return 0xFFFFFFFF;
        }
        u32 r = lua_tointeger(L, -1);
        lua_settop(L, IDX_STEP);
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

    void cacheFunc(const char* name) {
        lua_getfield(L, IDX_SELF, name);
    }
};



/* API */
extern "C"
{
    void Cartridge_resize(void* instance, int vecType, size_t size);
    void Cartridge_setMirror(void* instance, u8 mode);
    void Cartridge_triggerIRQ(void* instance);
    void Cartridge_clearIRQ(void* instance);
}
