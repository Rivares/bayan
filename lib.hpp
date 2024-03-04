#include <gperftools/profiler.h>

#include <iostream>
#include <regex>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <boost/algorithm/hex.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <boost/uuid/detail/sha1.hpp>

namespace prog_opt = boost::program_options;
using namespace boost::filesystem;
using boost::uuids::detail::md5;
using boost::uuids::detail::sha1;


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

    /*!
        Функция void getPathsForScan()
        - получение путей до папок, которых нужно сканировать на поиск файлов-дубликатов
    */
    std::vector<std::string> getPathsForScan() const
    {   return m_pathsForScan;  }

    /*!
        Функция void getPathsForUnScan()
        - получение путей до папок, которых нужно исключить из сканирования на поиск файлов-дубликатов
    */
    std::vector<std::string> getPathsForUnScan() const
    {   return m_pathsForUnScan;  }

    /*!
        Функция void pushCommand(const std::string& command)
        - добавление новой комманды в очередь std::queue<std::string> commands
        и фиксация времени поступления первой комманды time_t timeFirstCommandOut
    */
    /*!
        Функция void getDepthScan()
        - получение глубины сканирования.
    */
    size_t getDepthScan() const
    {   return m_depthScan;  }

    /*!
        Функция void getDepthScan()
        - получение минмиального размера файла для сканирования.
    */
    size_t getMinimalSizeOfFile() const
    {   return m_minimalSizeOfFile;  }

    /*!
        Функция void getDepthScan()
        - получение маски имени файла для сканирования.
    */
    std::string getMaskForScan() const
    {   return m_maskForScan;  }

    /*!
        Функция void getDepthScan()
        - получение размера блока памяти файла при сканировании.
    */
    size_t getSizeOfBlock() const
    {   return m_sizeOfBlock;  }

    /*!
        Функция void getDepthScan()
        - получение выбранного метода получения хэша для прочитанного блока памяти файла.
    */
    std::string getHashAlg() const
    {   return m_hashAlg;  }
};

/// Use Builder pattern
///
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


std::unordered_multimap<boost::uintmax_t, path> doubleFiles;


void outputFiles(Settings& options, const path& currGlobPath, size_t& curDepthScan, const std::vector<path>& unScanPath)
{
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

                ///    2 а). Исколючение из поиска файлов, которые:
                ///    меньше минимального рамера
                ///    или
                ///    имя файла не удовлетворяет заданной маске
                if (
                        (static_cast<size_t>(iterPath.size()) > options.getMinimalSizeOfFile()) &&
                        (std::regex_search(iterPath.string()
                                          , std::regex(options.getMaskForScan()
                                          , std::regex_constants::ECMAScript)))
                    )
                {
                    doubleFiles.insert({file_size(iterPath), iterPath});
//                    std::cout << static_cast<uint64_t>(file_size(iterPath)) << "  " << iterPath.string() << '\n';
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
    }
    --curDepthScan;
}



/*!
    Ф-ия взятия хэша блока памяти
*/
template <typename HashType, typename HashType_DigestType>
std::string getHash(char* readBlock, const uint64_t blockSize)
{
    std::string result;
    HashType_DigestType digest;

    HashType m_currHash;
    m_currHash.process_bytes(readBlock, blockSize);
    m_currHash.get_digest(digest);

    const auto charDigest = reinterpret_cast<const char*>(&digest);

    boost::algorithm::hex(charDigest, charDigest + sizeof(md5::digest_type), std::back_inserter(result));

    result.resize(5); // Cutting hash

    return result;
}

/*!
    Структура DataFile - хранилище св-в файла
*/
struct DataFile
{
    explicit DataFile(const std::string& pathToFile_, uint32_t blockSize_)
        : pathToFile(pathToFile_)
        , fileSize(static_cast<uint64_t>(file_size(pathToFile)))
        , fd(pathToFile, std::ios::in | std::ios::binary)
        , blockSize(blockSize_)
    {
        if (!fd.is_open())
        {   std::cout << "Error open file!\n";    } // std::throw
    }

    ~DataFile()
    {
//        std::cout << "~DataFile()!\n";

        if (fd.is_open())
        {   fd.close(); }
    }

    const std::string& pathToFile;
    uint64_t fileSize;
    std::ifstream fd;

    uint32_t blockSize;
    std::string hashFile;
};


