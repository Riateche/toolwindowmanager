#ifndef TOOLWINDOWMANAGER_H
#define TOOLWINDOWMANAGER_H

#include <QWidget>
#include <QSplitter>
#include <QTabWidget>
#include <QTabBar>
#include <QTimer>
#include <QRubberBand>

/*!
 * \brief The ToolWindowManager class provides Eclipse-like docking tool behavior.
 *
 * The ToolWindowManager class provides docking tool behavior  that is similar to
 * behavior of tool windows in Visual Studio or Eclipse. User can arrange tool windows
 * in tabs, dock it to any border, split with vertical and horizontal splitters,
 * tabify them together and detach to floating windows.
 */
class ToolWindowManager : public QWidget {
  Q_OBJECT
public:
  /*!
   * \brief Creates a manager with given parent.
   */
  explicit ToolWindowManager(QWidget *parent = 0);
  virtual ~ToolWindowManager();
  void setCentralWidget(QWidget* widget);
  QWidget* centralWidget() { return m_centralWidget; }


  enum DockArea {
    LeftDockArea,
    RightDockArea,
    TopDockArea,
    BottomDockArea,
    LastUsedArea,
    NewFloatingArea,
    NoArea
  };

  void addToolWindow(QWidget* toolWindow, DockArea dockArea = LeftDockArea);
  void moveToolWindow(QWidget* toolWindow, DockArea dockArea = LeftDockArea);

  const QList<QWidget*>& toolWindows() { return m_toolWindows; }
  //void setToolWindowVisible(QWidget* toolWindow, bool visible);
  //void showToolWindow(QWidget* toolWindow) { setToolWindowVisible(toolWindow, true); }
  void hideToolWindow(QWidget* toolWindow) { moveToolWindow(toolWindow, NoArea); }
  void setSuggestionSwitchInterval(int msec);
  int suggestionSwitchInterval();
  void setDragMimeType(const QString& mimeType);
  const QString& dragMimeType() { return m_dragMimeType; }
  int borderSensitivity() { return m_borderSensitivity; }
  void setBorderSensitivity(int pixels);
  void setRubberBandLineWidth(int pixels);
  int rubberBandLineWidth() { return m_rubberBandLineWidth; }
  QVariant saveState();
  void restoreState(const QVariant& data);
  QList<QWidget*> floatingWindows();


signals:
  void toolWindowVisibilityChanged(QWidget* toolWindow, bool visible);


private:
  QWidget* m_centralWidget;
  QWidget* m_centralWidgetContainer;
  QList<QWidget*> m_toolWindows;
  int m_borderSensitivity;
  int m_rubberBandLineWidth;
  QString m_dragMimeType;

  QRubberBand* m_rectPlaceHolder;
  QRubberBand* m_linePlaceHolder;
  QSplitter* m_suggestedSplitter;
  int m_suggestedIndexInSplitter;
  bool m_tabWidgetDragCanStart;
  QWidget* m_dragParent;

  int m_dropCurrentSuggestionIndex;
  QPoint m_dropGlobalPos;
  QTimer m_dropSuggestionSwitchTimer;
  QSplitter* m_rootSplitter;
  QTabWidget* m_lastUsedArea;

  QHash<QTabBar*, QTabWidget*> m_hash_tabbar_to_tabwidget;

  QWidget* createDockItem(const QList<QWidget*>& toolWindows, Qt::Orientations parentOrientation);
  void hidePlaceHolder();
  void releaseToolWindow(QWidget* toolWindow);
  void execDrag(const QList<QWidget*>& toolWindows);

  QVariantMap saveSplitterState(QSplitter* splitter);
  QSplitter* restoreSplitterState(const QVariantMap& data);

  void addMissingSplitters(QSplitter* splitter);

protected:
  virtual bool eventFilter(QObject *object, QEvent *event);
  bool tabBarEventFilter(QTabBar* tabBar, QEvent* event);
  bool tabWidgetEventFilter(QTabWidget* tabWidget, QEvent* event);
  bool topSplitterEventFilter(QSplitter* topSplitter, QEvent* event);
  virtual QSplitter* createSplitter();
  virtual QTabWidget* createTabWidget();

  virtual QPixmap generateDragPixmap(const QList<QWidget *> &toolWindows);

private slots:
  void dropSuggestionSwitchTimeout();
  void tabCloseRequested(int index);
  void tabBarDestroyed(QObject* object);

};

#endif // TOOLWINDOWMANAGER_H
