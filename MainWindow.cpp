#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QTextEdit>
#include <QPushButton>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose);
  ui->toolWindowManager->setCentralWidget(new QTextEdit());
  connect(ui->toolWindowManager, SIGNAL(toolWindowVisibilityChanged(QWidget*,bool)),
          this, SLOT(toolWindowVisibilityChanged(QWidget*,bool)));

  for(int i = 0; i < 6; i++) {
    QPushButton* b1 = new QPushButton(QString("tool%1").arg(i + 1));
    b1->setWindowTitle(b1->text());
    b1->setObjectName(b1->text());
    QAction* action = ui->menuToolWindows->addAction(b1->text());
    action->setData(i);
    action->setCheckable(true);
    action->setChecked(true);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(toolWindowActionToggled(bool)));
    actions << action;
    ui->toolWindowManager->addToolWindow(b1);
  }
  on_actionRestoreState_triggered();
}

MainWindow::~MainWindow() {
  on_actionSaveState_triggered();
  delete ui;
}

void MainWindow::toolWindowActionToggled(bool state) {
  int index = static_cast<QAction*>(sender())->data().toInt();
  QWidget* toolWindow = ui->toolWindowManager->toolWindows()[index];
  ui->toolWindowManager->setToolWindowVisible(toolWindow, state);

}

void MainWindow::toolWindowVisibilityChanged(QWidget *toolWindow, bool visible) {
  int index = ui->toolWindowManager->toolWindows().indexOf(toolWindow);
  actions[index]->blockSignals(true);
  actions[index]->setChecked(visible);
  actions[index]->blockSignals(false);

}

void MainWindow::on_actionCentralWidget_toggled(bool on) {
  ui->toolWindowManager->setCentralWidget(on ? new QTextEdit() : 0);

}

void MainWindow::on_actionSaveState_triggered() {
  QSettings settings;
  settings.setValue("toolWindowManagerState", ui->toolWindowManager->saveState());
  settings.setValue("geometry", saveGeometry());
}

void MainWindow::on_actionRestoreState_triggered() {
  QSettings settings;
  restoreGeometry(settings.value("geometry").toByteArray());
  ui->toolWindowManager->restoreState(settings.value("toolWindowManagerState"));
}
