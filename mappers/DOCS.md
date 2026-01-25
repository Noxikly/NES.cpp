# Документация по созданию мапперов

## Теория

Для того, чтобы писать/читать ту или иную информацию, желательно знать адрес, к которому стоит обращаться. Код из Lua занимается именно этим: все основные функции read/write **возвращают адрес**, который позже будет использован уже для самой операции.

Всего существует 6 основных функций:
* **init()** - инициализирует значения (выполняется всего 1 раз)
* **readPRGAddress(addr)** - получает PRG адрес для последующего чтения (readPRG)
* **readCHRAddress(addr)** - получает CHR адрес для последующего чтения (readCHR)
* **writePRGAddress(addr, value)** - получает PRG адрес для последующей записи (writePRG)
* **writeCHRAddress(addr, value)** - получает CHR адрес для последующей записи (writeCHR)
* **step()** - в основном нужен для счетчиков irq (и других)

Функции (кроме init) могут возвращать либо адрес(u32) либо nil (в таком случае read/write функция ничего не будет делать).

Так как код пишется на Lua, то вы можете создавать свои функции и импортировать свои библиотеки; самое главное, чтобы конечный адрес возвращался из вышеперечисленных функций.


## Псевдокод

```lua
local mp0 = {} -- Класс с функциями

function mp0:init()
    -- код --
end

function mp0:readPRGAddr(addr)
    -- код --
    return addr
end

function mp0:writePRGAddr(addr, value)
    -- код --
    return nil
end

function mp0:readCHRAddr(addr)
    -- код --
    return addr
end

function mp0:writeCHRAddr(addr, value)
    -- код --
    return nil
end

return mp0 -- Обязательно вернуть
```



## Библиотека lib.lua
Для обращения к C++ функциям из Lua как раз используется именно эта библиотека. Она содержит только основные функции для мапперов, на основе которых можно строить свою логику.


### Константы зеркалирования

| Имя | Значение | Описание |
|-----|----------|----------|
| MIRROR_HORIZONTAL | 0 | Горизонтальное зеркало (A A/B B) |
| MIRROR_VERTICAL | 1 | Вертикальное зеркало (A B/A B) |
| MIRROR_SINGLE_SCREEN_A | 2 | Только экран A |
| MIRROR_SINGLE_SCREEN_B | 3 | Только экран B |
| MIRROR_FOUR_SCREEN | 4 | Четырёхэкранный режим |


### Константы типов векторов
| Имя | Назначение |
|-----|------------|
| VEC_PRG_ROM | 0 – доступ к PRG-ROM |
| VEC_PRG_RAM | 1 – доступ к PRG-RAM |
| VEC_CHR_ROM | 2 – доступ к CHR-ROM |


### Функции
| Название | Что делает? | Пример |
| -------- | ----------- | ------ |
| resizePRG(size) | Изменяет размер PRG-ROM (в байтах) | `lib.resizePRG(32768)` |
| resizePRG_RAM(size) | Изменяет размер PRG-RAM (в байтах) | `lib.resizePRG_RAM(8192)` |
| resizeCHR(size) | Изменяет размер CHR-ROM (в байтах) | `lib.resizeCHR(8192)` |
| setMirror(mode) | Устанавливает режим зеркалирования экрана | `lib.setMirror(lib.MIRROR_VERTICAL)` |
| getMirror() | Возвращает текущий режим зеркалирования | `local m = lib.getMirror()` |
| triggerIRQ() | Поднимает флаг IRQ (irqFlag = true) | `lib.triggerIRQ()` |
| clearIRQ() | Сбрасывает флаг IRQ (irqFlag = false) | `lib.clearIRQ()` |
| getIRQ() | Возвращает состояние флага IRQ | `if lib.getIRQ() then ... end` |
| readPRGDirect(addr) | Читает 1 байт из PRG-ROM по адресу | `local b = lib.readPRGDirect(0xC000)` |
| writePRGDirect(addr, value) | Пишет 1 байт в PRG-ROM по адресу | `lib.writePRGDirect(0xC000, 0xA9)` |
| readCHRDirect(addr) | Читает 1 байт из CHR-ROM по адресу | `local tile = lib.readCHRDirect(0x0000)` |
| writeCHRDirect(addr, value) | Пишет 1 байт в CHR-ROM по адресу | `lib.writeCHRDirect(0x0000, 0xFF)` |
| readRAMDirect(addr) | Читает 1 байт из PRG-RAM (addr & 0x1FFF) | `local save = lib.readRAMDirect(0x6000)` |
| writeRAMDirect(addr, value) | Пишет 1 байт в PRG-RAM (addr & 0x1FFF) | `lib.writeRAMDirect(0x6000, 0x55)` |
| getBit(value, bit) | Возвращает значение бита (0/1) | `local bit0 = lib.getBit(0x83, 0)` → 1 |
| setBit(value, bit) | Устанавливает бит в 1 | `lib.setBit(flags, 3)` |
| clearBit(value, bit) | Сбрасывает бит в 0 | `lib.clearBit(flags, 3)` |
| toggleBit(value, bit) | Инвертирует бит | `lib.toggleBit(flags, 3)` |
| testBit(value, bit) | Проверяет, установлен ли бит | `if lib.testBit(flags, 3) then ... end` |