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
#ifndef TOOLWINDOWMANAGER_H
#define TOOLWINDOWMANAGER_H

#include <QWidget>
#include <QSplitter>
#include <QTabWidget>
#include <QTabBar>
#include <QTimer>
#include <QRubberBand>
#include <QHash>
#include <QVariant>

/*!
 * \brief The ToolWindowManager class provides docking tool behavior.
 *
 * The behavior is similar to tool windows mechanism in Visual Studio or Eclipse.
 * User can arrange tool windows
 * in tabs, dock it to any border, split with vertical and horizontal splitters,
 * tabify them together and detach to floating windows.
 *
 * See https://github.com/Riateche/toolwindowmanager for detailed description.
 */
class ToolWindowManager : public QWidget {
  Q_OBJECT
  /*!
   * \brief The delay between showing the next suggestion of drop location in milliseconds.
   *
   * When user starts a tool window drag and moves mouse pointer to a position, there can be
   * an ambiguity in new position of the tool window. If user holds the left mouse button and
   * stops mouse movements, all possible suggestions will be indicated periodically, one at a time.
   *
   * Default value is 1000 (i.e. 1 second).
   *
   * Access functions: suggestionSwitchInterval, setSuggestionSwitchInterval.
   *
   */
  Q_PROPERTY(int suggestionSwitchInterval READ suggestionSwitchInterval WRITE setSuggestionSwitchInterval)
  /*!
   * \brief The mime type used for dragding tool windows.
   *
   * Default value is "application/x-tool-window-ids".
   *
   * Access functions: dragMimeType, setDragMimeType.
   */
  Q_PROPERTY(QString dragMimeType READ dragMimeType WRITE setDragMimeType)
  /*!
   * \brief Maximal distance in pixels between mouse position and area border that allows
   * to display a suggestion.
   *
   * Default value is 12.
   *
   * Access functions: borderSensitivity, setBorderSensitivity.
   */
  Q_PROPERTY(int borderSensitivity READ borderSensitivity WRITE setBorderSensitivity)
  /*!
   * \brief Visible width of rubber band line that is used to display drop suggestions.
   *
   * Default value is 4.
   *
   * Access functions: rubberBandLineWidth, setRubberBandLineWidth.
   *
   */
  Q_PROPERTY(int rubberBandLineWidth READ rubberBandLineWidth WRITE setRubberBandLineWidth)

public:
  /*!
   * \brief Creates a manager with given \a parent.
   */
  explicit ToolWindowManager(QWidget *parent = 0);
  /*!
   * \brief Destroys the widget. Additionally all tool windows and all floating windows
   * created by this widget are destroyed.
   */
  virtual ~ToolWindowManager();

  /*!
   * \brief The DockArea enum is used to determine new position of the tool window.
   */
  enum DockArea {
    /*! Use an existing area in the left side of the manager.
     *  Creates new area if there isn't one yet.
     */
    LeftDockArea,
    /*! Similar to LeftDockArea. */
    RightDockArea,
    /*! Similar to LeftDockArea. */
    TopDockArea,
    /*! Similar to LeftDockArea. */
    BottomDockArea,
    /*! Similar to LeftDockArea. */
    CentralDockArea,
    /*! Use the area that has been used last time for adding a tool window.
     * Calling ToolWindowManager::addToolWindow, ToolWindowManager::moveToolWindow and
     * dragging tool windows with mouse will affect later behavior of this option.
     * If there is no appropriate area (i.e. none of above actions has been performed or
     * the last used area has been deleted), this options defaults to LeftDockArea.
      */
    LastUsedArea,
    /*! Use separate top level window for displaying the tool window. */
    NewFloatingArea,
    /*! Hide the tool window. */
    NoArea
  };

  /*!
   * \brief Adds \a toolWindow to the manager and moves it to the position specified by \a dockArea.
   * The manager takes ownership of the tool window and will delete it upon destruction.
   *
   * toolWindow->windowIcon() and toolWindow->windowTitle() will be used as the icon and title
   * of the tab that represents the tool window.
   *
   * If you intend to use ToolWindowManager::saveState
   * and ToolWindowManager::restoreState functions, you must set toolWindow->objectName() to
   * a non-empty unique string.
   */
  void addToolWindow(QWidget* toolWindow, DockArea dockArea = LeftDockArea);
  /*!
   * \brief Moves \a toolWindow to the position specified by \a dockArea.
   *
   * \a toolWindow must be added to the manager prior to calling this function.
   */
  void moveToolWindow(QWidget* toolWindow, DockArea dockArea = LeftDockArea);

  /*!
   * \brief Removes \a toolWindow from the manager. \a toolWindow becomes a hidden
   * top level widget. The ownership of \a toolWindow is returned to the caller.
   */
  void removeToolWindow(QWidget* toolWindow);
  /*!
   * \brief Returns all tool window added to the manager.
   */
  const QList<QWidget*>& toolWindows() { return m_toolWindows; }
  /*!
   * Hides \a toolWindow.
   *
   * \a toolWindow must be added to the manager prior to calling this function.
   */
  void hideToolWindow(QWidget* toolWindow) { moveToolWindow(toolWindow, NoArea); }
  /*!
   * \brief saveState
   */
  QVariant saveState();
  /*!
   * \brief restoreState
   */
  void restoreState(const QVariant& data);
  /*!
   * \brief floatingWindows
   */
  QList<QWidget*> floatingWindows();

  /*! \cond PRIVATE */
  void setSuggestionSwitchInterval(int msec);
  int suggestionSwitchInterval();
  void setDragMimeType(const QString& mimeType);
  const QString& dragMimeType() { return m_dragMimeType; }
  int borderSensitivity() { return m_borderSensitivity; }
  void setBorderSensitivity(int pixels);
  void setRubberBandLineWidth(int pixels);
  int rubberBandLineWidth() { return m_rubberBandLineWidth; }
  /*! \endcond */


signals:
  /*!
   * \brief This signal is emitted when \a toolWindow may be hidden or shown.
   * \a visible indicates new visibility state of the tool window.
   */
  void toolWindowVisibilityChanged(QWidget* toolWindow, bool visible);

private:
  QList<QWidget*> m_toolWindows; // all added tool windows
  int m_borderSensitivity;
  int m_rubberBandLineWidth;
  QString m_dragMimeType;

  QRubberBand* m_rectPlaceHolder; // placeholder objects used for displaying drop suggestions
  QRubberBand* m_linePlaceHolder;
  QWidget* m_suggestedReceiver; //splitter or tab widget that will receive the tool window if user
                                //dropped it now; 0 if there is currently no suggesion
  int m_suggestedIndexInSplitter; // position for dropping in suggested splitter
  bool m_tabWidgetDragCanStart; // indicates that user has started mouse movement on QTabWidget
                                // that can be considered as dragging it if the cursor will leave
                                // its area
  QSplitter* m_dragParent; // the widget that currently receives drag events; it's always
                           // the most top level splitter in the window (excluding parent widgets of
                           // the manager itself)
  QTabWidget* m_emptySpacer; // the widget that is added to m_subRootSplitter when there is no
                             // other items in it, to prevent collapsing parent widths to null size;
                             // 0, if there is no need in empty spacer.
  int m_dropCurrentSuggestionIndex; // index of currently displayed drop suggestion
                                    // (e.g. always 0 if there is only one possible drop location)
  QPoint m_dropGlobalPos; // mouse global position in the last accepted drag event
  QTimer m_dropSuggestionSwitchTimer; // used for switching drop suggestions
  QSplitter* m_rootSplitter; // most top level splitter of the manager (always horizontal)
  QSplitter* m_subRootSplitter; // second most top level splitter of the manager  (always vertical)
  QTabWidget* m_lastUsedArea; // last widget used for adding tool windows, or 0 if there isn't one

  QHash<QTabBar*, QTabWidget*> m_hash_tabbar_to_tabwidget; // all tab widgets managed by the manager
                                                           // and their respective tab bars

  QSplitter *createDockItem(const QList<QWidget*>& toolWindows, Qt::Orientations parentOrientation);
  void hidePlaceHolder();
  void releaseToolWindow(QWidget* toolWindow);
  void deleteAllEmptyItems();
  void deleteEmptyItems(QTabWidget* tabWidget);
  QWidget* wrapTopLevelSplitter(QSplitter* splitter);
  void execDrag(const QList<QWidget*>& toolWindows);

  QVariantMap saveSplitterState(QSplitter* splitter);
  QSplitter* restoreSplitterState(const QVariantMap& data);

  void addMissingSplitters(QSplitter* splitter);
  bool tabBarEventFilter(QTabBar* tabBar, QEvent* event);
  bool tabWidgetEventFilter(QTabWidget* tabWidget, QEvent* event);
  bool topSplitterEventFilter(QSplitter* topSplitter, QEvent* event);
  void setupTopLevelSplitter(QSplitter* splitter);
  void updateEmptySpacer();

protected:
  /*!
   * \brief Reimplemented from QWidget::eventFilter.
   */
  virtual bool eventFilter(QObject *object, QEvent *event);
  /*!
   * \brief Creates new splitter and sets its default properties. You may reimplement
   * this function to change properties of all splitters used by this class.
   */
  virtual QSplitter* createSplitter();
  /*!
   * \brief Creates new tab widget and sets its default properties. You may reimplement
   * this function to change properties of all tab widgets used by this class.
   */
  virtual QTabWidget* createTabWidget();
  /*!
   * \brief Generates a pixmap that is used to represent the data in a drag and drop operation
   * near the mouse cursor.
   * You may reimplement this function to use different pixmaps.
   */
  virtual QPixmap generateDragPixmap(const QList<QWidget *> &toolWindows);

private slots:
  void dropSuggestionSwitchTimeout();
  void tabCloseRequested(int index);
  void tabBarDestroyed(QObject* object);

};

#endif // TOOLWINDOWMANAGER_H
