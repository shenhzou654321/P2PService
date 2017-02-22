#ifndef __SVRCONF_H__
#define __SVRCONF_H__

#include "ace/Configuration_Import_Export.h"
#include <string>

using std::string;

class SvrConf
{
    public:
        int LoadConfigInfo(const char *confFile);
        string BCSvrInfo()
        {
            return m_BCSSvrInfo;
        }
        string LogFile()
        {
            return m_LogPath;
        }
    private:
        string          m_BCSSvrInfo;
        string          m_LogPath;
};


#endif
