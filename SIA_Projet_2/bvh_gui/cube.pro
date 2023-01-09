QT += core gui widgets opengl

TARGET = tpose
TEMPLATE = app

SOURCES += main.cpp

SOURCES += \
    mainwidget.cpp \
    geometryengine.cpp \
    ../joint.cpp

HEADERS += \
    mainwidget.h \
    geometryengine.h \
    ../joint.h

RESOURCES += \
    shaders.qrc \
    textures.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/tpose
INSTALLS += target
