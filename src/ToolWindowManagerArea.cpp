#include "ToolWindowManagerArea.h"
#include "ToolWindowManager.h"
#include <QApplication>
#include <QMouseEvent>

ToolWindowManagerArea::ToolWindowManagerArea(ToolWindowManager *manager, QWidget *parent) :
  QTabWidget(parent)
, m_manager(manager)
{
  m_dragCanStart = false;
  setMovable(true);
  setTabsClosable(true);
  tabBar()->installEventFilter(this);
  m_manager->m_areas << this;
}

ToolWindowManagerArea::~ToolWindowManagerArea() {
  m_manager->m_areas.removeOne(this);
}

void ToolWindowManagerArea::addToolWindow(QWidget *toolWindow) {
  addToolWindows(QList<QWidget*>() << toolWindow);
}

void ToolWindowManagerArea::addToolWindows(const QList<QWidget *> &toolWindows) {
  int index = 0;
  foreach(QWidget* toolWindow, toolWindows) {
    index = addTab(toolWindow, toolWindow->windowIcon(), toolWindow->windowTitle());
  }
  setCurrentIndex(index);
  m_manager->m_lastUsedArea = this;
}

QList<QWidget *> ToolWindowManagerArea::toolWindows() {
  QList<QWidget *> result;
  for(int i = 0; i < count(); i++) {
    result << widget(i);
  }
  return result;
}

void ToolWindowManagerArea::mousePressEvent(QMouseEvent *) {
  if (qApp->mouseButtons() & Qt::LeftButton) {
    m_dragCanStart = true;
  }
}

void ToolWindowManagerArea::mouseReleaseEvent(QMouseEvent *) {
  m_dragCanStart = false;
}

void ToolWindowManagerArea::mouseMoveEvent(QMouseEvent *) {
  if (qApp->mouseButtons() & Qt::LeftButton &&
      !rect().contains(mapFromGlobal(QCursor::pos())) &&
      m_dragCanStart) {
    m_dragCanStart = false;
    QList<QWidget*> toolWindows;
    for(int i = 0; i < count(); i++) {
      QWidget* toolWindow = widget(i);
      if (!m_manager->m_toolWindows.contains(toolWindow)) {
        qWarning("tab widget contains unmanaged widget");
      } else {
        toolWindows << toolWindow;
      }
    }
    m_manager->execDrag(toolWindows);
  }
}

bool ToolWindowManagerArea::eventFilter(QObject *object, QEvent *event) {
  if (object == tabBar() && event->type() == QEvent::MouseMove) {
    if (tabBar()->rect().contains(static_cast<QMouseEvent*>(event)->pos())) {
      return false;
    }
    if (!(qApp->mouseButtons() & Qt::LeftButton)) {
      return false;
    }
    QWidget* toolWindow = currentWidget();
    if (!toolWindow || !m_manager->m_toolWindows.contains(toolWindow)) {
      return false;
    }
    QMouseEvent* releaseEvent = new QMouseEvent(QEvent::MouseButtonRelease,
                                                static_cast<QMouseEvent*>(event)->pos(),
                                                Qt::LeftButton, Qt::LeftButton, 0);
    qApp->sendEvent(tabBar(), releaseEvent);
    m_manager->execDrag(QList<QWidget*>() << toolWindow);
  }
  return QTabWidget::eventFilter(object, event);
}

QVariantMap ToolWindowManagerArea::saveState() {
  QVariantMap result;
  result["type"] = "area";
  result["currentIndex"] = currentIndex();
  QStringList objectNames;
  for(int i = 0; i < count(); i++) {
    QString name = widget(i)->objectName();
    if (name.isEmpty()) {
      qWarning("cannot save state of tool window without object name");
    } else {
      objectNames << name;
    }
  }
  result["objectNames"] = objectNames;
  return result;
}

void ToolWindowManagerArea::restoreState(const QVariantMap &data) {
  foreach(QVariant objectNameValue, data["objectNames"].toList()) {
    QString objectName = objectNameValue.toString();
    if (objectName.isEmpty()) { continue; }
    bool found = false;
    foreach(QWidget* toolWindow, m_manager->m_toolWindows) {
      if (toolWindow->objectName() == objectName) {
        addToolWindow(toolWindow);
        found = true;
        break;
      }
    }
    if (!found) {
      qWarning("tool window with name '%s' not found", objectName.toLocal8Bit().constData());
    }
  }
  setCurrentIndex(data["currentIndex"].toInt());
}
