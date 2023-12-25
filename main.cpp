#include <gperftools/profiler.h>

#include <iostream>
#include <memory>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "lib.hpp"

namespace prog_opt = boost::program_options;

int main(int argc, const char* argv[])
{
    ProfilerStart("bayan.prof");

///    Условие
///    Пользуясь имеющимися в библиотеке Boost структурами и алгоритмами
///    разработать утилиту для обнаружения файлов-дубликатов.

    /*!
        Утилита должна иметь возможность через параметры командной строки
        указывать:
        • директории для сканирования (может быть несколько)
        • директории для исключения из сканирования (может быть несколько)
        • уровень сканирования (один на все директории, 0 - только указанная
        директория без вложенных)
        • минимальный размер файла, по умолчанию проверяются все файлы
        больше 1 байта.
        • маски имен файлов разрешенных для сравнения (не зависят от
        регистра)
        • размер блока, которым производится чтения файлов, в задании этот
        размер упоминается как S
        • один из имеющихся алгоритмов хэширования (crc32, md5 -
        конкретные варианты определить самостоятельно), в задании
        эта функция упоминается как H

        Результатом работы утилиты должен быть список полных путей файлов
        с идентичным содержимым, выводимый на стандартный вывод. На одной
        строке один файл. Идентичные файлы должны подряд, одной группой.
        Разные группы разделяются пустой строкой.
        Обязательно свойство утилиты - бережное обращение с дисковым вводом
        выводом. Каждый файл может быть представлен в виде списка блоков
        размера S. Если размер файла не кратен, он дополняется бинарными
        нулями.
    */

    /*!
        Файл world.txt из одной строки
        Hello, World\n

        При размере блока в 5 байт, будет представлен как
        Hello
        , Wor
        ld\n\0\0

        Каждый блок должен быть свернут выбранной функцией хэширования.
        Возможные коллизии игнорируются. Из предположения, что
        H("Hello") == A
        H(", Wor") == B
        H("ld\n\0\0") == C

        Наш файл world.txt может быть представлен в виде последовательности
        ABC

        Рассмотрим второй файл cpp.txt
        Hello, C++\n

        Который после хэширования блоков
        H("Hello") == A
        H(", C++") == D
        H("\n\0\0\0\0") == E

        может быть представлен в виде последовательности ADE
        Порядок сравнения этих файлов должен быть максимально бережным. То
        есть обработка первого файла world.txt вообще не приводит к чтению с
        диска, ведь нам еще не с чем сравнивать. Как только мы добираемся до
        файла cpp.txt только в этот момент происходит перое чтение первого блока
        обоих файлов. В данном случае блоки идентичны, и необходимо прочесть
        вторые блоки, которые уже различаются. Файлы различны, оставшиеся
        данные не читаются.
        Файлы считаются идентичными при полном совпадении последовательности
        хешей блоков.
    */

    /*!
        Самоконтроль

        • блок файла читается с диска не более одного раза
        • блок файла читается только в случае необходимости
        • не забыть, что дубликатов может быть больше чем два
        • пакет bayan содержащий исполняемый файл bayan опубликован
        • описание параметров в файле README.md корне репозитория
        • отправлена на проверку ссылка на страницу репозитория
    */

    /*!
        Проверка

        Задание считается выполнено успешно, если после просмотра кода,
        подключения репозитория, установки пакета и запуска бинарного файла
        командой (параметры из описания):

        $ bayan [...]

        будут обнаружены файлы-дубликаты, без ложных срабатываний и
        пропуска существующих дубликатов.
        Количество прочитанных данных с диска минимально.
    */

    /*!
        Collecting args
    */

    try
    {
        std::cout << "Constructions project of objects:\n\n";


        prog_opt::options_description desc{"Options"};
        desc.add_options()
                ("sc",  prog_opt::value<std::vector<std::string>>(),            "directories for scan (\"path\", ..., \"path\")")
                ("unsc",prog_opt::value<std::vector<std::string>>(),            "directories for unscan (\"path\", ..., \"path\")")
                ("l",   prog_opt::value<size_t>()->default_value(0),            "level in tree of directories for scan (0..)")
                ("msf", prog_opt::value<size_t>()->default_value(1),            "minimal size of file (1..)")
                ("mask",prog_opt::value<std::string>()->default_value("*"),     "mask of file for scan (regexp)")
                ("sb",  prog_opt::value<size_t>()->default_value(1),            "size of block in file (bytes)")
                ("hash",prog_opt::value<std::string>()->default_value("md5"),   "algorithm of hash (md5, crc32)")
                ;

        prog_opt::variables_map vm;
        prog_opt::store(parse_command_line(argc, argv, desc), vm);
        prog_opt::notify(vm);

        if (vm.count("sc"))
        {
            std::cout << vm["sc"].as<std::vector<std::string>>().size() << '\n';
        }
        else if (vm.count("unsc"))
        {
            std::cout << vm["unsc"].as<std::vector<std::string>>().size() << '\n';
        }
        else if (vm.count("l"))
        {
            std::cout << vm["l"].as<size_t>() << '\n';
        }
        else if (vm.count("msf"))
        {
            std::cout << vm["msf"].as<size_t>() << '\n';
        }
        else if (vm.count("mask"))
        {
            std::cout << vm["mask"].as<std::string>() << '\n';
        }
        else if (vm.count("sb"))
        {
            std::cout << vm["sb"].as<size_t>() << '\n';
        }
        else if (vm.count("hash"))
        {
            std::cout << vm["hash"].as<std::string>() << '\n';
        }

        std::cout << "\n\nDestructions objects:\n";
    }
    catch (const std::exception& except)
    {
        std::cerr << except.what() << '\n';
    }


    std::cout << "\n";

    ProfilerStop();
    return 0;
}


