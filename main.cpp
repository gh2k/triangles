#include "triangles.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Triangles w;
    w.show();
    return a.exec();
}
