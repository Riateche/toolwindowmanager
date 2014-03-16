#ifndef TOOLWINDOWMANAGERAREA_H
#define TOOLWINDOWMANAGERAREA_H

#include <QTabWidget>

class ToolWindowManager;

class ToolWindowManagerArea : public QTabWidget {
  Q_OBJECT
public:
  explicit ToolWindowManagerArea(ToolWindowManager* manager, QWidget *parent = 0);
  virtual ~ToolWindowManagerArea();
  void addToolWindow(QWidget* toolWindow);
  void addToolWindows(const QList<QWidget*>& toolWindows);
  QList<QWidget*> toolWindows();

protected:
  virtual void mousePressEvent(QMouseEvent *);
  virtual void mouseReleaseEvent(QMouseEvent *);
  virtual void mouseMoveEvent(QMouseEvent *);
  virtual bool eventFilter(QObject *object, QEvent *event);

private:
  ToolWindowManager* m_manager;
  bool m_dragCanStart; // indicates that user has started mouse movement on QTabWidget
                       // that can be considered as dragging it if the cursor will leave
                       // its area

  QVariantMap saveState();
  void restoreState(const QVariantMap& data);

  friend class ToolWindowManager;
  friend class ToolWindowManagerWrapper;

};

#endif // TOOLWINDOWMANAGERAREA_H
