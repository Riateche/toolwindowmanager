#include "ToolWindowManager.h"
#include <QVBoxLayout>

ToolWindowManager::ToolWindowManager(QWidget *parent) :
  QWidget(parent)
{
  m_centralWidget = 0;
  m_mainSplitter = new QSplitter();
  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(m_mainSplitter);
  mainLayout->setContentsMargins(0, 0, 0, 0);

  m_centralWidgetContainer = new QWidget();
  m_mainSplitter->addWidget(m_centralWidgetContainer);
  QVBoxLayout* layout = new QVBoxLayout(m_centralWidgetContainer);
  layout->setContentsMargins(0, 0, 0, 0);
}

void ToolWindowManager::setCentralWidget(QWidget *widget) {
  if(m_centralWidget) {
    m_centralWidget->deleteLater();
  }
  m_centralWidget = widget;
  m_centralWidgetContainer->layout()->addWidget(widget);

}

void ToolWindowManager::addToolWindow(QWidget *widget) {
  m_toolWindows << widget;
  widget->setWindowFlags(widget->windowFlags() & Qt::Tool);
  widget->setParent(0);
  widget->show();

}
