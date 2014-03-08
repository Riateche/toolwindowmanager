#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  a.setOrganizationName("ToolWindowManagerTest");
  a.setApplicationName("ToolWindowManagerTest");
  MainWindow* w = new MainWindow();
  w->show();

  return a.exec();
}
