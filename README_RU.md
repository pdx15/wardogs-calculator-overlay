[![GitHub Release](https://img.shields.io/github/release/pdx15/wardogs-calculator-overlay)](https://github.com/pdx15/wardogs-calculator-overlay/releases)
[![Github All Releases](https://img.shields.io/github/downloads/pdx15/wardogs-calculator-overlay/total.svg)](https://github.com/pdx15/wardogs-calculator-overlay/releases)
[![ViewCount](https://views.whatilearened.today/views/github/pdx15/wardogs-calculator-overlay.svg)](https://github.com/pdx15/wardogs-calculator-overlay)
[![Donations Page](https://github.com/andry81-cache/gh-content-static-cache/raw/master/common/badges/donate/donate.svg)](https://github.com/pdx15/wardogs-calculator-overlay#%D0%B4%D0%BE%D0%BD%D0%B0%D1%82donate)

[English](README.md) | **Русский**

# WARDOGS Калькулятор артиллерии — Оверлей

Лёгкий C++ оверлей для артиллерийских расчётов в WARDOGS.
Отображает MIL, азимут и расстояние с полной коррекцией по высоте рельефа.

Скачать для Windows: [Releases](https://github.com/pdx15/wardogs-calculator-overlay/releases)

### Интерфейс

<table>
  <tr>
    <th width="72%">В игре</th>
    <th width="28%">Интерфейс</th>
  </tr>
  <tr>
    <td align="center">
      <img src="assets/prev.png">
    </td>
    <td align="center">
      <img src="assets/ui.png">
    </td>
  </tr>
</table>

## Возможности

- **Баллистические таблицы** — L81 (миномёт) и SPH-2 (низкая + высокая дуга)
- **Коррекция рельефа (RK4)** — численный физический солвер, совпадающий с Apollyon
- **Коррекция курса корпуса** — модель платформы SPH-2 (крен 5°)
- **Сохранение/загрузка целей** — закладки целей по каждой карте
- **Поверх всех окон** — системный трей для показа/скрытия и выхода
- **Иконка в трее** — ЛКМ переключает оверлей, ПКМ — меню

## Сборка

Запустите `build.bat` и выберите компилятор в меню. CMake, SDL3 и ImGui скачиваются и собираются автоматически — никаких ручных зависимостей.

### Данные рельефа

Для коррекции по высоте рельефа скачайте и поместите папку `wardogs-data` рядом с `calculator.exe`:

```
wardogs-data/
├── data/maps/
│   ├── bakurani/terrain.json
│   ├── ozeti/terrain.json
│   └── zestafona/terrain.json
└── assets/maps/
    ├── kavkazi/heightmap.png
    ├── europe/heightmap.png
    └── north-america/heightmap.png
```

Скачать [wardogs-data](https://drive.google.com/file/d/1vN_mXTHkiWVPnv3evhWjevvLWi_dmDWB/view?usp=sharing)

Без `wardogs-data` калькулятор всё равно работает — MIL рассчитывается по баллистическим таблицам, но без коррекции по высоте рельефа (Delta Z = 0).

### Загрузка рельефа

Если `wardogs-data` лежит рядом с `calculator.exe`, рельеф загружается автоматически при запуске. Иначе — укажите путь вручную и нажмите **Load**.

## Использование

1. Выберите орудие и карту
2. Введите координаты артиллерии (точка стрельбы) и цели (в игровых единицах, допускаются десятичные)
3. MIL рассчитывается автоматически при изменении координат
4. **Save** — сохранить текущую цель с именем (по карте)
5. **Load** — выбрать ранее сохранённую цель
6. Для SPH-2: включите **Hull heading** и задайте курс машины для коррекции платформы

## Трей

| Действие | Результат |
|----------|-----------|
| F5 (по умолчанию) | Показать / скрыть оверлей |
| ЛКМ по иконке в трее | Показать / скрыть |
| ПКМ по иконке в трее | Выход |

Горячую клавишу можно переназначить в панели Settings — нажмите на кнопку текущей клавиши и нажмите любую клавишу. Esc — отключить.

Окно перемещается перетаскиванием за заголовок.

## Донат/Donate

Если вы хотите оставить чаевые, то можно это сделать по ссылке ниже или через QR-код:

[![Поддержать через CloudTips](assets/qrCode.png)](https://pay.cloudtips.ru/p/1569852d)

**Оставить чаевые через CloudTips:** https://pay.cloudtips.ru/p/1569852d

| Криптовалюта | Сеть | Кошелёк |
|--------------|------|---------|
| **USDT**     | TRC20 | `TGm951N5TpuBKayeAP6vL8wAdB9vEvdMyo` |
| **USDT**     | ERC20 | `0xd938130bce0C9A6a8F83dDe4fEAF6A86847c148B` |
| **BTC**      |    | `c1qhg00wd03fnhdpwhg8fa4a3tpshhmsf0uslw8nf` |

---