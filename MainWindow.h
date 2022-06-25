#pragma once

#include <QFileSystemWatcher>
#include <QMainWindow>
#include <QWebEngineView>
#include <QDropEvent>
#include <QApplication>
#include <QHash>
#include <QLabel>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent, QApplication* app);
    ~MainWindow();

    void watchDocument(const QString& path);

public slots:
    void openFile();
    void exportFile();
    void exportFileAs();
    void about();
    void restoreScrollBarPosition();

protected:
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);

private:
    enum class Status {
        READY,
        PROCESSING_DOCUMENT,
        COMPILING_DOCUMENT
    };

    void createActions();
    void createMenus();
    void updateDocument();
    QByteArray filterDocument(const QByteArray& document);
    void exportTo(const QString& path);
    void setStatus(Status status);

    QFileSystemWatcher *watcher;
    QWebEngineView *browser { NULL };
    double scrollBarPos;
    int lastWindowSize;
    QApplication *app;
    QLabel *statusLabel;

    QString appDir;
    QString resourceDir;
    QString currentSourcePath;
    QString lastExportPath;

    QMenu *fileMenu;
    QMenu *viewMenu;
    QMenu *helpMenu;

    QAction *openAction;
    QAction *saveAction;
    QAction *saveAsAction;
    QAction *reloadAction;
    QAction *exitAction;
    QAction *scrollToEndAction;
    QAction *aboutAction;
};
