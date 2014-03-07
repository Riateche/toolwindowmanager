#include "ToolWindowManager.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QEvent>
#include <QApplication>

ToolWindowManager::ToolWindowManager(QWidget *parent) :
  QWidget(parent)
{
  m_borderSensitivity = 10;

  m_centralWidget = 0;
  m_mainSplitter = new QSplitter();
  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(m_mainSplitter);
  mainLayout->setContentsMargins(0, 0, 0, 0);

  m_centralWidgetContainer = new QWidget();
  m_mainSplitter->addWidget(m_centralWidgetContainer);
  QVBoxLayout* layout = new QVBoxLayout(m_centralWidgetContainer);
  layout->setContentsMargins(0, 0, 0, 0);
  m_placeHolder = new QWidget();
  QPalette palette = m_placeHolder->palette();
  palette.setColor(QPalette::Window, palette.color(QPalette::Highlight));
  m_placeHolder->setPalette(palette);
  m_placeHolder->setAutoFillBackground(true);
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
  widget->installEventFilter(this);

}

bool ToolWindowManager::eventFilter(QObject *object, QEvent *event) {
  if (m_toolWindows.contains(static_cast<QWidget*>(object)) && event->type() == QEvent::Move) {
    QPoint globalPos = QCursor::pos();
    if(m_placeHolder->isVisible()) {
      QRect placeHolderRect = m_placeHolder->rect();
      placeHolderRect.adjust(-m_borderSensitivity, -m_borderSensitivity,
                              m_borderSensitivity,  m_borderSensitivity);
      if (!placeHolderRect.contains(m_placeHolder->mapFromGlobal(globalPos))) {
        m_placeHolder->hide();
        m_placeHolder->setParent(0);
        qDebug() << "placeholder removed";
      }
    } else {
      foreach(QSplitter* splitter, findChildren<QSplitter*>()) {
        int widgetsCount = splitter->count();
        int indexUnderMouse = -1;
        for(int widgetIndex = 0; widgetIndex < widgetsCount; ++widgetIndex) {
          QWidget* widget = splitter->widget(widgetIndex);
          QPoint localPos = widget->mapFromGlobal(globalPos);
          if (splitter->orientation() == Qt::Horizontal) {
            if (qAbs(localPos.x()) < m_borderSensitivity) {
              indexUnderMouse = widgetIndex;
              break;
            }
            if (qAbs(localPos.x() - widget->width()) < m_borderSensitivity) {
              indexUnderMouse = widgetIndex + 1;
              break;
            }
          } else {
            if (qAbs(localPos.y()) < m_borderSensitivity) {
              indexUnderMouse = widgetIndex;
              break;
            }
            if (qAbs(localPos.y() - widget->height()) < m_borderSensitivity) {
              indexUnderMouse = widgetIndex + 1;
              break;
            }
          }
        }
        qDebug() << "index" << indexUnderMouse;
        if(indexUnderMouse >= 0) {
          QSize placeHolderSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
          if (splitter->orientation() == Qt::Horizontal) {
            placeHolderSize.setWidth(m_borderSensitivity);
          } else {
            placeHolderSize.setHeight(m_borderSensitivity);
          }
          m_placeHolder->setFixedSize(placeHolderSize);
          splitter->insertWidget(indexUnderMouse, m_placeHolder);
          m_placeHolder->show();
          qDebug() << "placeholder inserted";
          break;
        }
      }
    }
  }
  return false;
}
