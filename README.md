# Simple Chat

## 1 Simple Messanger

Вам необходимо разработать систему обмена мгновенными сообщениями
(chat, messanger)

## 1.1 Server

Сервер принимает сообщения от клиентов и рассылает их другим подключенным клиентам. Использует потоки для работы с клиентами. Имя бинарного файла должно быть server. Сервер должен иметь консольный интерфейс в параметры которого передается: port

## 1.2 Client

Каждый клиент должен иметь свой уникальный nickname установленный
пользователем. При получении сообщения от другого клиент, на экране
должно отображаться время получения сообщения, nickname отправителя,
и текст. Например {05:20} [John] Hi!
Клиент должен быть выполнен в виде консольного интерфейса. Название бинарного файла client. Клиентская утилита получает аргументы командной строоки в следующем порядке: server address, port, nickname
Для предотвращения получения входящих сообщения когда пользователь вводи сообщения для отправки предлагается ввести раздельный режим для отправки и получения сообщений. Когда клавиша m нажата, то
пользователь вводит сообщение и новые соообщения от других клиентов не
отображаются. После отправки сообщения (by Enter) режим ввода

## 1.3 Что не требуется в работе

- Регистрация, авторизация, аутентификция клиента
- Больше чем один канал для общения
- Держать собственную история
- Обрабатывать временные зоны
- Работа с языки отличающимися от Английского

## 1.4 Требования

- Сервер и клиент должны быть написаны на языке C
- В качестве билдовой системы нужно использовать Make
- Код должен компилироваться и успешно работать для Linux и MacOS
- Valgrind и Google Thread Sanitizer должны не находить ошибок
