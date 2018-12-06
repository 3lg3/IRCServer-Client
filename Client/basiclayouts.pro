QT += widgets

HEADERS     = dialog.h \
    LoginDialog.h \
    RoomDialog.h

SOURCES     = dialog.cpp \
              main.cpp \
    LoginDialog.cpp \
    RoomDialog.cpp



# install

target.path = $$[QT_INSTALL_EXAMPLES]/widgets/layouts/basiclayouts

INSTALLS += target

