#include "SvrConf.h"

int SvrConf::LoadConfigInfo(const char *confFile)
{
    ACE_TString serverInfo;
    ACE_Configuration_Heap config;

    ACE_DEBUG((LM_ERROR, ACE_TEXT("[%D](%P|%t) P2PSvr configure file: %s\n"), confFile));
    if(config.open() == -1)
    {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("[%D](%P|%t) Creat config failed!\n")), -1);
    }
    ACE_Registry_ImpExp configImp(config);
    if(configImp.import_config(confFile) == -1)
    {
        ACE_ERROR_RETURN
            ((LM_ERROR, ACE_TEXT("[%D](%P|%t) Open config file failed!\n")), -2);
    }
    ACE_Configuration_Section_Key statusSection;
    //打开节点名
    if(config.open_section(config.root_section(),
                ACE_TEXT("BCSvr"),
                0,
                statusSection) == 0)
    {
        //获取关键字信息
        if(config.get_string_value
                (statusSection, ACE_TEXT("IPInfo"), serverInfo) == -1)
        {
            ACE_ERROR_RETURN
                ((LM_ERROR, ACE_TEXT("[%D](%P|%t) BCSvr IPInfo does not exist\n")), -3);
        }
        m_BCSSvrInfo = serverInfo.c_str();
    }
    else
    {
        ACE_ERROR_RETURN
            ((LM_ERROR, ACE_TEXT("[%D](%P|%t) Can't open section[BCSvr]\n")), -4);
    }

    if(config.open_section(config.root_section(),
                ACE_TEXT("LOG"),
                0,
                statusSection) == 0)
    {
        ACE_TString dir;
        //获取关键字信息
        config.get_string_value
                (statusSection, ACE_TEXT("LogDir"), dir);

        ACE_TString file;
        config.get_string_value
                (statusSection, ACE_TEXT("LogFile"), file);

        if(dir.empty())
        {
            m_LogPath = "/var/log/TBCloud/P2PSvr/";
        }
        else
        {
            m_LogPath = dir.c_str();
        }

        if(file.empty())
        {
            m_LogPath += "P2PSvr.log";
        }
        else
        {
            m_LogPath += file.c_str();
        }
    }
    else
    {
        m_LogPath = "/var/log/TBCloud/P2PSvr/P2PSvr.log";
        ACE_ERROR_RETURN
            ((LM_ERROR, ACE_TEXT("[%D](%P|%t) Can't open section[LogFile]\n")), -4);
    }

    ACE_DEBUG((LM_ERROR, ACE_TEXT("[%D](%P|%t) P2PSvr log file: %s\n"), m_LogPath.c_str()));
}

