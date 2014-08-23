#include "triangles.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    triangles w;
    w.show();
    return a.exec();
}
