#include "ToolWindowManagerWrapper.h"
#include "ToolWindowManager.h"
#include "ToolWindowManagerArea.h"
#include <QVBoxLayout>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDebug>

ToolWindowManagerWrapper::ToolWindowManagerWrapper(ToolWindowManager *manager) :
  QWidget(manager)
, m_manager(manager)
{
  setAcceptDrops(true);
  setWindowFlags(windowFlags() | Qt::Tool);

  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  m_manager->m_wrappers << this;
}

ToolWindowManagerWrapper::~ToolWindowManagerWrapper() {
  m_manager->m_wrappers.removeOne(this);
}


void ToolWindowManagerWrapper::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->formats().contains(m_manager->m_dragMimeType)) {
    event->accept();
  }
}

void ToolWindowManagerWrapper::dragMoveEvent(QDragMoveEvent *event) {
  m_manager->findSuggestions(this);

  if (!m_manager->m_suggestions.isEmpty()) {
    //starting or restarting timer
    if (m_manager->m_dropSuggestionSwitchTimer.isActive()) {
      m_manager->m_dropSuggestionSwitchTimer.stop();
    }
    m_manager->m_dropSuggestionSwitchTimer.start();
  }
  event->setAccepted(!m_manager->m_suggestions.isEmpty());

}

void ToolWindowManagerWrapper::dragLeaveEvent(QDragLeaveEvent *) {
  if (!rect().contains(mapFromGlobal(QCursor::pos()))) {
    m_manager->hidePlaceHolder();
  }
}

void ToolWindowManagerWrapper::dropEvent(QDropEvent *event) {
  if (m_manager->m_dropCurrentSuggestionIndex >= m_manager->m_suggestions.count()) {
    qWarning("unexpected drop event");
    return;
  }
  ToolWindowManager::AreaReference suggestion = m_manager->m_suggestions[m_manager->m_dropCurrentSuggestionIndex];
  QList<QWidget*> toolWindows = m_manager->extractToolWindowsFromDropEvent(event);
  m_manager->hidePlaceHolder();
  event->acceptProposedAction();
  m_manager->moveToolWindows(toolWindows, suggestion);
}


void ToolWindowManagerWrapper::closeEvent(QCloseEvent *) {
  QList<QWidget*> toolWindows;
  foreach(ToolWindowManagerArea* tabWidget, findChildren<ToolWindowManagerArea*>()) {
    toolWindows << tabWidget->toolWindows();
  }
  m_manager->moveToolWindows(toolWindows, ToolWindowManager::NoArea);
}

QVariantMap ToolWindowManagerWrapper::saveState() {
  if (layout()->count() > 1) {
    qWarning("too many children for wrapper");
    return QVariantMap();
  }
  if (isWindow() && layout()->count() == 0) {
    qWarning("empty top level wrapper");
    return QVariantMap();
  }
  QVariantMap result;
  result["geometry"] = saveGeometry();
  QSplitter* splitter = findChild<QSplitter*>("", Qt::FindDirectChildrenOnly);
  if (splitter) {
    result["splitter"] = m_manager->saveSplitterState(splitter);
  } else {
    ToolWindowManagerArea* area = findChild<ToolWindowManagerArea*>("", Qt::FindDirectChildrenOnly);
    if (area) {
      result["area"] = area->saveState();
    } else if (layout()->count() > 0) {
      qWarning("unknown child");
      return QVariantMap();
    }
  }
  return result;
}

void ToolWindowManagerWrapper::restoreState(const QVariantMap &data) {
  restoreGeometry(data["geometry"].toByteArray());
  if (layout()->count() > 0) {
    qWarning("wrapper is not empty");
    return;
  }
  if (data.contains("splitter")) {
    layout()->addWidget(m_manager->restoreSplitterState(data["splitter"].toMap()));
  } else if (data.contains("area")) {
    ToolWindowManagerArea* area = m_manager->createArea();
    area->restoreState(data["area"].toMap());
    layout()->addWidget(area);
  }
}
