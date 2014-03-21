#ifndef TOOLWINDOWMANAGERWRAPPER_H
#define TOOLWINDOWMANAGERWRAPPER_H

#include <QWidget>
#include <QVariantMap>

class ToolWindowManager;

class ToolWindowManagerWrapper : public QWidget {
  Q_OBJECT
public:
  explicit ToolWindowManagerWrapper(ToolWindowManager* manager);
  virtual ~ToolWindowManagerWrapper();

protected:
  virtual void dragEnterEvent(QDragEnterEvent *);
  virtual void dragMoveEvent(QDragMoveEvent *);
  virtual void dragLeaveEvent(QDragLeaveEvent *);
  virtual void dropEvent(QDropEvent *);
  virtual void closeEvent(QCloseEvent *);

private:
  ToolWindowManager* m_manager;
  QVariantMap saveState();
  void restoreState(const QVariantMap& data);

  friend class ToolWindowManager;

};

#endif // TOOLWINDOWMANAGERWRAPPER_H
