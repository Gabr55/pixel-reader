# Build for Miyoo Mini Plus / OnionOS

Короткая инструкция: как собрать zip, который можно положить на SD-карту OnionOS.

## Один раз

1. Установи и запусти Docker Desktop.
2. В корне проекта выполни:

```sh
git submodule update --init --recursive
```

## Собрать OnionOS zip

Из корня проекта:

```sh
./scripts/build_miyoo_onion.sh 0.1
```

Где `0.1` - номер версии. Можно поставить любой, например `0.2`.

После успешной сборки появится файл:

```text
build/pixel_reader_onion_v0.1.zip
```

## Собрать из VS Code

1. Открой папку `/Users/alex/code/pixel-reader` в VS Code.
2. Открой `Run and Debug`.
3. Выбери `Build Miyoo OnionOS package`.
4. Нажми старт.
5. Введи версию, например `0.1`.

Готовый zip появится в `build/`.

## Совсем коротко: что тыкать

```sh
cd /Users/alex/code/pixel-reader
git submodule update --init --recursive
./scripts/build_miyoo_onion.sh 0.1
```

Потом взять:

```text
/Users/alex/code/pixel-reader/build/pixel_reader_onion_v0.1.zip
```

И распаковать в корень SD-карты OnionOS.

## Установить на Miyoo

1. Вставь SD-карту OnionOS в компьютер.
2. Скопируй `build/pixel_reader_onion_v0.1.zip` в корень SD-карты.
3. Распакуй zip прямо в корень SD-карты.
4. Безопасно извлеки SD-карту.
5. Вставь SD-карту в Miyoo Mini Plus.
6. Запусти `Apps -> Ebook Reader`.

## Где смотреть лог

После запуска на устройстве лог лежит здесь:

```text
App/PixelReader/log.txt
```

Если приложение не стартует, сначала смотри этот файл.

## Пересобрать заново

```sh
./scripts/build_miyoo_onion.sh 0.2
```

Если Docker/toolchain сломался:

```sh
make -C cross-compile/miyoo-mini/union-miyoomini-toolchain clean
./scripts/build_miyoo_onion.sh 0.2
```
