# DB Statistics Calculator

Программа вычисляет статистические параметры для заданной колонки в таблице SQLite.

## Сборка в MSYS2

Установите SQLite3:

```bash
pacman -S mingw-w64-ucrt-x86_64-sqlite3
```
Сборка:

```bash
make
```
Использование:
```bash
./stats <database> <table> <column>
```
## Пример
Создание тестовой БД:

```bash
sqlite3 test.db
CREATE TABLE test (value INTEGER);
INSERT INTO test VALUES (10), (20), (30), (40), (50);
.quit
```

## Запуск:
```bash
./stats test.db test value
```

## Вывод:
text
Statistics for column 'value' in table 'test':
  Count (numeric): 5
  Sum: 150.000000
  Mean: 30.000000
  Min: 10.000000
  Max: 50.000000
  Variance: 250.000000
  
## Обработка ошибок
 - Несуществующая БД → сообщение об ошибке
 - Несуществующая таблица → сообщение
 - Несуществующая колонка → сообщение

Нечисловые значения → игнорируются с предупреждением
