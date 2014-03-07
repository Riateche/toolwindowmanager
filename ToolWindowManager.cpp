#include "ToolWindowManager.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QEvent>
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>

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

ToolWindowManager::~ToolWindowManager() {
  //todo: delete all tool windows
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

  QTabWidget* tabWidget = new QTabWidget();
  tabWidget->setWindowFlags(widget->windowFlags() & Qt::Tool);
  tabWidget->addTab(widget, widget->windowIcon(), widget->windowTitle());
  tabWidget->setMovable(true);
  tabWidget->show();
  tabWidget->tabBar()->installEventFilter(this);
  m_hash_tabbar_to_tabwidget[tabWidget->tabBar()] = tabWidget;

  //widget->installEventFilter(this);

}

QPixmap ToolWindowManager::generateDragPixmap(QWidget* toolWindow) {
  QTabBar widget;
  widget.addTab(toolWindow->windowIcon(), toolWindow->windowTitle());
  return widget.grab();
}

bool ToolWindowManager::eventFilter(QObject *object, QEvent *event) {
  QTabBar* tabBar = qobject_cast<QTabBar*>(object);
  if (tabBar && event->type() == QEvent::MouseMove) {
    if (tabBar->rect().contains(static_cast<QMouseEvent*>(event)->pos())) {
      return false;
    }
    if (!(qApp->mouseButtons() & Qt::LeftButton)) {
      return false;
    }
    QTabWidget* tabWidget = m_hash_tabbar_to_tabwidget[tabBar];
    if (!tabWidget) { return false; }
    QWidget* toolWindow = tabWidget->currentWidget();
    if (!toolWindow) { return false; }
    QMouseEvent* releaseEvent = new QMouseEvent(QEvent::MouseButtonRelease,
                                                static_cast<QMouseEvent*>(event)->localPos(),
                                                Qt::LeftButton, Qt::LeftButton, 0);
    qApp->sendEvent(tabBar, releaseEvent);
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::red);
    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData();
    drag->setMimeData(mimeData);
    drag->setPixmap(generateDragPixmap(toolWindow));
    drag->exec(Qt::MoveAction);
  }
  return false;
  qDebug() << "buttons" << (qApp->mouseButtons() & Qt::LeftButton);
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
