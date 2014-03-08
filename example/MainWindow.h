#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

private:
  Ui::MainWindow *ui;
  QList<QAction*> actions;

private slots:
  void toolWindowActionToggled(bool state);
  void toolWindowVisibilityChanged(QWidget* toolWindow, bool visible);
  void on_actionCentralWidget_toggled(bool arg1);
  void on_actionSaveState_triggered();
  void on_actionRestoreState_triggered();
};

#endif // MAINWINDOW_H
