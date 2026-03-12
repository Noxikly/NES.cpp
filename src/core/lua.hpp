#pragma once

extern "C" {
#include <luajit-2.1/lauxlib.h>
#include <luajit-2.1/lua.h>
#include <luajit-2.1/luajit.h>
#include <luajit-2.1/lualib.h>
}

#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "cartridge.hpp"
#include "common.hpp"

namespace Core {
class Lua : public Cartridge {
  protected:
    enum : u8 {
        IDX_SELF = 1,
        IDX_READ_PRG = 2,
        IDX_READ_CHR = 3,
        IDX_WRITE_PRG = 4,
        IDX_WRITE_CHR = 5,
        IDX_STEP = 6,
        IDX_SAVE_STATE = 7,
        IDX_LOAD_STATE = 8,
    };

  public:
    explicit Lua() : L(luaL_newstate()) {
        if (!L)
            throw std::runtime_error("[LUA]: Неудалось создать lua_State");
        luaL_openlibs(L);
        luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_ON);
    }
    ~Lua() {
        if (L)
            lua_close(L);
    }

    void open(const std::filesystem::path &path) {
        lua_settop(L, 0);
        lua_pushlightuserdata(L, this);
        lua_setglobal(L, "__instance");

        const std::filesystem::path mapperDir = path.parent_path();

        lua_getglobal(L, "package");
        lua_getfield(L, -1, "path");
        const char *oldPath = lua_tostring(L, -1);
        const std::string prevPath = oldPath ? oldPath : "";
        lua_pop(L, 1);

        const std::string newPath =
            (mapperDir / "?.lua").generic_string() + ";" + prevPath;
        lua_pushlstring(L, newPath.c_str(), newPath.size());
        lua_setfield(L, -2, "path");
        lua_pop(L, 1);

        lua_pushinteger(L, PRG_ROM.size());
        lua_setglobal(L, "prgSize");

        lua_pushinteger(L, PRG_RAM.size());
        lua_setglobal(L, "prgRamSize");

        lua_pushinteger(L, CHR_ROM.size());
        lua_setglobal(L, "chrSize");

        /* Загружаем файл */
        if (luaL_dofile(L, path.string().c_str()) != LUA_OK)
            throw std::runtime_error("[LUA]: " + luaError());

        if (!lua_istable(L, -1))
            throw std::runtime_error("[LUA]: Скрипт должен возвращать таблицу");

        /* Вызываем init */
        lua_getfield(L, -1, "init");
        if (lua_isfunction(L, -1)) {
            lua_pushvalue(L, -2);
            if (lua_pcall(L, 1, 0, 0) != LUA_OK)
                throw std::runtime_error("[LUA]: " + luaError());
        } else
            lua_pop(L, 1);

        /* Кэшируем функции */
        cacheFunc("readPRGAddr");
        cacheFunc("readCHRAddr");
        cacheFunc("writePRGAddr");
        cacheFunc("writeCHRAddr");
        cacheFunc("step");
        cacheFunc("saveState");
        cacheFunc("loadState");

        lua_settop(L, IDX_LOAD_STATE);

        hasReadPRG = !lua_isnil(L, IDX_READ_PRG);
        hasReadCHR = !lua_isnil(L, IDX_READ_CHR);
        hasWritePRG = !lua_isnil(L, IDX_WRITE_PRG);
        hasWriteCHR = !lua_isnil(L, IDX_WRITE_CHR);
        hasStep = !lua_isnil(L, IDX_STEP);
        hasSaveState = !lua_isnil(L, IDX_SAVE_STATE);
        hasLoadState = !lua_isnil(L, IDX_LOAD_STATE);
    }

  public:
    inline void step() {
        if (!hasStep)
            return;
        lua_pushvalue(L, IDX_STEP);
        lua_pushvalue(L, IDX_SELF);
        if (lua_pcall(L, 1, 0, 0) != LUA_OK)
            throwLuaError();
    }

    inline std::vector<u8> saveMapperState() {
        if (!hasSaveState)
            return {};
        lua_pushvalue(L, IDX_SAVE_STATE);
        lua_pushvalue(L, IDX_SELF);
        if (lua_pcall(L, 1, 1, 0) != LUA_OK)
            throwLuaError();

        if (lua_isnil(L, -1)) {
            lua_settop(L, IDX_LOAD_STATE);
            return {};
        }

        size_t len = 0;
        const char *ptr = lua_tolstring(L, -1, &len);
        std::vector<u8> out;
        out.resize(len);
        if (len && ptr)
            std::memcpy(out.data(), ptr, len);

        lua_settop(L, IDX_LOAD_STATE);
        return out;
    }

    inline void loadMapperState(const std::vector<u8> &data) {
        if (!hasLoadState)
            return;
        lua_pushvalue(L, IDX_LOAD_STATE);
        lua_pushvalue(L, IDX_SELF);
        lua_pushlstring(L, reinterpret_cast<const char *>(data.data()),
                        data.size());
        if (lua_pcall(L, 2, 0, 0) != LUA_OK)
            throwLuaError();
        lua_settop(L, IDX_LOAD_STATE);
    }

  protected:
    lua_State *L;
    bool hasReadPRG{false};
    bool hasReadCHR{false};
    bool hasWritePRG{false};
    bool hasWriteCHR{false};
    bool hasStep{false};
    bool hasSaveState{false};
    bool hasLoadState{false};

    inline u32 callFunc(int idx, u16 addr) {
        lua_pushvalue(L, idx);
        lua_pushvalue(L, IDX_SELF);
        lua_pushinteger(L, addr);
        if (lua_pcall(L, 2, 1, 0) != LUA_OK)
            throwLuaError();
        if (lua_isnil(L, -1)) {
            lua_settop(L, IDX_LOAD_STATE);
            return 0xFFFFFFFF;
        }
        const u32 r = static_cast<u32>(lua_tointeger(L, -1));
        lua_settop(L, IDX_LOAD_STATE);
        return r;
    }

    inline u32 callFunc(int idx, u16 addr, u8 value) {
        lua_pushvalue(L, idx);
        lua_pushvalue(L, IDX_SELF);
        lua_pushinteger(L, addr);
        lua_pushinteger(L, value);
        if (lua_pcall(L, 3, 1, 0) != LUA_OK)
            throwLuaError();
        if (lua_isnil(L, -1)) {
            lua_settop(L, IDX_LOAD_STATE);
            return 0xFFFFFFFF;
        }
        const u32 r = static_cast<u32>(lua_tointeger(L, -1));
        lua_settop(L, IDX_LOAD_STATE);
        return r;
    }

    inline std::string luaError() {
        const char *err = lua_tostring(L, -1);
        return err ? std::string(err) : std::string("Неизвестная ошибка");
    }

    void throwLuaError() {
        const std::string err = luaError();
        lua_settop(L, IDX_LOAD_STATE);
        throw std::runtime_error("[LUA]: " + err);
    }

  private:
    void cacheFunc(const char *name) {
        lua_getfield(L, IDX_SELF, name);
        if (!lua_isfunction(L, -1)) {
            lua_pop(L, 1);
            lua_pushnil(L);
        }
    }
};

} // namespace Core

/* API */
#ifdef _WIN32
#define API_EXPORT extern "C" __declspec(dllexport)
#else
#define API_EXPORT extern "C"
#endif

enum VecType : u8 {
    VEC_PRG_ROM = 0,
    VEC_PRG_RAM = 1,
    VEC_CHR_ROM = 2,
};

API_EXPORT void Cartridge_resize(void *instance, u8 vecType, size_t size);
API_EXPORT void Cartridge_setMirror(void *instance, u8 mode);
API_EXPORT void Cartridge_triggerIRQ(void *instance);
API_EXPORT void Cartridge_clearIRQ(void *instance);
