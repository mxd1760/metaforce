#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <system_error>

#include "HECLDatabase.hpp"
#include "CSQLiteMain.hpp"

namespace HECLDatabase
{

class CProject : public IProject
{
    std::string m_rootPath;
    CSQLiteMain* m_db;
public:
    CProject(const std::string& rootPath)
    : m_rootPath(rootPath)
    {
        /* Stat for existing project directory (must already exist) */
        struct stat myStat;
        if (stat(m_rootPath.c_str(), &myStat))
            throw std::error_code(errno, std::system_category());

        if (!S_ISDIR(myStat.st_mode))
            throw std::invalid_argument("provided path must be a directory; '" + m_rootPath + "' isn't");

        /* Create project directory */
        if (mkdir((m_rootPath + "/.hecl").c_str(), 0755))
        {
            if (errno != EEXIST)
                throw std::error_code(errno, std::system_category());
        }

        /* Create or open databases */
        m_db = new CSQLiteMain(m_rootPath + "/.hecl/main.db");
    }

    ~CProject()
    {
        delete m_db;
    }

    void registerLogger(HECL::TLogger logger)
    {
    }

    std::string getProjectRootPath(bool absolute) const
    {
    }

    bool validateSubPath(const std::string& subpath) const
    {
    }

    bool addPath(const std::string& path)
    {
    }

    bool removePath(const std::string& path, bool recursive)
    {
    }

    bool addGroup(const std::string& path)
    {
    }

    bool removeGroup(const std::string& path)
    {
    }

    bool cookPath(const std::string& path,
                          std::function<void(std::string&, Cost, unsigned)> feedbackCb,
                          bool recursive)
    {
    }

    void interruptCook()
    {
    }

    bool cleanPath(const std::string& path, bool recursive)
    {
    }

    bool packagePath(const std::string& path, bool recursive)
    {
    }

};

IProject* NewProject(const std::string& rootPath)
{
    return new CProject(rootPath);
}

}
