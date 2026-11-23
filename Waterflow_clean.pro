QT       += core gui sql printsupport charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Application WaterFlow : Commandes + Livraisons + Personnel
# Ordre des dépendances : Personnel (livreurs) et Commandes -> Livraisons
#
# Architecture MVC :
#   Modèle  : commande, employe, livraison, historiquelivraison (accès Oracle)
#             itineraire (logique métier pure : optimisation des tournées, ETA)
#   Vue     : mainwindow.ui (fichier .ui unique), cartetournee, barchart, style.h
#   Contrôleur : mainwindow, personnel, livraisons (slots)
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    connection.cpp \
    theme.cpp \
    commande.cpp \
    employe.cpp \
    personnel.cpp \
    barchart.cpp \
    animationlivraison.cpp \
    itineraire.cpp \
    historiquelivraison.cpp \
    livraison.cpp \
    cartetournee.cpp \
    livraisons.cpp

HEADERS += \
    mainwindow.h \
    connection.h \
    commande.h \
    employe.h \
    personnel.h \
    barchart.h \
    animationlivraison.h \
    itineraire.h \
    historiquelivraison.h \
    livraison.h \
    cartetournee.h \
    livraisons.h \
    session.h \
    theme.h \
    style.h \
    logindialog.h

# Interface UNIQUE : un seul fichier .ui (dashboard + Commandes + Livraisons + Personnel)
FORMS += \
    mainwindow.ui
