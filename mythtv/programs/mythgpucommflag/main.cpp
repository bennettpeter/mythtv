#include <iostream>
using namespace std;

#include <QApplication>

#include "exitcodes.h"
#include "mythcontext.h"
#include "commandlineparser.h"
#include "mythlogging.h"
#include "mythversion.h"
#include "ringbuffer.h"
#include "gpuplayer.h"
#include "playercontext.h"
#include "remotefile.h"
#include "jobqueue.h"

#include "openclinterface.h"
#include "packetqueue.h"
#include "resultslist.h"
#include "audioconsumer.h"
#include "videoconsumer.h"

#define LOC      QString("MythGPUCommFlag: ")

namespace
{
    void cleanup()
    {
        delete gContext;
        gContext = NULL;
    }

    class CleanupGuard
    {
      public:
        typedef void (*CleanupFunc)();

      public:
        CleanupGuard(CleanupFunc cleanFunction) :
            m_cleanFunction(cleanFunction) {}

        ~CleanupGuard()
        {
            m_cleanFunction();
        }

      private:
        CleanupFunc m_cleanFunction;
    };
}

MythGPUCommFlagCommandLineParser cmdline;

static QString get_filename(ProgramInfo *program_info)
{
    QString filename = program_info->GetPathname();
    if (!QFile::exists(filename))
        filename = program_info->GetPlaybackURL(true);
    return filename;
}

static bool DoesFileExist(ProgramInfo *program_info)
{
    QString filename = get_filename(program_info);
    long long size = 0;
    bool exists = false;

    if (filename.startsWith("myth://"))
    {
        RemoteFile remotefile(filename, false, false, 0);
        struct stat filestat;

        if (remotefile.Exists(filename, &filestat))
        {
            exists = true;
            size = filestat.st_size;
        }
    }
    else
    {
        QFile file(filename);
        if (file.exists())
        {
            exists = true;
            size = file.size();
        }
    }

    if (!exists)
    {
        LOG(VB_GENERAL, LOG_ERR, QString("Couldn't find file %1, aborting.")
                .arg(filename));
        return false;
    }

    if (size == 0)
    {
        LOG(VB_GENERAL, LOG_ERR, QString("File %1 is zero-byte, aborting.")
                .arg(filename));
        return false;
    }

    return true;
}

int main(int argc, char **argv)
{
    // Parse commandline
    if (!cmdline.Parse(argc, argv))
    {
        cmdline.PrintHelp();
        return GENERIC_EXIT_INVALID_CMDLINE;
    }

    if (cmdline.toBool("showhelp"))
    {
        cmdline.PrintHelp();
        return GENERIC_EXIT_OK;
    }

    if (cmdline.toBool("showversion"))
    {
        cmdline.PrintVersion();
        return GENERIC_EXIT_OK;
    }

    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("mythgpucommflag");
    int retval = cmdline.ConfigureLogging("general", false);
//                                          !cmdline.toBool("noprogress"));
    if (retval != GENERIC_EXIT_OK)
        return retval;

    CleanupGuard callCleanup(cleanup);
    gContext = new MythContext(MYTH_BINARY_VERSION);
    if (!gContext->Init( false,  /* use gui */
                         false,  /* prompt for backend */
                         false,  /* bypass auto discovery */
                         false)) /* ignoreDB */
    {
        LOG(VB_GENERAL, LOG_EMERG, "Failed to init MythContext, exiting.");
        return GENERIC_EXIT_NO_MYTHCONTEXT;
    }
    cmdline.ApplySettingsOverride();

    // Determine capabilities
    OpenCLDeviceMap *devMap = new OpenCLDeviceMap();

    OpenCLDevice *devices[2];    // 0 = video, 1 = audio;
    devices[0] = devMap->GetBestDevice(NULL);
    devices[1] = devMap->GetBestDevice(devices[0]);

    if (devices[0])
    {
        LOG(VB_GENERAL, LOG_INFO, "OpenCL instance for video processing");
        devices[0]->Display();
    }
    else
        LOG(VB_GENERAL, LOG_INFO, "Video processing via software");

    if (devices[1])
    {
        LOG(VB_GENERAL, LOG_INFO, "OpenCL instance for audio processing");
        devices[1]->Display();
    }
    else
        LOG(VB_GENERAL, LOG_INFO, "Audio processing via software");

    ProgramInfo pginfo;
    int jobID = -1;

    if (cmdline.toBool("chanid") && cmdline.toBool("starttime"))
    {
        // operate on a recording in the database
        uint chanid = cmdline.toUInt("chanid");
        QDateTime starttime = cmdline.toDateTime("starttime");
        pginfo = ProgramInfo(chanid, starttime);
    }
    else if (cmdline.toBool("jobid"))
    {
        int jobType = JOB_NONE;
        jobID = cmdline.toInt("jobid");
        uint chanid;
        QDateTime starttime;

        if (!JobQueue::GetJobInfoFromID(jobID, jobType, chanid, starttime))
        {
            cerr << "mythgpucommflag: ERROR: Unable to find DB info for "
                 << "JobQueue ID# " << jobID << endl;
            return GENERIC_EXIT_NO_RECORDING_DATA;
        }
        int jobQueueCPU = gCoreContext->GetNumSetting("JobQueueCPU", 0);

        if (jobQueueCPU < 2)
        {
            myth_nice(17);
            myth_ioprio((0 == jobQueueCPU) ? 8 : 7);
        }

        pginfo = ProgramInfo(chanid, starttime);
    }
    else if (cmdline.toBool("file"))
    {
        pginfo = ProgramInfo(cmdline.toString("file"));
    }
    else
    {
        cerr << "You must indicate the recording to flag" << endl;
        return GENERIC_EXIT_NO_RECORDING_DATA;
    }
        
    QString filename = get_filename(&pginfo);

    if (!DoesFileExist(&pginfo))
    {
        // file not found on local filesystem
        // assume file is in Video storage group on local backend
        // and try again

        filename = QString("myth://Videos@%1/%2")
                            .arg(gCoreContext->GetHostName()).arg(filename);
        pginfo.SetPathname(filename);
        if (!DoesFileExist(&pginfo))
        {
            LOG(VB_GENERAL, LOG_ERR,
                "Unable to find file in defined storage paths.");
            return GENERIC_EXIT_PERMISSIONS_ERROR;
        }
    }

    // Open file
    RingBuffer *rb = RingBuffer::Create(filename, false);
    if (!rb)
    {
        LOG(VB_GENERAL, LOG_ERR, 
            QString("Unable to create RingBuffer for %1").arg(filename));
        return GENERIC_EXIT_PERMISSIONS_ERROR;
    }

    GPUPlayer *cfp = new GPUPlayer();
    PlayerContext *ctx = new PlayerContext(kFlaggerInUseID);
    ctx->SetSpecialDecode(kAVSpecialDecode_GPUDecode);
    ctx->SetPlayingInfo(&pginfo);
    ctx->SetRingBuffer(rb);
    ctx->SetPlayer(cfp);
    cfp->SetPlayerInfo(NULL, NULL, true, ctx);

    // Create input and output queues
    PacketQueue audioQ(2048);
    PacketQueue videoQ(2048);
    ResultsList audioMarks;
    ResultsList videoMarks;

    // Create consumer threads
    AudioConsumer *audioThread = new AudioConsumer(&audioQ, &audioMarks);
    VideoConsumer *videoThread = new VideoConsumer(&videoQ, &videoMarks);

    audioThread->SetOpenCLDevice(devices[1]);
    videoThread->SetOpenCLDevice(devices[0]);

    audioThread->start();
    videoThread->start();

    LOG(VB_GENERAL, LOG_INFO, "Starting queuing packets");

    // Loop: Pull frames from file, queue on video or audio queue
    cfp->ProcessFile(true, NULL, NULL, &audioQ, &videoQ);

    // Wait for processing to finish
    LOG(VB_GENERAL, LOG_INFO, "Done queuing packets");
    audioThread->done();
    videoThread->done();

    audioThread->wait();
    videoThread->wait();

    // Close file
    delete ctx;

    // Loop:
        // Summarize the various criteria to get commercial flag map

    // Send map to the db
    
    return(GENERIC_EXIT_OK);
}

/*
 * vim:ts=4:sw=4:ai:et:si:sts=4
 */
