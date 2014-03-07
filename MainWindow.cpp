#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QTextEdit>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  ui->toolWindowManager->setCentralWidget(new QTextEdit());

  ui->toolWindowManager->addToolWindow(new QPushButton("tool1"));
  ui->toolWindowManager->addToolWindow(new QPushButton("tool2"));
  ui->toolWindowManager->addToolWindow(new QPushButton("tool3"));
}

MainWindow::~MainWindow() {
  delete ui;
}
