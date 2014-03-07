#ifndef TOOLWINDOWMANAGER_H
#define TOOLWINDOWMANAGER_H

#include <QWidget>
#include <QSplitter>
#include <QTabWidget>
#include <QTabBar>


class ToolWindowManager : public QWidget {
  Q_OBJECT
public:
  explicit ToolWindowManager(QWidget *parent = 0);
  virtual ~ToolWindowManager();
  void setCentralWidget(QWidget* widget);
  QWidget* centralWidget() { return m_centralWidget; }
  void addToolWindow(QWidget* widget);


signals:

public slots:

private:
  QWidget* m_centralWidget;
  QWidget* m_centralWidgetContainer;
  QSplitter* m_mainSplitter;
  QList<QWidget*> m_toolWindows;
  int m_borderSensitivity;
  QWidget* m_placeHolder;
  QString m_dragMimeType;

  QHash<QTabBar*, QTabWidget*> m_hash_tabbar_to_tabwidget;


protected:
  virtual bool eventFilter(QObject *object, QEvent *event);
  virtual void dragEnterEvent(QDragEnterEvent* event);
  virtual void dragMoveEvent(QDragMoveEvent *event);
  virtual void dragLeaveEvent(QDragLeaveEvent *event);
  virtual void dropEvent(QDropEvent *event);

  virtual QPixmap generateDragPixmap(QWidget *toolWindow);

};

#endif // TOOLWINDOWMANAGER_H
