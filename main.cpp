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
#include <regex>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <boost/algorithm/hex.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <boost/uuid/detail/sha1.hpp>

#include "lib.hpp"

namespace prog_opt = boost::program_options;
using namespace boost::filesystem;
using boost::uuids::detail::md5;
using boost::uuids::detail::sha1;

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

void outputFiles(Settings& options, const path& currPath, size_t& curDepthScan, const std::vector<path>& unScanPath);
std::unordered_multimap</*uint64_t*/ boost::uintmax_t, path> doubleFiles;



template <typename HashType, typename HashType_DigestType>
class DataFile
{
private:
    size_t blockSize = 5; // initialization from constexpr variable
    HashType m_currHash;
    path m_currPath;

    std::mutex& m_mutex;
    std::condition_variable& m_cv;    
    bool                     m_notified;
    bool                     m_earlyRelease;

public:
    std::string hashFile;

    explicit DataFile(const path& path_, std::mutex& genMutex, std::condition_variable& genCV) noexcept
        : m_currPath(path_)
        , m_mutex(genMutex)
        , m_cv(genCV)
        , m_notified(false)
        , m_earlyRelease(false)
    {
        std::cout << m_currPath.string() << '\n';
    }
    ~DataFile() = default;


    void readBlockOfFile()
    {
        std::cout << "Read file. thread_id: " << std::this_thread::get_id() << '\n';

        // Read file
        std::ifstream fileStream(m_currPath.string(), std::ifstream::binary);

        if (!fileStream.is_open())
        {   std::cout << "Error open file!\n";   return;  }


        char* readBlock = new char[blockSize];

        {
            std::unique_lock<std::mutex> lock(m_mutex);

            while ((!fileStream.eof()) && (!m_earlyRelease))
            {
                fileStream.read(readBlock, blockSize);
                hashFile += getHash(readBlock);

                m_tasks.push(this);
                m_notified = true;
                m_cv.notify_one();
            }
        }

        delete [] readBlock;
        readBlock = nullptr;

        fileStream.close();


        hashFile.resize(20);
        std::cout << hashFile << '\n';
    }

    std::string getPath() const noexcept
    {   return m_currPath.string(); }

    std::string getHash(char* readBlock)
    {
        std::string result;
        HashType_DigestType digest;

        m_currHash.process_bytes(readBlock, blockSize);
        m_currHash.get_digest(digest);

        const auto charDigest = reinterpret_cast<const char*>(&digest);

        boost::algorithm::hex(charDigest, charDigest + sizeof(md5::digest_type), std::back_inserter(result));

        result.resize(5);

        return result;
    }


    void setEarlyRelease() noexcept
    {   m_earlyRelease = true;    }

};

template <typename HashType, typename HashType_DigestType>
class HashChecker
{
    private:

    uint64_t m_countTasks;

    std::mutex& m_mutex;
    std::condition_variable& m_cv;
    std::vector<DataFile<HashType, HashType_DigestType>*>   m_tasks;
    bool                     m_done;
    bool                     m_notified;

    public:
        explicit HashChecker(const uint64_t countTasks, std::mutex& genMutex, std::condition_variable& genCV) noexcept
            : m_countTasks(countTasks)
            , m_mutex(genMutex)
            , m_cv(genCV)
            , m_done(false)
            , m_notified(false)
        {}
        ~HashChecker() = default;

        void check()
        {
            while(!m_done)
            {
                std::unique_lock<std::mutex> locker(m_mutex);
                while(!m_notified) // от ложных пробуждений
                {   m_cv.wait(locker);  }

                // если есть ошибки в очереди, обрабатывать их
                if (m_tasks.size() == m_countTasks)
                {
                    std::cout << "Checking hashes!\n";
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                    m_tasks.at(0)->setEarlyRelease();

                    m_tasks.clear();
                }
                m_notified = false;
            }
        }

        void setDone() noexcept
        {   m_done = true;    }
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
                    std::cout << "Full@ " << item << '\n';
                    isFoundFullUnScanPath = true;   // Full match
                    break;
                }

                if ((currPath.compare(item) == -1))
                {
                    std::cout << "Part@" << item << '\n';
                    foundPartialUnScanPath.push_back(item);  // Partial match
                }
            }

            if (!isFoundFullUnScanPath)
            {
                if (!foundPartialUnScanPath.empty())
                {
                    std::cout << "Part " << currPath.string() << '\n';

                    outputFiles(options, currPath, curDepthScan, foundPartialUnScanPath);
                }
                else
                {
                    std::cout << "|878_" << currPath.string() << '\n';
                    outputFiles(options, currPath, curDepthScan, foundPartialUnScanPath);
                }
            }
            else
            {
                // Исключение path из проверки
                std::cout << "Full " << currPath.string() << '\n';
                continue;
            }

        }




        for (auto itMap = doubleFiles.begin(); itMap != doubleFiles.end();)
        {
            if (doubleFiles.count(itMap->first) == 1)
            {   itMap = doubleFiles.erase(itMap);    }
            else
            {
                auto bucket = doubleFiles.bucket(itMap->first);
                auto itr = doubleFiles.begin(bucket);
                auto referenceSize = file_size((*itr).second);

//                std::atomic<bool> genDataReady;
//                genDataReady.store(false);

                std::mutex genMutex;
                std::condition_variable genCV;
                HashChecker<md5, md5::digest_type> head{genMutex, genCV};

                std::unordered_multiset<std::shared_ptr<DataFile<md5, md5::digest_type>>> tasks;    // SFINAE
                std::vector<std::shared_ptr<std::thread>> threadPool(doubleFiles.bucket_size(bucket));//std::thread::hardware_concurrency());
                for (auto it = doubleFiles.begin(bucket); it != doubleFiles.end(bucket); ++it)
                {
                    if (referenceSize == file_size((*it).second))
                    {
                        tasks.insert(std::make_shared<DataFile<md5, md5::digest_type>>((*it).second, genMutex, genCV));
                    }
                }



                std::thread threadChecker(&HashChecker<md5, md5::digest_type>::check, head);

                for (std::shared_ptr<DataFile<md5, md5::digest_type>> item : tasks)
                {
                    threadPool.emplace_back(std::move(std::make_shared<std::thread>(&DataFile<md5, md5::digest_type>::readBlockOfFile, item.get()))); //std::transform algorithm
                    ++itMap;
                }


                for (auto& item : threadPool)
                {
                    if ( (item) && (threadChecker.joinable()) )
                    {   item->join();   }
                }

                if (threadChecker.joinable())
                {   threadChecker.join();   }

                head.setDone();
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


void outputFiles(Settings& options, const path& currGlobPath, size_t& curDepthScan, const std::vector<path>& unScanPath)
{
    size_t idxVecStr = 0;

    auto outputPath = [](const size_t replacedCount, const path& path_){
        std::string replacedStr(path_.string());
//        std::cout << "|_" << replacedStr.replace(0, replacedCount, replacedCount, '_') << '\n';
    };

    auto checkMatchPath = [unScanPath](const path& path_) -> bool {
        for(const auto& item : unScanPath)
        {
            if (path_.compare(item) == 0)
            {   return true;    }
        }
        return false;
    };

    ++curDepthScan;

    directory_iterator itrBeg(currGlobPath);
    directory_iterator itrEnd;
    path prevPath = currGlobPath;
    while((curDepthScan < options.getDepthScan()) || ((itrBeg == itrEnd)))
    {
        while (itrBeg != itrEnd)
        {
            path iterPath = itrBeg->path();

            if (is_regular_file(iterPath))
            {
                outputPath(currGlobPath.string().size(), iterPath);

                if (
                        (static_cast<size_t>(iterPath.size()) > options.getMinimalSizeOfFile()) &&
                        (std::regex_search(iterPath.string()
                                          , std::regex(options.getMaskForScan()
                                          , std::regex_constants::ECMAScript)))
                    )
                {
                    doubleFiles.insert({file_size(iterPath), iterPath});
                    std::cout << static_cast<uint64_t>(file_size(iterPath)) << "  " << iterPath.string() << '\n';
                }
            }
            else if (
                     (is_directory(iterPath)) &&
                     (curDepthScan < options.getDepthScan()) &&
                     (!checkMatchPath(iterPath))
                     )
            {
                prevPath = currGlobPath;

                outputPath(currGlobPath.string().size(), iterPath);

                outputFiles(options, iterPath, curDepthScan, unScanPath);
            }
            ++itrBeg;
        }
        break;
        // Отметка проверенных папок

    }
    --curDepthScan;

}

