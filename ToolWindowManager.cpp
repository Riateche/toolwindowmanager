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
  setAcceptDrops(true);

  connect(&m_dropSuggestionSwitchTimer, SIGNAL(timeout()),
          this, SLOT(dropSuggestionSwitchTimeout()));
  m_dropSuggestionSwitchTimer.setInterval(1000); //todo: add setter for this interval
  m_dropCurrentSuggestionIndex = -1;

  m_rectPlaceHolder = new QRubberBand(QRubberBand::Rectangle, this);
  m_linePlaceHolder = new QRubberBand(QRubberBand::Line, this);
  m_suggestedSplitter = 0;
  m_suggestedIndexInSplitter = -1;
}

ToolWindowManager::~ToolWindowManager() {
  QSet<QTabWidget*> tabWidgets;
  foreach(QWidget* toolWindow, m_toolWindows) {
    QTabWidget* tabWidget = findClosestParent<QTabWidget*>(toolWindow);
    if (!tabWidget) {
      qWarning("cannot find tab widget for tool window");
    } else {
      tabWidgets << tabWidget;
    }
  }
  foreach(QTabWidget* tabWidget, tabWidgets) {
    delete tabWidget;
  }
}

void ToolWindowManager::setCentralWidget(QWidget *widget) {
  if(m_centralWidget) {
    m_centralWidget->deleteLater();
  }
  m_centralWidget = widget;
  m_centralWidgetContainer->layout()->addWidget(widget);

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
  QTabWidget* tabWidget = createTabWidget();
  tabWidget->addTab(toolWindow, toolWindow->windowIcon(), toolWindow->windowTitle());
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
  tabWidget->tabBar()->setAcceptDrops(true);
  m_hash_tabbar_to_tabwidget[tabWidget->tabBar()] = tabWidget;
  return tabWidget;
  //todo: clear m_hash_tabbar_to_tabwidget when widgets are destroyed

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

QPixmap ToolWindowManager::generateDragPixmap(QWidget* toolWindow) {
  QTabBar widget;
  widget.addTab(toolWindow->windowIcon(), toolWindow->windowTitle());
  return widget.grab();
}

void ToolWindowManager::dropSuggestionSwitchTimeout() {
  m_dropCurrentSuggestionIndex++;
  int currentIndex = 0;
  bool foundPlace = false;
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
            placeHolderGeometry.setLeft(splitter->width() - m_borderSensitivity);
          } else {
            placeHolderGeometry.setLeft(
                  m_suggestedSplitter->widget(m_suggestedIndexInSplitter)->geometry().left() -
                  m_borderSensitivity / 2);
          }
          placeHolderGeometry.setWidth(m_borderSensitivity);
        } else {
          placeHolderGeometry.setLeft(0);
          placeHolderGeometry.setRight(splitter->width());
          if (m_suggestedIndexInSplitter == 0) {
            placeHolderGeometry.setTop(0);
          } else if (m_suggestedIndexInSplitter == m_suggestedSplitter->count()) {
            placeHolderGeometry.setTop(splitter->height() - m_borderSensitivity);
          } else {
            placeHolderGeometry.setTop(
                  m_suggestedSplitter->widget(m_suggestedIndexInSplitter)->geometry().top() -
                  m_borderSensitivity / 2);
          }
          placeHolderGeometry.setHeight(m_borderSensitivity);
        }
        placeHolderGeometry.moveTopLeft(
              m_suggestedSplitter->mapTo(this, placeHolderGeometry.topLeft()));
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

bool ToolWindowManager::eventFilter(QObject *object, QEvent *event) {
  QTabBar* tabBar = qobject_cast<QTabBar*>(object);
  if (tabBar) {
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
      QPixmap pixmap(16, 16);
      pixmap.fill(Qt::red);
      QDrag* drag = new QDrag(this);
      QMimeData* mimeData = new QMimeData();
      mimeData->setData(m_dragMimeType, QByteArray::number(m_toolWindows.indexOf(toolWindow)));
      drag->setMimeData(mimeData);
      drag->setPixmap(generateDragPixmap(toolWindow));
      Qt::DropAction dropAction = drag->exec(Qt::MoveAction);
      if (dropAction == Qt::IgnoreAction) {
        releaseToolWindow(toolWindow);
        QTabWidget* newTabWidget = createTabWidget();
        newTabWidget->addTab(toolWindow, toolWindow->windowIcon(), toolWindow->windowTitle());
        newTabWidget->move(QCursor::pos());
      }
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
      bool ok = false;
      int toolWindowIndex = static_cast<QDropEvent*>(event)->mimeData()->data(m_dragMimeType).toInt(&ok);
      if (!ok || toolWindowIndex < 0 || toolWindowIndex >= m_toolWindows.count()) {
        qWarning("invalid index in mime data");
        return false;
      }
      QWidget* toolWindow = m_toolWindows[toolWindowIndex];
      releaseToolWindow(toolWindow);
      hidePlaceHolder();
      tabWidget->addTab(toolWindow, toolWindow->windowIcon(), toolWindow->windowTitle());
      static_cast<QDropEvent*>(event)->acceptProposedAction();
    }
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
  m_dropCurrentSuggestionIndex = -1;
  dropSuggestionSwitchTimeout();
  if (m_suggestedSplitter) {
    if (m_dropSuggestionSwitchTimer.isActive()) {
      m_dropSuggestionSwitchTimer.stop();
    }
    m_dropSuggestionSwitchTimer.start();
  }
  event->setAccepted(m_suggestedSplitter != 0);

}

void ToolWindowManager::dragLeaveEvent(QDragLeaveEvent *event) {
  if (!rect().contains(mapFromGlobal(QCursor::pos()))) {
    hidePlaceHolder();
  }
  Q_UNUSED(event);
}


void ToolWindowManager::dropEvent(QDropEvent *event) {
  if (!m_suggestedSplitter) {
    qWarning("unexpected drop event");
    return;
  }
  bool ok = false;
  int toolWindowIndex = event->mimeData()->data(m_dragMimeType).toInt(&ok);
  if (!ok || toolWindowIndex < 0 || toolWindowIndex >= m_toolWindows.count()) {
    qWarning("invalid index in mime data");
    return;
  }
  QWidget* toolWindow = m_toolWindows[toolWindowIndex];
  releaseToolWindow(toolWindow);

  m_suggestedSplitter->insertWidget(m_suggestedIndexInSplitter,
                                    createDockItem(toolWindow, m_suggestedSplitter->orientation()));
  hidePlaceHolder();
  event->acceptProposedAction();

  Q_UNUSED(event);
}
