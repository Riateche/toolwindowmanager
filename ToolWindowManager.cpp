#include "ToolWindowManager.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QEvent>
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>

template<class T>
T findClosestParent(QWidget* widget) {
  while(widget) {
    if (qobject_cast<T>(widget)) {
      return static_cast<T>(widget);
    }
    widget = widget->parentWidget();
  }
  return 0;
}

ToolWindowManager::ToolWindowManager(QWidget *parent) :
  QWidget(parent)
{
  m_borderSensitivity = 16;
  m_dragMimeType = "application/x-tool-window-id";

  m_centralWidget = 0;
  QSplitter* mainSplitter1 = new QSplitter();
  mainSplitter1->setOrientation(Qt::Horizontal);
  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(mainSplitter1);
  mainLayout->setContentsMargins(0, 0, 0, 0);

  QSplitter* mainSplitter2 = new QSplitter();
  mainSplitter2->setOrientation(Qt::Vertical);
  mainSplitter1->addWidget(mainSplitter2);
  m_centralWidgetContainer = new QWidget();
  mainSplitter2->addWidget(m_centralWidgetContainer);
  QVBoxLayout* layout = new QVBoxLayout(m_centralWidgetContainer);
  layout->setContentsMargins(0, 0, 0, 0);
  m_placeHolder = new QWidget();
  QPalette palette = m_placeHolder->palette();
  palette.setColor(QPalette::Window, palette.color(QPalette::Highlight));
  m_placeHolder->setPalette(palette);
  m_placeHolder->setAutoFillBackground(true);
  setAcceptDrops(true);

  connect(&m_dropSuggestionSwitchTimer, SIGNAL(timeout()),
          this, SLOT(dropSuggestionSwitchTimeout()));
  m_dropSuggestionSwitchTimer.setInterval(1000); //todo: add setter for this interval
  m_dropCurrentSuggestionIndex = -1;
}

ToolWindowManager::~ToolWindowManager() {
  foreach(QWidget* toolWindow, m_toolWindows) {
    QTabWidget* tabWidget = findClosestParent<QTabWidget*>(toolWindow);
    if (!tabWidget) {
      qWarning("cannot find tab widget for tool window");
    } else {
      delete tabWidget;
    }
  }
}

void ToolWindowManager::setCentralWidget(QWidget *widget) {
  if(m_centralWidget) {
    m_centralWidget->deleteLater();
  }
  m_centralWidget = widget;
  m_centralWidgetContainer->layout()->addWidget(widget);

}

void ToolWindowManager::addToolWindow(QWidget *widget) {
  if (!widget) {
    qWarning("cannot add null widget");
    return;
  }
  if (m_toolWindows.contains(widget)) {
    qWarning("this tool window has already been added");
    return;
  }
  m_toolWindows << widget;
  QTabWidget* tabWidget = createTabWidget();
  tabWidget->addTab(widget, widget->windowIcon(), widget->windowTitle());
}

QWidget *ToolWindowManager::createDockItem(QWidget* toolWindow, Qt::Orientation parentOrientation) {
  QSplitter* splitter = new QSplitter();
  splitter->setOrientation(parentOrientation == Qt::Horizontal ? Qt::Vertical : Qt::Horizontal);
  QTabWidget* tabWidget = createTabWidget();
  tabWidget->addTab(toolWindow, toolWindow->windowIcon(), toolWindow->windowTitle());
  splitter->addWidget(tabWidget);
  return splitter;
}

QTabWidget *ToolWindowManager::createTabWidget() {
  QTabWidget* tabWidget = new QTabWidget();
  tabWidget->setWindowFlags(tabWidget->windowFlags() & Qt::Tool);
  tabWidget->setMovable(true);
  tabWidget->show();
  tabWidget->tabBar()->installEventFilter(this);
  m_hash_tabbar_to_tabwidget[tabWidget->tabBar()] = tabWidget;
  return tabWidget;
  //todo: clear m_hash_tabbar_to_tabwidget when widgets are destroyed

}

void ToolWindowManager::deleteEmptyItems(QTabWidget *tabWidget) {
  if (tabWidget->count() == 0) {
    tabWidget->deleteLater();
    QSplitter* splitter = qobject_cast<QSplitter*>(tabWidget->parentWidget());
    while(splitter) {
      if (splitter->count() == 1) {
        splitter->deleteLater();
        splitter = qobject_cast<QSplitter*>(splitter->parentWidget());
      } else {
        break;
      }
    }
  }
}

void ToolWindowManager::hidePlaceHolder() {
  m_placeHolder->hide();
  m_placeHolder->setParent(0);
  if (m_dropSuggestionSwitchTimer.isActive()) {
    m_dropSuggestionSwitchTimer.stop();
  }
}

QPixmap ToolWindowManager::generateDragPixmap(QWidget* toolWindow) {
  QTabBar widget;
  widget.addTab(toolWindow->windowIcon(), toolWindow->windowTitle());
  return widget.grab();
}

void ToolWindowManager::dropSuggestionSwitchTimeout() {
  m_dropCurrentSuggestionIndex++;
  int currentIndex = 0;
  bool foundPlace = false;
  QSplitter* currentSplitter = findClosestParent<QSplitter*>(m_placeHolder);
  foreach(QSplitter* splitter, findChildren<QSplitter*>()) {
    int widgetsCount = splitter->count();
    int indexUnderMouse = -1;
    for(int widgetIndex = 0; widgetIndex < widgetsCount; ++widgetIndex) {
      QWidget* widget = splitter->widget(widgetIndex);
      QPoint localPos = widget->mapFromGlobal(m_dropGlobalPos);
      if (splitter->orientation() == Qt::Horizontal) {
        if (localPos.y() >= 0 && localPos.y() < widget->height()) {
          if (qAbs(localPos.x()) < m_borderSensitivity) {
            indexUnderMouse = widgetIndex;
            break;
          }
          if (qAbs(localPos.x() - widget->width()) < m_borderSensitivity) {
            indexUnderMouse = widgetIndex + 1;
            break;
          }
        }
      } else {
        if (localPos.x() >= 0 && localPos.x() < widget->width()) {
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
    }
    if (currentSplitter == splitter) {
      int placeHolderIndex = splitter->indexOf(m_placeHolder);
      if (indexUnderMouse > placeHolderIndex) {
        indexUnderMouse = placeHolderIndex;
      }
    }
    if(indexUnderMouse >= 0) {
      if (currentIndex == m_dropCurrentSuggestionIndex) {
        //hidePlaceHolder();
        QSize placeHolderSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        if (splitter->orientation() == Qt::Horizontal) {
          placeHolderSize.setWidth(m_borderSensitivity / 2);
        } else {
          placeHolderSize.setHeight(m_borderSensitivity / 2);
        }
        m_placeHolder->setFixedSize(placeHolderSize);
        splitter->insertWidget(indexUnderMouse, m_placeHolder);
        m_placeHolder->show();
        foundPlace = true;
        break;
      } else {
        currentIndex++;
      }
    }
  }
  if (currentIndex > 0 && !foundPlace) {
    m_dropCurrentSuggestionIndex = -1;
    dropSuggestionSwitchTimeout();
  }
  if (currentIndex == 0 && !foundPlace && m_placeHolder->isVisible()) {
    hidePlaceHolder();
  }
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
    if (!toolWindow || !m_toolWindows.contains(toolWindow)) {
      return false;
    }
    QMouseEvent* releaseEvent = new QMouseEvent(QEvent::MouseButtonRelease,
                                                static_cast<QMouseEvent*>(event)->localPos(),
                                                Qt::LeftButton, Qt::LeftButton, 0);
    qApp->sendEvent(tabBar, releaseEvent);
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::red);
    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData();
    mimeData->setData(m_dragMimeType, QByteArray::number(m_toolWindows.indexOf(toolWindow)));
    drag->setMimeData(mimeData);
    drag->setPixmap(generateDragPixmap(toolWindow));
    drag->exec(Qt::MoveAction);
  }
  return false;
}

void ToolWindowManager::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->formats().contains(m_dragMimeType)) {
    event->accept();
  }
}

void ToolWindowManager::dragMoveEvent(QDragMoveEvent *event) {
  m_dropGlobalPos = mapToGlobal(event->pos());
  /*if(m_placeHolder->isVisible()) {
    QRect placeHolderRect = m_placeHolder->rect();
    placeHolderRect.adjust(-m_borderSensitivity, -m_borderSensitivity,
                            m_borderSensitivity,  m_borderSensitivity);
    if (!placeHolderRect.contains(m_placeHolder->mapFromGlobal(m_dropGlobalPos))) {
      hidePlaceHolder();
    }
  } else {*/
  m_dropCurrentSuggestionIndex = -1;
  dropSuggestionSwitchTimeout();
  if (m_placeHolder->isVisible()) {
    if (m_dropSuggestionSwitchTimer.isActive()) {
      m_dropSuggestionSwitchTimer.stop();
    }
    m_dropSuggestionSwitchTimer.start();
  }
  //}
  event->setAccepted(m_placeHolder->isVisible());

}

void ToolWindowManager::dragLeaveEvent(QDragLeaveEvent *event) {
  if (!rect().contains(mapFromGlobal(QCursor::pos()))) {
    hidePlaceHolder();
  }
  Q_UNUSED(event);
}


void ToolWindowManager::dropEvent(QDropEvent *event) {
  if (!m_placeHolder->isVisible()) {
    qWarning("unexpected drop event");
    return;
  }
  QSplitter* splitter = qobject_cast<QSplitter*>(m_placeHolder->parentWidget());
  if (!splitter) {
    qWarning("invalid parent for splitter");
    return;
  }
  bool ok = false;
  int toolWindowIndex = event->mimeData()->data(m_dragMimeType).toInt(&ok);
  if (!ok || toolWindowIndex < 0 || toolWindowIndex >= m_toolWindows.count()) {
    qWarning("invalid index in mime data");
    return;
  }
  QWidget* toolWindow = m_toolWindows[toolWindowIndex];
  QTabWidget* previousTabWidget = findClosestParent<QTabWidget*>(toolWindow);
  if (!previousTabWidget) {
    qWarning("cannot find tab widget for tool window");
    return;
  }
  previousTabWidget->removeTab(previousTabWidget->indexOf(toolWindow));
  deleteEmptyItems(previousTabWidget);

  int index = splitter->indexOf(m_placeHolder);
  hidePlaceHolder();
  splitter->insertWidget(index, createDockItem(toolWindow, splitter->orientation()));

  Q_UNUSED(event);
}


