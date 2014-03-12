/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Pavel Strakhov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
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
    ui->toolWindowManager->addToolWindow(b1, i > 2 ? ToolWindowManager::RightDockArea :
                                                     ToolWindowManager::LeftDockArea);
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
  ui->toolWindowManager->moveToolWindow(toolWindow, state ?
                                          ToolWindowManager::LastUsedArea :
                                          ToolWindowManager::NoArea);

}

void MainWindow::toolWindowVisibilityChanged(QWidget *toolWindow, bool visible) {
  int index = ui->toolWindowManager->toolWindows().indexOf(toolWindow);
  actions[index]->blockSignals(true);
  actions[index]->setChecked(visible);
  actions[index]->blockSignals(false);
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
