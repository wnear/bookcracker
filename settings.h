#pragma once
#include <QObject>
#include <memory>

class QSettings;

class Settings : public QObject {
    Q_OBJECT
  public:
    static Settings* instance();
    ~Settings();
    void load();

    double pageScale() const;
    void setPageScale(double scale);

  private:
    Settings(QObject* parent = 0);

    static Settings* s_instance;

    // using uniq_ptr is suffice for QSettings to sync.
    // from the qdoc page about `QSettings::sync()`:
    // " This function is called automatically from QSettings's destructor
    //   and by the event loop at regular intervals, so you normally don't need to call it
    //   yourself."
    std::unique_ptr<QSettings> m_settings;

    double m_page_scale;
};
