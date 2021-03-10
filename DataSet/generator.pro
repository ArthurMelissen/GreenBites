NAME            = GreenBitesDataGenerator
TEMPLATE        = app

QT += network

HEADERS = Generator.h
SOURCES = main.cpp Generator.cpp

CONFIG += static

QMAKE_CXXFLAGS += -O3

