#ifndef TOOLWINDOWMANAGER_H
#define TOOLWINDOWMANAGER_H

#include <QWidget>
#include <QSplitter>
#include <QTabWidget>
#include <QTabBar>
#include <QTimer>
#include <QRubberBand>

class ToolWindowManager : public QWidget {
  Q_OBJECT
public:
  explicit ToolWindowManager(QWidget *parent = 0);
  virtual ~ToolWindowManager();
  void setCentralWidget(QWidget* widget);
  QWidget* centralWidget() { return m_centralWidget; }
  void addToolWindow(QWidget* toolWindow);
  const QList<QWidget*>& toolWindows() { return m_toolWindows; }
  void setToolWindowVisible(QWidget* toolWindow, bool visible);
  void showToolWindow(QWidget* toolWindow) { setToolWindowVisible(toolWindow, true); }
  void hideToolWindow(QWidget* toolWindow) { setToolWindowVisible(toolWindow, false); }

signals:
  void toolWindowVisibilityChanged(QWidget* toolWindow, bool visible);


private:
  QWidget* m_centralWidget;
  QWidget* m_centralWidgetContainer;
  QList<QWidget*> m_toolWindows;
  int m_borderSensitivity;
  QString m_dragMimeType;

  QRubberBand* m_rectPlaceHolder;
  QRubberBand* m_linePlaceHolder;
  QSplitter* m_suggestedSplitter;
  int m_suggestedIndexInSplitter;
  bool m_tabWidgetDragCanStart;

  int m_dropCurrentSuggestionIndex;
  QPoint m_dropGlobalPos;
  QTimer m_dropSuggestionSwitchTimer;

  QHash<QTabBar*, QTabWidget*> m_hash_tabbar_to_tabwidget;

  QWidget* createDockItem(const QList<QWidget*>& toolWindows, Qt::Orientation parentOrientation);
  QTabWidget* createTabWidget();

  void hidePlaceHolder();
  void releaseToolWindow(QWidget* toolWindow);

  void execDrag(const QList<QWidget*>& toolWindows);

protected:
  virtual bool eventFilter(QObject *object, QEvent *event);
  virtual void dragEnterEvent(QDragEnterEvent* event);
  virtual void dragMoveEvent(QDragMoveEvent *event);
  virtual void dragLeaveEvent(QDragLeaveEvent *event);
  virtual void dropEvent(QDropEvent *event);

  virtual QPixmap generateDragPixmap(const QList<QWidget *> &toolWindows);

private slots:
  void dropSuggestionSwitchTimeout();
  void tabCloseRequested(int index);

};

#endif // TOOLWINDOWMANAGER_H
