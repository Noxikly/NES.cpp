# Компиляция под UNIX системы

## Зависимости
* GCC v6+ (для поддержки C++ 17)
* CMake 3.22+
* LuaJIT 2.1
* Qt6


Все вышеперечисленные зависимости могут быть установленны одной командой в зависимости от дистрибутива:
* Arch/Manjaro:
`sudo pacman -Syu gcc cmake luajit qt6-{base,multimedia}`

* Ubuntu/Debian:
`sudo apt install luajit cmake gcc g++ qt6-base-dev`
* Fedora:
`sudo dnf install luajit cmake gcc gcc-c++ qt6-qtbase-devel`

## Компиляция
Скопируйте репозиторий
``` bash
git clone https://github.com/Noxikly/NES.cpp
cd NES.cpp
```

Затем скомпилируйте Release версию (Или Debug, если это необходимо)
``` bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```