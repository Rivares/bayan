#include <gperftools/profiler.h>

#include <iostream>
#include <memory>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "lib.hpp"

namespace prog_opt = boost::program_options;
using namespace boost::filesystem;

// Builder pattern
class Settings
{
    std::vector<std::string> m_pathsForScan;
    std::vector<std::string> m_pathsForUnScan;
    size_t m_depthScan;
    size_t m_minimalSizeOfFile;
    std::string m_maskForScan;
    size_t m_sizeOfBlock;
    std::string m_hashAlg;

public:
    Settings() :
        m_pathsForScan()
        , m_pathsForUnScan()
        , m_depthScan(0)
        , m_minimalSizeOfFile(1)
        , m_maskForScan("*")
        , m_sizeOfBlock(1)
        , m_hashAlg("md5")
    {};
    ~Settings() = default;

    friend class SettingsBuilder;

    std::vector<std::string> getPathsForScan() const
    {   return m_pathsForScan;  }

    std::vector<std::string> getPathsForUnScan() const
    {   return m_pathsForUnScan;  }

    size_t getDepthScan() const
    {   return m_depthScan;  }

    size_t getMinimalSizeOfFile() const
    {   return m_minimalSizeOfFile;  }

    std::string getMaskForScan() const
    {   return m_maskForScan;  }

    size_t getSizeOfBlock() const
    {   return m_sizeOfBlock;  }

    std::string getHashAlg() const
    {   return m_hashAlg;  }
};

class SettingsBuilder
{
    Settings m_settings;

public:
    SettingsBuilder() = default;
    ~SettingsBuilder() = default;

    SettingsBuilder& withPathScan(const std::vector<std::string>& pathsForScan)
    {
        m_settings.m_pathsForScan = pathsForScan;
        return *this;
    }

    SettingsBuilder& withPathUnScan(const std::vector<std::string>& pathsForUnScan)
    {
        m_settings.m_pathsForUnScan = pathsForUnScan;
        return *this;
    }

    SettingsBuilder& withDepthScan(const size_t& depthScan)
    {
        m_settings.m_depthScan = depthScan;
        return *this;
    }

    SettingsBuilder& withMinimalSizeOfFile(const size_t& minimalSizeOfFile)
    {
        m_settings.m_minimalSizeOfFile = minimalSizeOfFile;
        return *this;
    }

    SettingsBuilder& withMaskForScan(const std::string& maskForScan)
    {
        m_settings.m_maskForScan = maskForScan;
        return *this;
    }

    SettingsBuilder& withSizeOfBlock(const size_t& sizeOfBlock)
    {
        m_settings.m_sizeOfBlock = sizeOfBlock;
        return *this;
    }

    SettingsBuilder& withHashAlg(const std::string& hashAlg)
    {
        m_settings.m_hashAlg = hashAlg;
        return *this;
    }

    Settings& build()
    {
        return m_settings;
    }
};

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

        // bayan --sc="path" "path" --unsc="path" "path" --dpth=2 --msf=5 --mask="*" --sb=20 --hash=md5
        prog_opt::variables_map vm;
        prog_opt::store(parse_command_line(argc, argv, desc), vm);
        prog_opt::notify(vm);

        if (vm.count("sc"))
        {
            std::cout << vm["sc"].as<std::vector<std::string>>().size() << '\n';
            optionsBuilder.withPathScan(vm["sc"].as<std::vector<std::string>>());
        }
        if (vm.count("unsc"))
        {
            std::cout << vm["unsc"].as<std::vector<std::string>>().size() << '\n';
            optionsBuilder.withPathUnScan(vm["unsc"].as<std::vector<std::string>>());
        }
        if (vm.count("dpth"))
        {
            std::cout << vm["dpth"].as<size_t>() << '\n';
            optionsBuilder.withDepthScan(vm["dpth"].as<size_t>());
        }
        if (vm.count("msf"))
        {
            std::cout << vm["msf"].as<size_t>() << '\n';
            optionsBuilder.withMinimalSizeOfFile(vm["msf"].as<size_t>());
        }
        if (vm.count("mask"))
        {
            std::cout << vm["mask"].as<std::string>() << '\n';
            optionsBuilder.withMaskForScan(vm["mask"].as<std::string>());
        }
        if (vm.count("sb"))
        {
            std::cout << vm["sb"].as<size_t>() << '\n';
            optionsBuilder.withSizeOfBlock(vm["sb"].as<size_t>());
        }
        if (vm.count("hash"))
        {
            std::cout << vm["hash"].as<std::string>() << '\n';
            optionsBuilder.withHashAlg(vm["hash"].as<std::string>());
        }


        Settings options = optionsBuilder.build();

        path currPath("."); // sc
        boost::system::error_code error;

        const boost::interprocess::mode_t mode = boost::interprocess::read_only;
//        boost::interprocess::file_mapping fm(filename, mode);

//        boost::interprocess::mapped_region region(fm, mode, 0, 0);

//        const char* begin = static_cast<const char*>(

//            region.get_address()
//        );

//        const char* pos = std::find(
//            begin, begin + region.get_size(), '\1'
//        );

        directory_iterator end_itr;

        // cycle through the directory
        for (directory_iterator itr(currPath); itr != end_itr; ++itr)
        {
            // If it's not a directory, list it. If you want to list directories too, just remove this check.
            if (is_regular_file(itr->path())) {
                // assign current file name to current_file and echo it out to the console.
                std::string current_file = itr->path().string();
                std::cout << current_file << std::endl;
            }
        }

        try
        {
            if (exists(currPath))
            {
                if (is_regular_file(currPath))
                    std::cout << currPath << " size is " << file_size(currPath) << '\n';

                  else if (is_directory(currPath))
                  {
                    std::cout << currPath << " is a directory containing:\n";

                    for (directory_entry& x : directory_iterator(currPath))
                      std::cout << "    " << x.path() << '\n';
                  }
                  else
                    std::cout << currPath << " exists, but is not a regular file or directory\n";
            }
            else
                  std::cout << currPath << " does not exist\n";
        }
        catch (const filesystem_error& ex)
        {
            std::cout << ex.what() << '\n';
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


