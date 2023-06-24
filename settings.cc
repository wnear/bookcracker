#include "settings.h"
#include <QApplication>
#include <QSettings>
#include <iostream>
#include "qdebug.h"
Settings* Settings::s_instance = nullptr;

Settings* Settings::instance() {
    if (s_instance == nullptr) {
        s_instance = new Settings(qApp);
        s_instance->load();
        // s_instance->sync();
    }

    return s_instance;
}
Settings::Settings(QObject* parent)
    : QObject(parent),
      m_settings(new QSettings(QApplication::organizationName(),
                               QApplication::applicationName(), this)) {}

Settings::~Settings() {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    s_instance = nullptr; }
void Settings::load() {
    m_page_scale = m_settings->value("page/pagescale", 1.0).toDouble();
}

double Settings::pageScale() const { return m_page_scale; }
void Settings::setPageScale(double scale) { m_page_scale = scale;
    m_settings->setValue("page/pagescale", scale);
}

