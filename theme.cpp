#include "theme.h"
#include "style.h"
#include <QApplication>
#include <QSettings>

namespace
{
bool g_lu = false;
bool g_sombre = false;
}

bool Theme::sombre()
{
    if (!g_lu) {
        g_sombre = QSettings("WaterFlow", "Interface").value("themeSombre", false).toBool();
        g_lu = true;
    }
    return g_sombre;
}

void Theme::appliquer(bool sombre)
{
    g_sombre = sombre;
    g_lu = true;
    QSettings("WaterFlow", "Interface").setValue("themeSombre", sombre);
    qApp->setStyleSheet(feuilleDeStyle(sombre));
}
