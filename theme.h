#pragma once

// Thème de l'interface : JOUR (clair) ou SOMBRE. Le choix est mémorisé (QSettings)
// et appliqué à toute l'application via la feuille de style (style.h).
class Theme
{
public:
    static bool sombre();
    static void appliquer(bool sombre);
};
