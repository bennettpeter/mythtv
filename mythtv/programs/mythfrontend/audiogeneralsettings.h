#ifndef MYTHAUDIOSETTINGS_H
#define MYTHAUDIOSETTINGS_H

#include <QStringList>
#include <QObject>
#include <QMutex>

#include "mythuistatetype.h"
#include "mythscreentype.h"
#include "mythdialogbox.h"
#include "mythuibutton.h"
#include "audiooutput.h"
#include "mythcontext.h"
#include "standardsettings.h"
#include "mthread.h"

class AudioDeviceComboBox;
class AudioTest;

class AudioConfigScreen : public StandardSettingDialog
{
  public:
    AudioConfigScreen(MythScreenStack *parent, const char *name,
                      GroupSetting *groupSetting);
  protected:
    void Load(void) override; // StandardSettingDialog
    void Init(void) override; // StandardSettingDialog
};

class AudioConfigSettings : public GroupSetting
{
    Q_OBJECT

  public:
    AudioConfigSettings();
    void Load() override; // StandardSetting

    using ADCMap = QMap<QString,AudioOutput::AudioDeviceConfig>;

    ADCMap &AudioDeviceMap(void) { return m_audioDevs; };
    AudioOutput::ADCVect &AudioDeviceVect(void) { return m_devices; };

    void CheckConfiguration(void);

  private slots:
    void UpdateVisibility(StandardSetting *);
    AudioOutputSettings UpdateCapabilities(bool restore = true,
                                           bool AC3 = false);
    AudioOutputSettings UpdateCapabilitiesAC3(void);
    void AudioRescan();
    void UpdateAudioTest();

  private:
    void setMPCMEnabled(bool flag);

    AudioDeviceComboBox *OutputDevice();
    static HostComboBoxSetting *MaxAudioChannels();
    static HostCheckBoxSetting *AudioUpmix();
    static HostComboBoxSetting *AudioUpmixType();
    static HostCheckBoxSetting *AC3PassThrough();
    static HostCheckBoxSetting *DTSPassThrough();
    static HostCheckBoxSetting *EAC3PassThrough();
    static HostCheckBoxSetting *TrueHDPassThrough();
    static HostCheckBoxSetting *DTSHDPassThrough();
    static HostCheckBoxSetting *MythControlsVolume();
    static HostComboBoxSetting *MixerDevice();
    static HostComboBoxSetting *MixerControl();
    static HostSpinBoxSetting  *MixerVolume();
    static HostSpinBoxSetting  *PCMVolume();

    //advanced setting
    static HostCheckBoxSetting *MPCM();
    static HostCheckBoxSetting *SRCQualityOverride();
    static HostComboBoxSetting *SRCQuality();
    static HostCheckBoxSetting *Audio48kOverride();
    static HostCheckBoxSetting *PassThroughOverride();
    static HostComboBoxSetting *PassThroughOutputDevice();
    static HostCheckBoxSetting *SPDIFRateOverride();
    static HostCheckBoxSetting *HBRPassthrough();

    bool                CheckPassthrough();

    AudioDeviceComboBox *m_outputDevice              {nullptr};
    HostComboBoxSetting *m_maxAudioChannels          {nullptr};
    HostCheckBoxSetting *m_audioUpmix                {nullptr};
    HostComboBoxSetting *m_audioUpmixType            {nullptr};

    // digital settings
    GroupSetting        *m_triggerDigital            {nullptr};
    HostCheckBoxSetting *m_ac3PassThrough            {nullptr};
    HostCheckBoxSetting *m_dtsPassThrough            {nullptr};
    HostCheckBoxSetting *m_eac3PassThrough           {nullptr};
    HostCheckBoxSetting *m_trueHDPassThrough         {nullptr};
    HostCheckBoxSetting *m_dtsHDPassThrough          {nullptr};
    //advanced setting
    HostCheckBoxSetting *m_mpcm                      {nullptr};
    HostCheckBoxSetting *m_passThroughOverride       {nullptr};
    HostComboBoxSetting *m_passThroughDeviceOverride {nullptr};

    AudioTest           *m_audioTest                 {nullptr};

    ADCMap               m_audioDevs;
    AudioOutput::ADCVect m_devices;
    QMutex               m_slotLock;

    int                  m_maxSpeakers               {0};
    QString              m_lastAudioDevice;
    static const char   *kMixerControlControls[];
};

class AudioDeviceComboBox : public HostComboBoxSetting
{
    Q_OBJECT
  public:
    explicit AudioDeviceComboBox(AudioConfigSettings*);
    void AudioRescan();

    void edit(MythScreenType * screen) override; // MythUIComboBoxSetting

  private slots:
    void AudioDescriptionHelp(StandardSetting * setting);

  private:
    AudioConfigSettings *m_parent {nullptr};
};

class ChannelChangedEvent : public QEvent
{
  public:
    ChannelChangedEvent(const QString& channame, bool fulltest) :
        QEvent(kEventType), m_channel(channame), m_fulltest(fulltest) {}
    ~ChannelChangedEvent() = default;

    QString m_channel;
    bool    m_fulltest;

    static Type kEventType;
};

class AudioTestThread : public MThread
{
    Q_DECLARE_TR_FUNCTIONS(AudioTestThread);

  public:

    AudioTestThread(QObject *parent, QString main, QString passthrough,
                    int channels, AudioOutputSettings &settings, bool hd);
    ~AudioTestThread();

    void cancel();
    QString result();
    void setChannel(int channel);

  protected:
    void run() override; // MThread

  private:
    QObject                *m_parent      {nullptr};
    AudioOutput            *m_audioOutput {nullptr};
    int                     m_channels;
    QString                 m_device;
    QString                 m_passthrough;
    bool                    m_interrupted {false};
    int                     m_channel     {-1};
    bool                    m_hd          {false};
    int                     m_samplerate  {48000};
    AudioFormat             m_format      {FORMAT_S16};
};

class AudioTest : public GroupSetting
{
    Q_OBJECT
  public:
    AudioTest();
    ~AudioTest();
    void UpdateCapabilities(const QString &main, const QString &passthrough,
                            int channels, const AudioOutputSettings &settings);
    bool event(QEvent *event) override; // QObject

  private:
    int                         m_channels      {2};
    ButtonStandardSetting      *m_frontleft     {nullptr};
    ButtonStandardSetting      *m_frontright    {nullptr};
    ButtonStandardSetting      *m_center        {nullptr};
    ButtonStandardSetting      *m_surroundleft  {nullptr};
    ButtonStandardSetting      *m_surroundright {nullptr};
    ButtonStandardSetting      *m_rearleft      {nullptr};
    ButtonStandardSetting      *m_rearright     {nullptr};
    ButtonStandardSetting      *m_lfe           {nullptr};
    AudioTestThread            *m_at            {nullptr};
    ButtonStandardSetting      *m_startButton   {nullptr};
    TransMythUICheckBoxSetting *m_hd            {nullptr};
    QString                     m_main;
    QString                     m_passthrough;
    AudioOutputSettings         m_settings;
    bool                        m_quality       {false};

  private slots:
    void toggle();
    void togglequality(void);
    void cancelTest();
    void prepareTest();
};
#endif
