#-------------------------------------------------
#
# Project created by QtCreator 2015-10-16T20:53:48
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = agent
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

#
# APM Planner library
#
include(apm_planner.pri)

INCLUDEPATH += \
    agent \
    network \
    sensor \

HEADERS += \
    agent/UBAgent.h \
    network/UBNetwork.h \
    network/UBPacket.h \
    sensor/UBVision.h \
    config.h

SOURCES += \
    agent/UBAgent.cpp \
    network/UBNetwork.cpp \
    network/UBPacket.cpp \
    sensor/UBVision.cpp
