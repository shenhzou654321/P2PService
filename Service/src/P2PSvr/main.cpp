#include <string>
#include <iostream>
#include "ace/Log_Msg.h"
#include "ace/Get_Opt.h"
#include "ace/INET_Addr.h"
#include "ace/Signal.h"
#include "ace/streams.h"

#include "Tools.h"
#include "P2PSvr.h"
#include "SvrConf.h"

using std::string;

const string SVRNAME = "P2PSvr";
const string VERSION = "1.0.0.0";

SvrConf theConf;
extern P2PSvr theApp;

void Usage();
void ShowVersionInfo(int outFlag);
void SigHandle(int sig);

int main(int argc, char *argv[])
{
    const char *confFile = "../../etc/P2PSvr.conf";
    static const ACE_TCHAR options[] = ACE_TEXT("hvc:");
    ACE_Get_Opt cmdOpts(argc, argv, options); 
    int opt = 0;

    while((opt = cmdOpts()) != EOF)
    {
        switch(opt)
        {
            case 'h':
                Usage();
                exit(0);
                break;
            case 'v':
                ShowVersionInfo(1);
                exit(0);
            case 'c':
                confFile = cmdOpts.opt_arg();
                ShowVersionInfo(0);
                break;
            default:
                fprintf(stderr, "invalid option '%c', use '-h' to see usage.\n", opt);
                exit(-1);
        }
    }

    theConf.LoadConfigInfo(confFile);
    string BCSAddr = theConf.BCSvrInfo();
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) BCSAddr: %s\n"), BCSAddr.c_str()));

    string logFile = theConf.LogFile();
    ACE_OSTREAM_TYPE *output = new ofstream(logFile.c_str());
    ACE_LOG_MSG->msg_ostream(output, 1);
    ACE_LOG_MSG->set_flags(ACE_Log_Msg::OSTREAM);
    ACE_LOG_MSG->clr_flags(ACE_Log_Msg::STDERR);

    try
    {
        ACE_Sig_Action sa(SigHandle, SIGINT);
        theApp.Init(BCSAddr);
        theApp.Start();
        theApp.Run();
        theApp.Fini();
    }
    catch(...)
    {
        theApp.Fini();
        return -1;
    }

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) P2PSvr exit.\n")));

    return 0;
}

void ShowVersionInfo(int outFlag)
{
    string verStr = VersionInfo(SVRNAME, VERSION);
    if(verStr.length() <= 0)
    {
        ACE_ERROR((LM_ERROR, ACE_TEXT("[%D](%P|%t) General version failed!\n")));
        return;
    }

    if(outFlag == 1)
        printf("%s", verStr.c_str());
    else
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("[%D](%P|%t) %s"), verStr.c_str()));
}

void Usage()
{
    const char *msg = 
        "usage: ./P2PSvr OPTIONS\n"
        " -c config-file: give config file path, default is '../conf/P2PSvrConf.conf'\n"
        " -h: print this help info and exit\n"
        " -v: printf the version of program\n"
        ;
    printf("%s", msg);
}

void SigHandle(int sig)
{
    theApp.Stop();
}
