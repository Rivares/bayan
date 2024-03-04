#include <gperftools/profiler.h>

#include <condition_variable>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <list>



#include "lib.hpp"


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
    */

    /*!
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

        SettingsBuilder optionsBuilder;

        prog_opt::options_description desc{"Options"};
        desc.add_options()
                ("sc",  prog_opt::value<std::vector<std::string>>()->multitoken(),  "directories for scan (\"path\", ..., \"path\")")
                ("unsc",prog_opt::value<std::vector<std::string>>()->multitoken(),  "directories for unscan (\"path\", ..., \"path\")")
                ("dpth",prog_opt::value<size_t>()->default_value(0),                "depth in tree of directories for scan (0..)")
                ("msf", prog_opt::value<size_t>()->default_value(1),                "minimal size of file (1..)")
                ("mask",prog_opt::value<std::string>()->default_value("*"),         "mask of file for scan (regexp)")
                ("sb",  prog_opt::value<size_t>()->default_value(1),                "size of block in file (bytes)")
                ("hash",prog_opt::value<std::string>()->default_value("md5"),       "algorithm of hash (md5, crc32)")
                ;

        ///    Пример запуска этой утилиты
        // bayan --sc="path" "path" --unsc="path" "path" --dpth=2 --msf=5 --mask="*" --sb=20 --hash=md5
        // bayan --sc="./" "/home/user/0_projects" --unsc="/home/user/0_projects/Arduino/libraries/ArduinoRS485"  "/home/ermolov/0_projects/Arduino/libraries/Modbus-Master-Slave-for-Arduino-master" --dpth=4 --msf=5 --mask=".*\.(cpp|h)" --sb=20 --hash=sha1

        prog_opt::variables_map vm;
        prog_opt::store(parse_command_line(argc, argv, desc), vm);
        prog_opt::notify(vm);

        if (vm.count("sc"))
        {
            optionsBuilder.withPathScan(vm["sc"].as<std::vector<std::string>>());
        }
        if (vm.count("unsc"))
        {
            optionsBuilder.withPathUnScan(vm["unsc"].as<std::vector<std::string>>());
        }
        if (vm.count("dpth"))
        {
            optionsBuilder.withDepthScan(vm["dpth"].as<size_t>());
        }
        if (vm.count("msf"))
        {
            optionsBuilder.withMinimalSizeOfFile(vm["msf"].as<size_t>());
        }
        if (vm.count("mask"))
        {
            optionsBuilder.withMaskForScan(vm["mask"].as<std::string>());
        }
        if (vm.count("sb"))
        {
            optionsBuilder.withSizeOfBlock(vm["sb"].as<size_t>());
        }
        if (vm.count("hash"))
        {
            optionsBuilder.withHashAlg(vm["hash"].as<std::string>());
        }


        Settings options = optionsBuilder.build();


        size_t curDepthScan = 0;
        const auto& listUnScan = options.getPathsForUnScan();

        ///    1. Исколючение из поиска путей, которые не нужно сканировать (m_pathsForUnScan)
        for (path currPath: options.getPathsForScan())
        {
            currPath = (currPath.is_relative())? current_path() : currPath;


            // Исключение из проверки
            bool isFoundFullUnScanPath = false;
            std::vector<path> foundPartialUnScanPath(listUnScan.size());
            for (const auto& item : listUnScan)
            {
//                std::cout << "1@ " << currPath << '\t';
//                std::cout << "2@ " << item << '\n';
                if ((currPath.compare(item) == 0))
                {
//                    std::cout << "Full@ " << item << '\n';
                    isFoundFullUnScanPath = true;   // Full match
                    break;
                }

                if ((currPath.compare(item) == -1))
                {
//                    std::cout << "Part@" << item << '\n';
                    foundPartialUnScanPath.push_back(item);  // Partial match
                }
            }

            if (!isFoundFullUnScanPath)
            {
                if (!foundPartialUnScanPath.empty())
                {
//                    std::cout << "Part " << currPath.string() << '\n';

                    outputFiles(options, currPath, curDepthScan, foundPartialUnScanPath);
                }
                else
                {
//                    std::cout << "|878_" << currPath.string() << '\n';
                    outputFiles(options, currPath, curDepthScan, foundPartialUnScanPath);
                }
            }
            else
            {
                // Исключение path из проверки
//                std::cout << "Full " << currPath.string() << '\n';
                continue;
            }
        }



        ///    2 б). Исколючение из поиска файлов, которые "уникальны" по размеру
        for (auto itMap = doubleFiles.begin(); itMap != doubleFiles.end();)
        {
            if (doubleFiles.count(itMap->first) == 1)
            {   itMap = doubleFiles.erase(itMap);    }
            else
            {
                auto bucket = doubleFiles.bucket(itMap->first);
                auto itr = doubleFiles.begin(bucket);
                auto referenceSize = file_size((*itr).second);
                auto blockSize = options.getSizeOfBlock();


                std::list<DataFile> openedFiles;
                for (auto it = doubleFiles.begin(bucket); it != doubleFiles.end(bucket); ++it)
                {
                    if (referenceSize == file_size((*it).second))
                    {
                        openedFiles.emplace_back((*it).second.string(), blockSize);
                    }
                }

std::cout << '\n';
std::cout << '\n';
std::cout << '\n';

                size_t cntProcessedFiles = 0;
                while(cntProcessedFiles != openedFiles.size())
                {
                    for (auto& item: openedFiles)
                    {
                        auto restReadSize = item.fileSize - item.fd.tellg();
                        if (restReadSize > 0)
                        {
                            ///    3. Формирование необходимого  размера блока данных
                            auto currBlockSize = ((restReadSize / item.blockSize)  >= 1.0) ? item.blockSize : restReadSize;
                            std::unique_ptr<char[]> readBlock = std::make_unique<char[]>(item.blockSize);

                            ///    4. Чтение блока данных из преодполагаемых файлов-дубликатов
                            item.fd.read(readBlock.get(), currBlockSize);

                            if (!item.fd.fail())
                            {
//                                std::cout << std::string(readBlock.get(), currBlockSize) << '\n';

                                ///    5. Получение хэша блока данных выбранным методом
                                if (options.getHashAlg() == "sha1")
                                {
                                    item.hashFile += getHash<sha1, sha1::digest_type>(readBlock.get(), currBlockSize);
                                }
                                else
                                {
                                    item.hashFile += getHash<md5, md5::digest_type>(readBlock.get(), currBlockSize);
                                }
                            }
                        }
                        else
                        {
                            ++cntProcessedFiles;
                            ++itMap;

                            if (item.fd.is_open())
                            {   item.fd.close();    }

                             continue;
                        }
                    }

                    ///    6. Немедленное прекращение чтения файла, который оказался уникальным
                    std::queue<decltype(openedFiles.begin())> removeItems;
                    for (auto refIt = openedFiles.begin(); refIt != openedFiles.end(); ++refIt)
                    {
                        const std::string& referenceStr = (*refIt).hashFile;
                        auto otherIt = openedFiles.begin();
                        for (; otherIt != openedFiles.end(); ++otherIt)
                        {
                            if (((*otherIt).hashFile == referenceStr) && (otherIt != refIt))
                            {   break;  }
                        }

                        if (otherIt == openedFiles.end())
                        {
//                            std::cout << "Erase()! " << (*refIt).pathToFile << '\t';

                            if ((*refIt).fd.is_open())
                            {   (*refIt).fd.close();   }

                            ++itMap;

                            removeItems.push(refIt);
                        }
                    }

                    while(removeItems.size() != 0)
                    {
                        openedFiles.erase(removeItems.front());
                        removeItems.pop();
                    }
                }

                ///    7. Вывод настоящих файлов-дубликатов
                std::cout << "Doubles:\n";
                for (auto& item: openedFiles)
                {
                    std::cout << item.pathToFile << '\n';
                }
            }
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

