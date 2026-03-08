# Компиляция под Windows

## Зависимости
* GCC v6+ (для поддержки C++ 17)
* CMake 3.22+
* LuaJIT 2.1
* Qt6

Для установки вышеперечисленных зависимостей вам понадобится:
* Установить [MSYS2](https://www.msys2.org/)
* Запустить в MSYS2 **UCRT64** и выполнить команду:
  ``` bash
    pacman -S --needed base-devel \
    mingw-w64-ucrt-x86_64-gcc \
    mingw-w64-ucrt-x86_64-luajit \
    mingw-w64-ucrt-x86_64-cmake \
    mingw-w64-ucrt-x86_64-qt6-base \
    mingw-w64-ucrt-x86_64-qt6-multimedia
  ```
* Установить пакет [Qt6](https://www.qt.io/development/download)

## Сборка проекта
Скопируйте репозиторий
``` bash
git clone https://github.com/Noxikly/NES.cpp
cd NES.cpp
```

Затем скомпилируйте промежуточную версию
``` bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

Откройте терминал (могут понадобиться права администратора) и выполните следующие команды:
```
cd "C:\Qt\<ваша версия Qt>\mingw_64\bin"
windeploy.exe --no-translations <путь к .exe> --dir <путь до конечной директории>
```
Затем переместите nespp.exe файл из папки `build` в вашу конечную директорию
