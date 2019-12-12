/*  -*- Mode: c++ -*-
 *
 * Copyright (C) John Poet 2018
 *
 * This file is part of MythTV
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _MythTVExternRecApp_H_
#define _MythTVExternRecApp_H_

#include <QObject>
#include <QtCore/QtCore>

#include <atomic>
#include <condition_variable>

class MythExternControl;

class MythExternRecApp : public QObject
{
    Q_OBJECT

  public:
    MythExternRecApp(QString command,  QString conf_file,
                     QString log_file, QString logging);
    ~MythExternRecApp(void);

    bool Open(void);
    void Run(void);

    QString Desc(void) const;
    void MythLog(const QString & msg)
    { SendMessage("", "0", QString("STATUS:%1").arg(msg)); }
    void SetErrorMsg(const QString & msg) { emit ErrorMessage(msg); }

  signals:
    void SetDescription(const QString & desc);
    void SendMessage(const QString & func, const QString & serial,
                     const QString & msg);
    void ErrorMessage(const QString & msg);
    void Opened(void);
    void Done(void);
    void Streaming(bool val);
    void Fill(const QByteArray & buffer);

  public slots:
    void ProcStarted(void);
    void ProcFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void ProcStateChanged(QProcess::ProcessState newState);
    void ProcError(QProcess::ProcessError error);
    void ProcReadStandardError(void);
    void ProcReadStandardOutput(void);

    void Close(void);
    void StartStreaming(const QString & serial);
    void StopStreaming(const QString & serial, bool silent);
    void LockTimeout(const QString & serial);
    void HasTuner(const QString & serial);
    void LoadChannels(const QString & serial);
    void FirstChannel(const QString & serial);
    void NextChannel(const QString & serial);

    void TuneChannel(const QString & serial, const QString & channum);
    void HasPictureAttributes(const QString & serial);
    void SetBlockSize(const QString & serial, int blksz);

  protected:
    void GetChannel(const QString & serial, const QString & func);
    void TerminateProcess(void);

  private:
    bool config(void);

    bool                    m_fatal        { false };

    std::atomic<bool>       m_run          { true };
    std::condition_variable m_runCond;
    std::mutex              m_runMutex;
    std::atomic<bool>       m_streaming    { false };
    int                     m_result       { 0 };

    uint                    m_bufferMax    { 188 * 10000 };
    uint                    m_blockSize    { m_bufferMax / 4 };

    QProcess                m_proc;
    QString                 m_command;

    QString                 m_recCommand;
    QString                 m_recDesc;

    QMap<QString, QString>  m_appEnv;

    QString                 m_tuneCommand;
    QString                 m_channelsIni;
    uint                    m_lockTimeout  { 0 };

    QString                 m_scanCommand;
    uint                    m_scanTimeout  { 120000 };

    QString                 m_logFile;
    QString                 m_logging;
    QString                 m_configIni;
    QString                 m_desc;

    bool                    m_tuned        { false };

    // Channel scanning
    QSettings              *m_chanSettings { nullptr };
    QStringList             m_channels;
    int                     m_channelIdx   { -1 };

};

#endif
