# NES.cpp

NES.cpp - простой и легковесный эмулятор с поддержкой модульности мапперов

## Скриншоты
<img src="img/kirby.png" alt="kirby" width="256" height="240">
<img src="img/smb3.png" alt="smb3" width="256" height="240">
<img src="img/dk.png" alt="dk" width="256" height="240">

## Сборка
Зависимости для сборки:
* LuaJit
* gcc

**Linux/MacOS:**
```bash
git clone https://github.com/Noxikly/NES.cpp
cd NES.cpp
make -j
```

**Windows:**
Для начала установите MSYS2 и соответствующие зависимости для проекта
```bash
git clone https://github.com/Noxikly/NES.cpp
cd NES.cpp
make nes_win -j
```