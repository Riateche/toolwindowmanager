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

  QPushButton* b1 = new QPushButton("tool1");
  b1->setWindowTitle("tool1");
  ui->toolWindowManager->addToolWindow(b1);

  QPushButton* b2 = new QPushButton("tool2");
  b2->setWindowTitle("tool2");
  ui->toolWindowManager->addToolWindow(b2);

  QPushButton* b3 = new QPushButton("tool3");
  b3->setWindowTitle("tool3");
  ui->toolWindowManager->addToolWindow(b3);

  //ui->toolWindowManager->addToolWindow(new QPushButton("tool2"));
  //ui->toolWindowManager->addToolWindow(new QPushButton("tool3"));
}

MainWindow::~MainWindow() {
  delete ui;
}
