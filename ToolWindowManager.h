#ifndef TOOLWINDOWMANAGER_H
#define TOOLWINDOWMANAGER_H

#include <QWidget>
#include <QSplitter>


class ToolWindowManager : public QWidget {
  Q_OBJECT
public:
  explicit ToolWindowManager(QWidget *parent = 0);
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



};

#endif // TOOLWINDOWMANAGER_H
