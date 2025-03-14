# Улучшенный вектор 
Данный контейнер - аналог std::vector, улучшенная версия упрощенного вектора. Проведены доработки памятью.
## Возможности 
Функционал улучшенного вектора (Vector):
* При создании вектора можно указать: 
  * количество элементов вектора 
  * другой вектор (Vector) - lvalue или rvalue
* begin() - возвращает итератор на начало вектора 
* end() - возвращает итератор на элемент, следующий за последним 
* cbegin() - возвращает константный итератор на начало вектора 
* cend() - возвращает константный итератор на элемент, следующий за последним 
* Swap(...) - обменивает значение с другим вектором
* Reserve(...) - изменяет вместимость вектора (если вместимость вектора меньше нужной). Если элементы можно переместить, то перемещает, иначе копирует 
* Resize(...) - изменяет размер вектора. При увеличении размера новые элементы получают значение по умолчанию
* PopBack() - удаляет последний элемент вектора 
* EmplaceBack(...) - вставляет элемент в конец вектора. Принимает аргументы по forwarding-ссылке, используется вариативный шаблон. Если вместимость заполнена, то создает новый вектор с размером в 2 раза больше. Если элементы можно переместить, то перемещает, иначе копирует. Возвращает ссылку на добавленный элемент вектора. Строгая гарантия безопасности
* PushBack(...) - вставляет элемент в конец вектора. Использует метод EmplaceBack(...). Принимает lvalue и rvalue
* Insert(...) - вставляет элемент в заданную позицию вектора, возвращает итератор на вставленное значение. Использует метод Emplace(...)
* Emplace(...) - вставляет элемент в заданную позицию вектора. Для передачи своих параметров конструктору элемента использует perfect forwarding. допускает вставку элемента вектора внутрь того же самого вектора. Если вместимость заполнена, то создает новый вектор с размером в 2 раза больше. Если элементы можно переместить, то перемещает, иначе копирует. Возвращает итератор на вставленное значение. Строгая гарантия безопасности
* Erase(...) - удаляет элемент, на который указывает переданный итератор. Возвращает итератор, который ссылается на элемент, следующий за удалённым (если удаляется последний элемент вектора - возвращает end-итератор)
* Size() - вовращает количество элементов в векторе
* Capacity() - возвращает вместимость вектора
* operator[] - возвращает ссылку на элемент с нужным индексом 
## Установка и использование
* скопировать файл vector.h в папку с вашим проектом и импортировать как стороннюю библиотеку
