#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QTextEdit>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose);
  ui->toolWindowManager->setCentralWidget(new QTextEdit());

  for(int i = 0; i < 6; i++) {

    QPushButton* b1 = new QPushButton(QString("tool%1").arg(i + 1));
    b1->setWindowTitle(b1->text());
    ui->toolWindowManager->addToolWindow(b1);
  }
}

MainWindow::~MainWindow() {
  delete ui;
}
