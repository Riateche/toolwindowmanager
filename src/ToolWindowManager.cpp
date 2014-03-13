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
  m_lastUsedArea = 0;
  m_borderSensitivity = 12;
  m_rubberBandLineWidth = 4;
  m_dragMimeType = "application/x-tool-window-ids";
  m_tabWidgetDragCanStart = false;
  m_emptySpacer = 0;
  m_dragParent = 0;
  m_rootSplitter = createSplitter();
  setupTopLevelSplitter(m_rootSplitter);
  m_rootSplitter->setOrientation(Qt::Horizontal);
  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(m_rootSplitter);
  mainLayout->setContentsMargins(0, 0, 0, 0);

  m_subRootSplitter = createSplitter();
  m_subRootSplitter->setOrientation(Qt::Vertical);
  m_rootSplitter->addWidget(m_subRootSplitter);

  connect(&m_dropSuggestionSwitchTimer, SIGNAL(timeout()),
          this, SLOT(dropSuggestionSwitchTimeout()));
  m_dropSuggestionSwitchTimer.setInterval(1000);
  m_dropCurrentSuggestionIndex = -1;

  m_rectPlaceHolder = new QRubberBand(QRubberBand::Rectangle, this);
  m_linePlaceHolder = new QRubberBand(QRubberBand::Line, this);
  m_suggestedReceiver = 0;
  m_suggestedIndexInSplitter = -1;

  updateEmptySpacer();

}

ToolWindowManager::~ToolWindowManager() {
}

void ToolWindowManager::addToolWindow(QWidget *toolWindow, DockArea dockArea) {
  if (!toolWindow) {
    qWarning("cannot add null widget");
    return;
  }
  if (m_toolWindows.contains(toolWindow)) {
    qWarning("this tool window has already been added");
    return;
  }
  m_toolWindows << toolWindow;
  toolWindow->hide();
  toolWindow->setParent(0);
  moveToolWindow(toolWindow, dockArea);
}

void ToolWindowManager::moveToolWindow(QWidget *toolWindow, ToolWindowManager::DockArea dockArea) {
  if (!m_toolWindows.contains(toolWindow)) {
    qWarning("unknown tool window");
    return;
  }
  if (toolWindow->parentWidget() != 0) {
    releaseToolWindow(toolWindow);
  }
  if (dockArea == NoArea) {
    deleteAllEmptyItems();
    emit toolWindowVisibilityChanged(toolWindow, false);
    return;
  }
  if (m_rootSplitter->count() == 0) {
    qWarning("unexpected empty splitter");
  }
  QTabWidget* tabWidget = 0;
  QSplitter* parentSplitter = 0;
  bool atEnd = false;
  bool atCenter = false;
  if (dockArea == LastUsedArea && !m_lastUsedArea) {
    dockArea = LeftDockArea;
  }
  if (dockArea == LeftDockArea || dockArea == RightDockArea) {
    atEnd = dockArea == RightDockArea;
    parentSplitter = m_rootSplitter;
  }
  if (dockArea == TopDockArea || dockArea == BottomDockArea) {
    atEnd = dockArea == BottomDockArea;
    parentSplitter = m_subRootSplitter;
  }
  if (dockArea == CentralDockArea) {
    if (m_emptySpacer) {
      tabWidget = m_emptySpacer;
    } else {
      atCenter = true;
      parentSplitter = m_subRootSplitter;
    }
  }
  if (parentSplitter) {
    int index = atEnd ? parentSplitter->count() - 1 : atCenter ? parentSplitter->count() / 2 : 0;
    if (parentSplitter->count() > (atEnd ? 1 : 0)) {
      QWidget* widget = parentSplitter->widget(index);
      if (qobject_cast<QTabWidget*>(widget)) {
        tabWidget = static_cast<QTabWidget*>(widget);
      } else {
        tabWidget = widget->findChild<QTabWidget*>();
      }
    }
    if (!tabWidget) {
      QWidget* widget = createDockItem(QList<QWidget*>() << toolWindow, parentSplitter->orientation());
      int newIndex = atEnd ? parentSplitter->count() : atCenter ? parentSplitter->count() / 2 : 0;
      parentSplitter->insertWidget(newIndex, widget);
      addMissingSplitters(parentSplitter);
    }
  } else if (dockArea == LastUsedArea) {
    tabWidget = m_lastUsedArea;
  } else if (dockArea == NewFloatingArea) {
    QSplitter* splitter = createDockItem(QList<QWidget*>() << toolWindow, 0);
    wrapTopLevelSplitter(splitter)->show();

  } else {
    qWarning("unknown area");
  }
  if (tabWidget) {
    int newIndex = tabWidget->addTab(toolWindow, toolWindow->windowIcon(), toolWindow->windowTitle());
    tabWidget->setCurrentIndex(newIndex);
  }
  deleteAllEmptyItems();
  emit toolWindowVisibilityChanged(toolWindow, true);
}

void ToolWindowManager::removeToolWindow(QWidget *toolWindow) {
  if (!m_toolWindows.contains(toolWindow)) {
    qWarning("unknown tool window");
    return;
  }
  if (toolWindow->parentWidget() != 0) {
    releaseToolWindow(toolWindow);
  }
  m_toolWindows.removeOne(toolWindow);
  deleteAllEmptyItems();
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

void ToolWindowManager::setRubberBandLineWidth(int pixels) {
  m_rubberBandLineWidth = pixels;
}

QVariant ToolWindowManager::saveState() {
  QVariantMap result;
  result["toolWindowManagerStateFormat"] = 1;
  result["rootSplitter"] = saveSplitterState(m_rootSplitter);
  QVariantList floatingWindowsData;
  foreach(QWidget* window, floatingWindows()) {
    QSplitter* splitter = window->findChild<QSplitter*>();
    if (!splitter) {
      qWarning("invalid floating window found");
      continue;
    }
    floatingWindowsData << saveSplitterState(splitter);
  }
  result["floatingWindows"] = floatingWindowsData;
  return result;
}

void ToolWindowManager::restoreState(const QVariant &data) {
  if (!data.isValid()) { return; }
  QVariantMap dataMap = data.toMap();
  if (dataMap["toolWindowManagerStateFormat"].toInt() != 1) {
    qWarning("state format is not recognized");
    return;
  }
  foreach(QWidget* toolWindow, m_toolWindows) {
    if (toolWindow->parentWidget() != 0) {
      releaseToolWindow(toolWindow);
    }
  }
  deleteAllEmptyItems();
  m_subRootSplitter = 0;
  m_emptySpacer = 0;
  delete m_rootSplitter;
  m_rootSplitter = restoreSplitterState(dataMap["rootSplitter"].toMap());
  if (!m_rootSplitter) {
    m_rootSplitter = createSplitter();
    m_rootSplitter->setOrientation(Qt::Horizontal);
  }
  if (!m_subRootSplitter) {
    QList<QSplitter*> candidates;
    for(int i = 0; i < m_rootSplitter->count(); i++) {
      QSplitter* candidate = qobject_cast<QSplitter*>(m_rootSplitter->widget(i));
      if (candidate && candidate->orientation() == Qt::Vertical) {
        candidates << candidate;
      }
    }
    if (!candidates.isEmpty()) {
      m_subRootSplitter = candidates[candidates.count() / 2];
    } else {
      m_subRootSplitter = new QSplitter();
      m_subRootSplitter->setOrientation(Qt::Vertical);
      m_rootSplitter->insertWidget(m_rootSplitter->count() / 2, m_subRootSplitter);
    }
  }
  setupTopLevelSplitter(m_rootSplitter);
  layout()->addWidget(m_rootSplitter);
  foreach(QVariant windowData, dataMap["floatingWindows"].toList()) {
    QSplitter* splitter = restoreSplitterState(windowData.toMap());
    if (splitter) {
      setupTopLevelSplitter(splitter);
      QWidget* topLevelWidget = wrapTopLevelSplitter(splitter);
      topLevelWidget->restoreGeometry(windowData.toMap()["geometry"].toByteArray());
      topLevelWidget->show();
    }
  }
  deleteAllEmptyItems();
  foreach(QWidget* toolWindow, m_toolWindows) {
    emit toolWindowVisibilityChanged(toolWindow, toolWindow->parentWidget() != 0);
  }
}

QList<QWidget *> ToolWindowManager::floatingWindows() {
  QSet<QWidget*> result;
  foreach(QWidget* toolWindow, m_toolWindows) {
    if (toolWindow->parentWidget() != 0) {
      QWidget* window = toolWindow->topLevelWidget();
      if (window != topLevelWidget()) {
        result << window;
      }
    }
  }
  return result.toList();
}

QSplitter *ToolWindowManager::createDockItem(const QList<QWidget *> &toolWindows,
                                           Qt::Orientations parentOrientation) {
  QSplitter* splitter = createSplitter();
  QSplitter* topSplitter;
  if (parentOrientation == 0) {
    topSplitter = createSplitter();
    topSplitter->setOrientation(Qt::Horizontal);
    splitter->setOrientation(Qt::Vertical);
    topSplitter->addWidget(splitter);
    setupTopLevelSplitter(topSplitter);
  } else {
    splitter->setOrientation(parentOrientation == Qt::Horizontal ? Qt::Vertical : Qt::Horizontal);
    topSplitter = splitter;
  }
  QTabWidget* tabWidget = createTabWidget();
  foreach(QWidget* toolWindow, toolWindows) {
    tabWidget->addTab(toolWindow, toolWindow->windowIcon(), toolWindow->windowTitle());
    m_lastUsedArea = tabWidget;
  }
  splitter->addWidget(tabWidget);
  return topSplitter;
}

QTabWidget *ToolWindowManager::createTabWidget() {
  QTabWidget* tabWidget = new QTabWidget();
  tabWidget->setWindowFlags(tabWidget->windowFlags());
  tabWidget->setMovable(true);
  tabWidget->setTabsClosable(true);
  connect(tabWidget, SIGNAL(tabCloseRequested(int)),
          this, SLOT(tabCloseRequested(int)));
  tabWidget->installEventFilter(this); //for dragging the entire widget
  QTabBar* tabBar;
#if QT_VERSION >= 0x050000 // Qt5
  tabBar = tabWidget->tabBar();
#else //Qt4
  tabBar = tabWidget->findChild<QTabBar*>();
  if (!tabBar) {
    qWarning("failed to find tab bar");
    return tabWidget;
  }
#endif
  tabBar->installEventFilter(this); // for dragging a tab
  connect(tabBar, SIGNAL(destroyed(QObject*)),
          this, SLOT(tabBarDestroyed(QObject*)));
  m_hash_tabbar_to_tabwidget[tabBar] = tabWidget;
  return tabWidget;

}


void ToolWindowManager::hidePlaceHolder() {
  m_rectPlaceHolder->hide();
  m_linePlaceHolder->hide();
  m_linePlaceHolder->setParent(this);
  m_rectPlaceHolder->setParent(this);
  m_suggestedReceiver = 0;
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
  toolWindow->hide();
  toolWindow->setParent(0);

}

void ToolWindowManager::deleteAllEmptyItems() {
  foreach(QTabWidget* tabWidget, m_hash_tabbar_to_tabwidget) {
    deleteEmptyItems(tabWidget);
  }
  updateEmptySpacer();
}

void ToolWindowManager::deleteEmptyItems(QTabWidget *tabWidget) {
  if (tabWidget == m_emptySpacer) { return; }
  if (tabWidget->count() == 0) {
    tabWidget->deleteLater();
    QSplitter* splitter = qobject_cast<QSplitter*>(tabWidget->parentWidget());
    while(splitter) {
      if (splitter->count() == 1 &&
          splitter != m_rootSplitter &&
          splitter != m_subRootSplitter) {
        splitter->deleteLater();
        if (splitter->parentWidget() == splitter->topLevelWidget() &&
            splitter->parentWidget() != this) {
          splitter->parentWidget()->deleteLater();
        }
        splitter = qobject_cast<QSplitter*>(splitter->parentWidget());
      } else {
        break;
      }
    }
  }
}

QWidget *ToolWindowManager::wrapTopLevelSplitter(QSplitter *splitter) {
  QWidget* topLevelWidget = new QWidget(this, Qt::Tool);
  topLevelWidget->installEventFilter(this); // for close event
  QVBoxLayout* layout = new QVBoxLayout(topLevelWidget);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(splitter);
  return topLevelWidget;
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
    QSplitter* splitter = createDockItem(toolWindows, 0);
    QWidget* topLevelWidget = wrapTopLevelSplitter(splitter);
    topLevelWidget->move(QCursor::pos());
    topLevelWidget->show();
    deleteAllEmptyItems();
  }
}

QVariantMap ToolWindowManager::saveSplitterState(QSplitter *splitter) {
  QVariantMap result;
  result["state"] = splitter->saveState();
  result["type"] = "splitter";
  if (splitter == m_subRootSplitter) {
    result["subroot"] = true;
  }
  if (splitter->parentWidget() == splitter->topLevelWidget()) {
    result["geometry"] = splitter->topLevelWidget()->saveGeometry();
  }
  QVariantList items;
  for(int i = 0; i < splitter->count(); i++) {
    QWidget* item = splitter->widget(i);
    QVariantMap itemValue;
    QTabWidget* tabWidget = qobject_cast<QTabWidget*>(item);
    if (tabWidget) {
      itemValue["type"] = "tabWidget";
      itemValue["currentIndex"] = tabWidget->currentIndex();
      if (tabWidget == m_emptySpacer) {
        itemValue["emptySpacer"] = true;
      }
      QStringList objectNames;
      for(int i = 0; i < tabWidget->count(); i++) {
        QString name = tabWidget->widget(i)->objectName();
        if (name.isEmpty()) {
          qWarning("cannot save state of tool window without object name");
        } else {
          objectNames << name;
        }
      }
      itemValue["objectNames"] = objectNames;
    } else {
      QSplitter* childSplitter = qobject_cast<QSplitter*>(item);
      if (childSplitter) {
        itemValue = saveSplitterState(childSplitter);
      } else {
        qWarning("unknown splitter item");
      }
    }
    items << itemValue;
  }
  result["items"] = items;
  return result;
}

QSplitter *ToolWindowManager::restoreSplitterState(const QVariantMap &data) {
  if (data["items"].toList().isEmpty() && !data["subroot"].toBool()) {
    qWarning("empty splitter encountered");
    return 0;
  }
  QSplitter* splitter = createSplitter();
  foreach(QVariant itemData, data["items"].toList()) {
    QVariantMap itemValue = itemData.toMap();
    QString itemType = itemValue["type"].toString();
    if (itemType == "splitter") {
      QSplitter* newSplitter = restoreSplitterState(itemValue);
      if (newSplitter) {
        splitter->addWidget(newSplitter);
      }
    } else if (itemType == "tabWidget") {
      QTabWidget* tabWidget = createTabWidget();
      foreach(QVariant objectNameValue, itemValue["objectNames"].toList()) {
        QString objectName = objectNameValue.toString();
        if (objectName.isEmpty()) { continue; }
        bool found = false;
        foreach(QWidget* toolWindow, m_toolWindows) {
          if (toolWindow->objectName() == objectName) {
            tabWidget->addTab(toolWindow, toolWindow->windowIcon(), toolWindow->windowTitle());
            found = true;
            break;
          }
        }
        if (!found) {
          qWarning("tool window with name '%s' not found", objectName.toLocal8Bit().constData());
        }
      }
      tabWidget->setCurrentIndex(itemValue["currentIndex"].toInt());
      if (itemValue["emptySpacer"].toBool()) {
        m_emptySpacer = tabWidget;
        m_emptySpacer->setUpdatesEnabled(false);
      }
      splitter->addWidget(tabWidget);
    } else {
      qWarning("unknown item type");
    }
  }
  if (splitter->count() == 0) {
    delete splitter;
    return 0;
  }
  splitter->restoreState(data["state"].toByteArray());
  if (data["subroot"].toBool()) {
    m_subRootSplitter = splitter;
  }
  return splitter;
}

void ToolWindowManager::addMissingSplitters(QSplitter *splitter) {
  if (splitter->count() < 2) { return; }
  for(int i = 0; i < splitter->count(); i++) {
    QSplitter* childSplitter = qobject_cast<QSplitter*>(splitter->widget(i));
    if (!childSplitter || childSplitter->orientation() == splitter->orientation()) {
      QSplitter* newSplitter = createSplitter();
      newSplitter->setOrientation(splitter->orientation() == Qt::Horizontal ?
                                    Qt::Vertical : Qt::Horizontal);
      newSplitter->addWidget(splitter->widget(i));
      splitter->insertWidget(i, newSplitter);
    }
  }
}


QPixmap ToolWindowManager::generateDragPixmap(const QList<QWidget *> &toolWindows) {
  QTabBar widget;
  foreach(QWidget* toolWindow, toolWindows) {
    widget.addTab(toolWindow->windowIcon(), toolWindow->windowTitle());
  }
#if QT_VERSION >= 0x050000 // Qt5
  return widget.grab();
#else //Qt4
  return QPixmap::grabWidget(&widget);
#endif
}

void ToolWindowManager::dropSuggestionSwitchTimeout() {
  m_dropCurrentSuggestionIndex++;
  int currentIndex = 0, totalFound = 0;
  bool foundPlace = false;
  QList<QSplitter*> splitters = m_dragParent->findChildren<QSplitter*>();
  //if (qobject_cast<QSplitter*>(m_dragParent)) {
  splitters.prepend(m_dragParent);
  //}
  QWidget* topLevelWidget = m_dragParent->topLevelWidget();
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
      totalFound++;
      if (currentIndex == m_dropCurrentSuggestionIndex) {
        m_suggestedReceiver = splitter;
        m_suggestedIndexInSplitter = indexUnderMouse;
        QRect placeHolderGeometry;
        if (splitter->orientation() == Qt::Horizontal) {
          placeHolderGeometry.setTop(0);
          placeHolderGeometry.setBottom(splitter->height());
          if (m_suggestedIndexInSplitter == 0) {
            placeHolderGeometry.setLeft(0);
          } else if (m_suggestedIndexInSplitter == splitter->count()) {
            placeHolderGeometry.setLeft(splitter->width() - m_rubberBandLineWidth);
          } else {
            placeHolderGeometry.setLeft(
                  splitter->widget(m_suggestedIndexInSplitter)->geometry().left() -
                  m_rubberBandLineWidth / 2);
          }
          placeHolderGeometry.setWidth(m_rubberBandLineWidth);
        } else {
          placeHolderGeometry.setLeft(0);
          placeHolderGeometry.setRight(splitter->width());
          if (m_suggestedIndexInSplitter == 0) {
            placeHolderGeometry.setTop(0);
          } else if (m_suggestedIndexInSplitter == splitter->count()) {
            placeHolderGeometry.setTop(splitter->height() - m_rubberBandLineWidth);
          } else {
            placeHolderGeometry.setTop(
                  splitter->widget(m_suggestedIndexInSplitter)->geometry().top() -
                  m_rubberBandLineWidth / 2);
          }
          placeHolderGeometry.setHeight(m_rubberBandLineWidth);
        }
        placeHolderGeometry.moveTopLeft(
              splitter->mapTo(topLevelWidget,
                                         placeHolderGeometry.topLeft()));
        m_linePlaceHolder->setParent(topLevelWidget);
        m_linePlaceHolder->setGeometry(placeHolderGeometry);
        m_linePlaceHolder->show();
        m_rectPlaceHolder->hide();
        foundPlace = true;
        break;
      } else {
        currentIndex++;
      }
    }
  }
  if (!foundPlace) {
    foreach(QTabWidget* tabWidget, m_hash_tabbar_to_tabwidget) {
      if (tabWidget->topLevelWidget() != topLevelWidget) {
        continue;
      }
      if (tabWidget->rect().contains(tabWidget->mapFromGlobal(m_dropGlobalPos))) {
        totalFound++;
        if (currentIndex == m_dropCurrentSuggestionIndex) {
          m_suggestedReceiver = tabWidget;
          QRect placeHolderGeometry = tabWidget->rect();
          placeHolderGeometry.moveTopLeft(tabWidget->mapTo(topLevelWidget, placeHolderGeometry.topLeft()));
          m_rectPlaceHolder->setGeometry(placeHolderGeometry);
          m_rectPlaceHolder->setParent(topLevelWidget);
          m_rectPlaceHolder->show();
          m_linePlaceHolder->hide();
          foundPlace = true;
        }
        break; // only one tab widget can be under mouse
      }
    }
  }

  if (totalFound > 0 && !foundPlace) {
    m_dropCurrentSuggestionIndex = -1;
    dropSuggestionSwitchTimeout();
  }
  if (totalFound == 0 && !foundPlace) {
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
  QTabWidget* deletedTabWidget = m_hash_tabbar_to_tabwidget[static_cast<QTabBar*>(object)];
  if (deletedTabWidget == m_lastUsedArea) {
    m_lastUsedArea = 0;
  }
  m_hash_tabbar_to_tabwidget.remove(static_cast<QTabBar*>(object));
  if (m_emptySpacer == deletedTabWidget) {
    m_emptySpacer = 0;
  }
  updateEmptySpacer();
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
  if (event->type() == QEvent::Close) {
    foreach(QTabWidget* tabWidget, object->findChildren<QTabWidget*>()) {
      while(tabWidget->count() > 0) {
        QWidget* toolWindow = tabWidget->widget(0);
        hideToolWindow(toolWindow);
      }
    }
  }
  return false;
}

bool ToolWindowManager::tabBarEventFilter(QTabBar *tabBar, QEvent *event) {
  if (!m_hash_tabbar_to_tabwidget.contains(tabBar)) {
    qWarning("unknown tabbar");
    return false;
  }
  QTabWidget* tabWidget = m_hash_tabbar_to_tabwidget[tabBar];
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
                                                static_cast<QMouseEvent*>(event)->pos(),
                                                Qt::LeftButton, Qt::LeftButton, 0);
    qApp->sendEvent(tabBar, releaseEvent);
    execDrag(QList<QWidget*>() << toolWindow);
  }
  return false;
}

bool ToolWindowManager::tabWidgetEventFilter(QTabWidget *tabWidget, QEvent *event) {
  if (event->type() == QEvent::MouseButtonPress &&
      qApp->mouseButtons() & Qt::LeftButton &&
      tabWidget != m_emptySpacer) {
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
    if (m_suggestedReceiver) {
      if (m_dropSuggestionSwitchTimer.isActive()) {
        m_dropSuggestionSwitchTimer.stop();
      }
      m_dropSuggestionSwitchTimer.start();
    }
    event->setAccepted(m_suggestedReceiver != 0);
  } else if (event->type() == QEvent::DragLeave) {
    if (!topSplitter->rect().contains(topSplitter->mapFromGlobal(QCursor::pos()))) {
      hidePlaceHolder();
    }
    m_dragParent = 0;
  } else if (event->type() == QEvent::Drop) {
    if (!m_suggestedReceiver) {
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
    QSplitter* splitter = qobject_cast<QSplitter*>(m_suggestedReceiver);
    if (splitter) {
      splitter->insertWidget(m_suggestedIndexInSplitter,
        createDockItem(toolWindows, splitter->orientation()));
      addMissingSplitters(splitter);
    } else {
      QTabWidget* tabWidget = static_cast<QTabWidget*>(m_suggestedReceiver);
      foreach(QWidget* toolWindow, toolWindows) {
        tabWidget->addTab(toolWindow, toolWindow->windowIcon(), toolWindow->windowTitle());
      }
      m_lastUsedArea = tabWidget;
    }
    hidePlaceHolder();
    static_cast<QDropEvent*>(event)->acceptProposedAction();
    deleteAllEmptyItems();
    m_dragParent = 0;
  }
  return false;
}

void ToolWindowManager::setupTopLevelSplitter(QSplitter *splitter) {
  splitter->setAcceptDrops(true);
  splitter->installEventFilter(this); // for dropping
}

void ToolWindowManager::updateEmptySpacer() {
  if (m_emptySpacer && m_emptySpacer->count() > 0) {
     m_emptySpacer->setUpdatesEnabled(true);
    m_emptySpacer = 0;
  }
  bool anyContent = false; //m_subRootSplitter->count() > 0;
  foreach(QTabWidget* tabWidget, m_rootSplitter->findChildren<QTabWidget*>()) {
    if (tabWidget != m_emptySpacer) {
      anyContent = true;
      break;
    }
  }
  if (!anyContent && !m_emptySpacer && m_subRootSplitter) {
    m_emptySpacer = createTabWidget();
    m_emptySpacer->setUpdatesEnabled(false);
    m_subRootSplitter->addWidget(m_emptySpacer);
  }
  if (anyContent && m_emptySpacer) {
    m_emptySpacer->hide();
    delete m_emptySpacer;
    m_emptySpacer = 0;
  }
}

QSplitter *ToolWindowManager::createSplitter() {
  QSplitter* splitter = new QSplitter();
  splitter->setChildrenCollapsible(false);
  return splitter;
}


