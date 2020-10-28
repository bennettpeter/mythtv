#ifndef MYTHPLAYEROVERLAYUI_H
#define MYTHPLAYEROVERLAYUI_H

// MythTV
#include "mythplayeruibase.h"

class MTV_PUBLIC MythPlayerOverlayUI : public MythPlayerUIBase
{
    Q_OBJECT

  signals:
    void OverlayStateChanged(MythOverlayState OverlayState);

  public slots:
    void BrowsingChanged(bool Browsing);
    void EditingChanged(bool Editing);

  public:
    MythPlayerOverlayUI(MythMainWindow* MainWindow, TV* Tv, PlayerContext* Context, PlayerFlags Flags);
   ~MythPlayerOverlayUI() override;

    virtual void UpdateSliderInfo(osdInfo& Info, bool PaddedFields = false);

    OSD* GetOSD()    { return &m_osd; }
    void LockOSD()   { m_osdLock.lock(); }
    void UnlockOSD() { m_osdLock.unlock(); }

  protected slots:
    void UpdateOSDMessage (const QString& Message);
    void UpdateOSDMessage (const QString& Message, OSDTimeout Timeout);
    void SetOSDStatus     (const QString &Title, OSDTimeout Timeout);
    void UpdateOSDStatus  (osdInfo &Info, int Type, enum OSDTimeout Timeout);
    void UpdateOSDStatus  (const QString& Title, const QString& Desc,
                           const QString& Value, int Type, const QString& Units,
                           int Position, OSDTimeout Timeout);
    void ChangeOSDPositionUpdates(bool Enable);
    void UpdateOSDPosition();

  protected:
    virtual int64_t GetSecondsPlayed(bool HonorCutList, int Divisor = 1000);
    virtual int64_t GetTotalSeconds(bool HonorCutList, int Divisor = 1000) const;

    OSD    m_osd;
    QMutex m_osdLock  { QMutex::Recursive };
    bool   m_browsing { false };
    bool   m_editing  { false };

  private:
    Q_DISABLE_COPY(MythPlayerOverlayUI)

    QTimer m_positionUpdateTimer;
};

#endif