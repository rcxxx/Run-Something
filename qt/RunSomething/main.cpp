#include "runicon.h"

int main(int argc, char *argv[])
{

    QApplication app(argc, argv);

    RunIcon tray_icon;

    return app.exec();
}
