#include "ToolWindowManager.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QEvent>
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>

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
  m_borderSensitivity = 10;
  m_linePlaceHolderWidth = 4;
  m_dragMimeType = "application/x-tool-window-ids";
  m_tabWidgetDragCanStart = false;

  m_centralWidget = 0;
  QSplitter* mainSplitter1 = createSplitter();
  mainSplitter1->installEventFilter(this);
  mainSplitter1->setAcceptDrops(true);
  mainSplitter1->setOrientation(Qt::Horizontal);
  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(mainSplitter1);
  mainLayout->setContentsMargins(0, 0, 0, 0);

  QSplitter* mainSplitter2 = createSplitter();
  mainSplitter2->setOrientation(Qt::Vertical);
  mainSplitter1->addWidget(mainSplitter2);
  m_centralWidgetContainer = new QWidget();
  mainSplitter2->addWidget(m_centralWidgetContainer);
  QVBoxLayout* layout = new QVBoxLayout(m_centralWidgetContainer);
  layout->setContentsMargins(0, 0, 0, 0);

  connect(&m_dropSuggestionSwitchTimer, SIGNAL(timeout()),
          this, SLOT(dropSuggestionSwitchTimeout()));
  m_dropSuggestionSwitchTimer.setInterval(1000);
  m_dropCurrentSuggestionIndex = -1;

  m_rectPlaceHolder = new QRubberBand(QRubberBand::Rectangle, this);
  m_linePlaceHolder = new QRubberBand(QRubberBand::Line, this);
  m_suggestedSplitter = 0;
  m_suggestedIndexInSplitter = -1;

}

ToolWindowManager::~ToolWindowManager() {
  QSet<QWidget*> floatingWindows;
  foreach(QWidget* toolWindow, m_toolWindows) {
    QWidget* window = toolWindow->topLevelWidget();
    if (window != topLevelWidget()) {
      floatingWindows << window;
    }
  }
  foreach(QWidget* window, floatingWindows) {
    delete window;
  }
}

void ToolWindowManager::setCentralWidget(QWidget *widget) {
  if(m_centralWidget) {
    m_centralWidget->deleteLater();
  }
  m_centralWidget = widget;
  if (widget) {
    m_centralWidgetContainer->layout()->addWidget(widget);
  }
  //m_centralWidgetContainer->setFixedWidth(widget ? QWIDGETSIZE_MAX : m_borderSensitivity);

}

void ToolWindowManager::addToolWindow(QWidget *toolWindow) {
  if (!toolWindow) {
    qWarning("cannot add null widget");
    return;
  }
  if (m_toolWindows.contains(toolWindow)) {
    qWarning("this tool window has already been added");
    return;
  }
  m_toolWindows << toolWindow;
  setToolWindowVisible(toolWindow, true);
}

void ToolWindowManager::setToolWindowVisible(QWidget *toolWindow, bool visible) {
  if (!m_toolWindows.contains(toolWindow)) {
    qWarning("unknown tool window");
    return;
  }
  if (visible == (toolWindow->parentWidget() != 0)) {
    return;
  }
  if (visible) {
    QWidget* widget = createDockItem(QList<QWidget*>() << toolWindow, 0);
    widget->show();
  } else {
    releaseToolWindow(toolWindow);
  }
  emit toolWindowVisibilityChanged(toolWindow, visible);
}

void ToolWindowManager::setSuggestionSwitchInterval(int msec) {
  m_dropSuggestionSwitchTimer.setInterval(msec);
}

int ToolWindowManager::suggestionSwitchInterval() {
  return m_dropSuggestionSwitchTimer.interval();
}

void ToolWindowManager::setDragMimeType(const QString &mimeType) {
  m_dragMimeType = mimeType;
}

void ToolWindowManager::setBorderSensitivity(int pixels) {
  m_borderSensitivity = pixels;
}

QWidget *ToolWindowManager::createDockItem(const QList<QWidget *> &toolWindows,
                                           Qt::Orientations parentOrientation) {
  QSplitter* splitter = createSplitter();
  QSplitter* topSplitter;
  if (parentOrientation == 0) {
    topSplitter = createSplitter();
    topSplitter->setOrientation(Qt::Horizontal);
    splitter->setOrientation(Qt::Vertical);
    topSplitter->addWidget(splitter);
    topSplitter->installEventFilter(this);
    topSplitter->setAcceptDrops(true);
  } else {
    splitter->setOrientation(parentOrientation == Qt::Horizontal ? Qt::Vertical : Qt::Horizontal);
    topSplitter = splitter;
  }
  QTabWidget* tabWidget = createTabWidget();
  foreach(QWidget* toolWindow, toolWindows) {
    tabWidget->addTab(toolWindow, toolWindow->windowIcon(), toolWindow->windowTitle());
  }
  splitter->addWidget(tabWidget);
  return topSplitter;
}

QTabWidget *ToolWindowManager::createTabWidget() {
  QTabWidget* tabWidget = new QTabWidget();
  tabWidget->setWindowFlags(tabWidget->windowFlags() & Qt::Tool);
  tabWidget->setMovable(true);
  tabWidget->setTabsClosable(true);
  connect(tabWidget, SIGNAL(tabCloseRequested(int)),
          this, SLOT(tabCloseRequested(int)));
  tabWidget->show();
  tabWidget->tabBar()->installEventFilter(this);
  tabWidget->tabBar()->setAcceptDrops(true);
  connect(tabWidget->tabBar(), SIGNAL(destroyed(QObject*)),
          this, SLOT(tabBarDestroyed(QObject*)));
  tabWidget->installEventFilter(this);
  m_hash_tabbar_to_tabwidget[tabWidget->tabBar()] = tabWidget;
  return tabWidget;

}


void ToolWindowManager::hidePlaceHolder() {
  m_rectPlaceHolder->hide();
  m_linePlaceHolder->hide();
  m_linePlaceHolder->setParent(this);
  m_rectPlaceHolder->setParent(this);
  m_suggestedSplitter = 0;
  m_suggestedIndexInSplitter = -1;
  if (m_dropSuggestionSwitchTimer.isActive()) {
    m_dropSuggestionSwitchTimer.stop();
  }
}

void ToolWindowManager::releaseToolWindow(QWidget *toolWindow) {
  QTabWidget* previousTabWidget = findClosestParent<QTabWidget*>(toolWindow);
  if (!previousTabWidget) {
    qWarning("cannot find tab widget for tool window");
    return;
  }
  previousTabWidget->removeTab(previousTabWidget->indexOf(toolWindow));
  toolWindow->setParent(0);
  toolWindow->hide();
  if (previousTabWidget->count() == 0 && m_rectPlaceHolder->parent() != previousTabWidget) {
    previousTabWidget->deleteLater();
    QSplitter* splitter = qobject_cast<QSplitter*>(previousTabWidget->parentWidget());
    while(splitter) {
      if (splitter->count() == 1 && splitter != m_suggestedSplitter) {
        splitter->deleteLater();
        splitter = qobject_cast<QSplitter*>(splitter->parentWidget());
      } else {
        break;
      }
    }
  }
}

void ToolWindowManager::execDrag(const QList<QWidget *> &toolWindows) {
  QStringList ids;
  foreach(QWidget* toolWindow, toolWindows) {
    ids << QString::number(m_toolWindows.indexOf(toolWindow));
  }
  QDrag* drag = new QDrag(this);
  QMimeData* mimeData = new QMimeData();
  mimeData->setData(m_dragMimeType, ids.join(";").toLatin1());
  drag->setMimeData(mimeData);
  drag->setPixmap(generateDragPixmap(toolWindows));
  Qt::DropAction dropAction = drag->exec(Qt::MoveAction);
  if (dropAction == Qt::IgnoreAction) {
    foreach(QWidget* toolWindow, toolWindows) {
      releaseToolWindow(toolWindow);
    }
    QWidget* widget = createDockItem(toolWindows, 0);
    widget->move(QCursor::pos());
    widget->show();
  }
}


QPixmap ToolWindowManager::generateDragPixmap(const QList<QWidget *> &toolWindows) {
  QTabBar widget;
  foreach(QWidget* toolWindow, toolWindows) {
    widget.addTab(toolWindow->windowIcon(), toolWindow->windowTitle());
  }
  return widget.grab();
}

void ToolWindowManager::dropSuggestionSwitchTimeout() {
  m_dropCurrentSuggestionIndex++;
  int currentIndex = 0;
  bool foundPlace = false;
  QList<QSplitter*> splitters = m_dragParent->findChildren<QSplitter*>();
  if (qobject_cast<QSplitter*>(m_dragParent)) {
    splitters.prepend(static_cast<QSplitter*>(m_dragParent));
  }
  foreach(QSplitter* splitter, splitters) {
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
    if(indexUnderMouse >= 0) {
      if (currentIndex == m_dropCurrentSuggestionIndex) {
        m_suggestedSplitter = splitter;
        m_suggestedIndexInSplitter = indexUnderMouse;
        QRect placeHolderGeometry;
        if (splitter->orientation() == Qt::Horizontal) {
          placeHolderGeometry.setTop(0);
          placeHolderGeometry.setBottom(splitter->height());
          if (m_suggestedIndexInSplitter == 0) {
            placeHolderGeometry.setLeft(0);
          } else if (m_suggestedIndexInSplitter == m_suggestedSplitter->count()) {
            placeHolderGeometry.setLeft(splitter->width() - m_linePlaceHolderWidth);
          } else {
            placeHolderGeometry.setLeft(
                  m_suggestedSplitter->widget(m_suggestedIndexInSplitter)->geometry().left() -
                  m_linePlaceHolderWidth / 2);
          }
          placeHolderGeometry.setWidth(m_linePlaceHolderWidth);
        } else {
          placeHolderGeometry.setLeft(0);
          placeHolderGeometry.setRight(splitter->width());
          if (m_suggestedIndexInSplitter == 0) {
            placeHolderGeometry.setTop(0);
          } else if (m_suggestedIndexInSplitter == m_suggestedSplitter->count()) {
            placeHolderGeometry.setTop(splitter->height() - m_linePlaceHolderWidth);
          } else {
            placeHolderGeometry.setTop(
                  m_suggestedSplitter->widget(m_suggestedIndexInSplitter)->geometry().top() -
                  m_linePlaceHolderWidth / 2);
          }
          placeHolderGeometry.setHeight(m_linePlaceHolderWidth);
        }
        placeHolderGeometry.moveTopLeft(
              m_suggestedSplitter->mapToGlobal(placeHolderGeometry.topLeft()));
        m_linePlaceHolder->setParent(0);
        m_linePlaceHolder->setGeometry(placeHolderGeometry);
        m_linePlaceHolder->show();
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
  if (currentIndex == 0 && !foundPlace && m_suggestedSplitter) {
    hidePlaceHolder();
  }
}

void ToolWindowManager::tabCloseRequested(int index) {
  QTabWidget* tabWidget = static_cast<QTabWidget*>(sender());
  QWidget* toolWindow = tabWidget->widget(index);
  if (!m_toolWindows.contains(toolWindow)) {
    qWarning("unknown tab in tab widget");
    return;
  }
  hideToolWindow(toolWindow);
}

void ToolWindowManager::tabBarDestroyed(QObject *object) {
  m_hash_tabbar_to_tabwidget.remove(static_cast<QTabBar*>(object));
}

bool ToolWindowManager::eventFilter(QObject *object, QEvent *event) {
  QTabBar* tabBar = qobject_cast<QTabBar*>(object);
  if (tabBar) {
    return tabBarEventFilter(tabBar, event);
  }
  QTabWidget* tabWidget = qobject_cast<QTabWidget*>(object);
  if (tabWidget) {
    return tabWidgetEventFilter(tabWidget, event);
  }
  QSplitter* topSplitter = qobject_cast<QSplitter*>(object);
  if (topSplitter) {
    return topSplitterEventFilter(topSplitter, event);
  }
  return false;
}

bool ToolWindowManager::tabBarEventFilter(QTabBar *tabBar, QEvent *event) {
  QTabWidget* tabWidget = m_hash_tabbar_to_tabwidget[tabBar];
  if (!tabWidget) { return false; }
  if (event->type() == QEvent::MouseMove) {
    if (tabBar->rect().contains(static_cast<QMouseEvent*>(event)->pos())) {
      return false;
    }
    if (!(qApp->mouseButtons() & Qt::LeftButton)) {
      return false;
    }
    QWidget* toolWindow = tabWidget->currentWidget();
    if (!toolWindow || !m_toolWindows.contains(toolWindow)) {
      return false;
    }
    QMouseEvent* releaseEvent = new QMouseEvent(QEvent::MouseButtonRelease,
                                                static_cast<QMouseEvent*>(event)->localPos(),
                                                Qt::LeftButton, Qt::LeftButton, 0);
    qApp->sendEvent(tabBar, releaseEvent);
    execDrag(QList<QWidget*>() << toolWindow);
  } else if (event->type() == QEvent::DragEnter) {
    if (static_cast<QDragEnterEvent*>(event)->mimeData()->formats().contains(m_dragMimeType)) {
      event->accept();
      hidePlaceHolder();
    }
  } else if (event->type() == QEvent::DragMove) {
    event->accept();
    m_rectPlaceHolder->setGeometry(tabWidget->rect());
    m_rectPlaceHolder->setParent(tabWidget);
    m_rectPlaceHolder->show();
  } else if (event->type() == QEvent::DragLeave) {
    hidePlaceHolder();
  } else if (event->type() == QEvent::Drop) {
    QByteArray data = static_cast<QDropEvent*>(event)->mimeData()->data(m_dragMimeType);
    foreach(QByteArray dataItem, data.split(';')) {
      bool ok = false;
      int toolWindowIndex = dataItem.toInt(&ok);
      if (!ok || toolWindowIndex < 0 || toolWindowIndex >= m_toolWindows.count()) {
        qWarning("invalid index in mime data");
        return false;
      }
      QWidget* toolWindow = m_toolWindows[toolWindowIndex];
      releaseToolWindow(toolWindow);
      tabWidget->addTab(toolWindow, toolWindow->windowIcon(), toolWindow->windowTitle());
    }
    hidePlaceHolder();
    static_cast<QDropEvent*>(event)->acceptProposedAction();
  }
  return false;
}

bool ToolWindowManager::tabWidgetEventFilter(QTabWidget *tabWidget, QEvent *event) {
  if (event->type() == QEvent::MouseButtonPress && qApp->mouseButtons() & Qt::LeftButton) {
    m_tabWidgetDragCanStart = true;
  } else if (event->type() == QEvent::MouseButtonRelease) {
    m_tabWidgetDragCanStart = false;
  } else if (event->type() == QEvent::MouseMove &&
      qApp->mouseButtons() & Qt::LeftButton &&
      !tabWidget->rect().contains(tabWidget->mapFromGlobal(QCursor::pos())) &&
      m_tabWidgetDragCanStart) {
    m_tabWidgetDragCanStart = false;
    QList<QWidget*> toolWindows;
    for(int i = 0; i < tabWidget->count(); i++) {
      QWidget* toolWindow = tabWidget->widget(i);
      if (!m_toolWindows.contains(toolWindow)) {
        qWarning("tab widget contains unmanaged widget");
      } else {
        toolWindows << toolWindow;
      }
    }
    execDrag(toolWindows);
  }
  return false;
}

bool ToolWindowManager::topSplitterEventFilter(QSplitter *topSplitter, QEvent *event) {
  if (event->type() == QEvent::DragEnter) {
    if (static_cast<QDragEnterEvent*>(event)->mimeData()->formats().contains(m_dragMimeType)) {
      event->accept();
    }
  } else if (event->type() == QEvent::DragMove) {
    m_dragParent = topSplitter;
    m_dropGlobalPos = QCursor::pos();
    m_dropCurrentSuggestionIndex = -1;
    dropSuggestionSwitchTimeout();
    if (m_suggestedSplitter) {
      if (m_dropSuggestionSwitchTimer.isActive()) {
        m_dropSuggestionSwitchTimer.stop();
      }
      m_dropSuggestionSwitchTimer.start();
    }
    event->setAccepted(m_suggestedSplitter != 0);
  } else if (event->type() == QEvent::DragLeave) {
    if (!topSplitter->rect().contains(topSplitter->mapFromGlobal(QCursor::pos()))) {
      hidePlaceHolder();
    }
  } else if (event->type() == QEvent::Drop) {
    if (!m_suggestedSplitter) {
      qWarning("unexpected drop event");
      return false;
    }
    QByteArray data = static_cast<QDropEvent*>(event)->mimeData()->data(m_dragMimeType);
    QList<QWidget*> toolWindows;
    foreach(QByteArray dataItem, data.split(';')) {
      bool ok = false;
      int toolWindowIndex = dataItem.toInt(&ok);
      if (!ok || toolWindowIndex < 0 || toolWindowIndex >= m_toolWindows.count()) {
        qWarning("invalid index in mime data");
        return false;
      }
      QWidget* toolWindow = m_toolWindows[toolWindowIndex];
      releaseToolWindow(toolWindow);
      toolWindows << toolWindow;
    }
    m_suggestedSplitter->insertWidget(m_suggestedIndexInSplitter,
      createDockItem(toolWindows, m_suggestedSplitter->orientation()));
    hidePlaceHolder();
    static_cast<QDropEvent*>(event)->acceptProposedAction();
  } else if (event->type() == QEvent::Close && topSplitter->topLevelWidget() != topLevelWidget()) {
    foreach(QTabWidget* tabWidget, topSplitter->findChildren<QTabWidget*>()) {
      while(tabWidget->count() > 0) {
        QWidget* toolWindow = tabWidget->widget(0);
        hideToolWindow(toolWindow);
      }
    }
  }
  return false;
}

QSplitter *ToolWindowManager::createSplitter() {
  QSplitter* splitter = new QSplitter();
  splitter->setChildrenCollapsible(false);
  return splitter;
}


