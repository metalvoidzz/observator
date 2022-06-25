#include "MainWindow.h"

#include <QAction>
#include <QBoxLayout>
#include <QDebug>
#include <QErrorMessage>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QTextStream>
#include <QTimer>
#include <QStatusBar>
#include <QMimeData>
#include <QMimeDatabase>
#include <QProcess>
#include <QIODevice>
#include <QElapsedTimer>

MainWindow::MainWindow(QWidget *parent, QApplication* app)
    : QMainWindow(parent),
      app {app}
{
    QWidget *widget = new QWidget;
    setCentralWidget(widget);

    QLabel *label = new QLabel("Öffnen Sie ein Dokument");
    label->setAlignment(Qt::AlignCenter);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->addWidget(label);
    widget->setLayout(layout);

    setAcceptDrops(true);

    createActions();
    createMenus();

    resize(800, 800);

    setWindowTitle("Observator");

    statusLabel = new QLabel;
    statusBar()->addPermanentWidget(statusLabel);
    setStatus(Status::READY);

    lastWindowSize = width() * height();
    appDir = app->applicationDirPath() + '/';
    resourceDir = QDir::currentPath();
}

MainWindow::~MainWindow()
{
}

void MainWindow::watchDocument(const QString& path) {
    watcher = new QFileSystemWatcher(this);
    if(!watcher->addPath(path)) {
        QMessageBox error;
        error.warning(nullptr, "Fehler", "Kann die Datei nicht beobachten!");
        return;
    }
    currentSourcePath = path;
    lastExportPath.clear();
    QDir::setCurrent(QFileInfo(path).absolutePath());
    connect(watcher, &QFileSystemWatcher::fileChanged, this, &MainWindow::updateDocument);

    if (!browser) {
        QWidget *widget = new QWidget();
        browser = new QWebEngineView;
        browser->setAcceptDrops(false);
        browser->setZoomFactor(1.25);
        connect(browser->page(), SIGNAL(contentsSizeChanged(const QSizeF&)), this, SLOT(restoreScrollBarPosition()));
        QVBoxLayout *layout = new QVBoxLayout;
        layout->setSpacing(0);
        layout->setMargin(0);
        layout->addWidget(browser);
        widget->setLayout(layout);
        setCentralWidget(widget);
    }

    scrollBarPos = 0;
    restoreScrollBarPosition();
    updateDocument();
}

void MainWindow::createActions() {
    openAction = new QAction("Öffnen", this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);

    saveAction = new QAction("Speichern", this);
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::exportFile);

    saveAsAction = new QAction("Speichern unter", this);
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::exportFileAs);

    reloadAction = new QAction("Erneut laden", this);
    reloadAction->setShortcut(QKeySequence::Refresh);
    connect(reloadAction, &QAction::triggered, this, &MainWindow::updateDocument);

    exitAction = new QAction("Schließen", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &MainWindow::close);

    scrollToEndAction = new QAction("Zum Ende blättern", this);
    scrollToEndAction->setCheckable(true);
    scrollToEndAction->setChecked(true);

    aboutAction = new QAction("Über", this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::about);
}

void MainWindow::createMenus() {
    fileMenu = menuBar()->addMenu("Datei");
    fileMenu->addAction(openAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(saveAsAction);
    fileMenu->addAction(reloadAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);
    viewMenu = menuBar()->addMenu("Ansicht");
    viewMenu->addAction(scrollToEndAction);
    helpMenu = menuBar()->addMenu("Hilfe");
    helpMenu->addAction(aboutAction);
}

void MainWindow::openFile() {
    QString openDir;

    if (!currentSourcePath.isEmpty()) {
        openDir = QFileInfo(currentSourcePath).absoluteDir().path();
    } else {
        openDir = QDir::homePath();
    }

    QString filePath = QFileDialog::getOpenFileName(this, "Öffne Dokument", openDir, "Markdown Files (*.md)");
    if (!filePath.isEmpty()) {
        watchDocument(filePath);
    }
}

void MainWindow::exportFile() {
    if (!lastExportPath.isEmpty()) {
        exportTo(lastExportPath);
    } else {
        exportFileAs();
    }
}

void MainWindow::exportFileAs() {
    QString filePath = QFileDialog::getSaveFileName(this, "Exportieren", QDir::homePath());
    if (!filePath.isEmpty()) {
        exportTo(filePath);
        lastExportPath = filePath;
    }
}

void MainWindow::about() {
    QMessageBox message;
    message.about(this, "Über den Observator", "Copyright (C) 2022 Julian Offenhäuser");
    message.show();
}

void MainWindow::restoreScrollBarPosition() {
    if (lastWindowSize == width() * height()) {
        double pos = scrollBarPos / browser->zoomFactor();
        browser->page()->runJavaScript(QString("window.scrollTo(0, %1);").arg(static_cast<int>(pos)));
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event) {
    const QMimeData *data = event->mimeData();
    if (!data->hasUrls()) return;

    const QString url = data->urls().at(0).toLocalFile();

    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(url);
    if (type.name() == "text/markdown") {
        watchDocument(url);
    } else {
        QMessageBox error;
        error.warning(nullptr, "Öffnen fehlgeschlagen", "Falscher MIME type: " + type.name());
    }
}

void MainWindow::updateDocument() {
    QElapsedTimer timer;
    timer.start();

    setStatus(Status::PROCESSING_DOCUMENT);

    QFile inputFile(currentSourcePath);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        QMessageBox error;
        error.critical(nullptr, "Öffnen fehlgeschlagen", inputFile.errorString());
        setStatus(Status::READY);
        return;
    }

    QByteArray markdown = filterDocument(inputFile.readAll());
    inputFile.close();

    setStatus(Status::COMPILING_DOCUMENT);

    QProcess process;
    process.start("pandoc",
                  QStringList()
                  << "-s"
                  << "--from=markdown"
                  << "--to=html"
                  << "--toc"
                  << "-V" << "geometry:a4paper,margin=1cm"
                  << QString("--mathjax=%1/es5/tex-svg-full.js").arg(resourceDir));
    process.write(markdown);
    process.closeWriteChannel();
    process.waitForFinished(-1); // Pandoc solle sich nicht aufhängen

    if (process.exitCode()) {
        QMessageBox error;
        error.critical(nullptr, "Fehler beim Zusammenstellen", process.readAllStandardError());
        setStatus(Status::READY);
        return;
    }

    QString contents = process.readAllStandardOutput();

    /* Wenn die Bildlaufleiste weit unten im Dokument ist, springen wir ans Ende.
     * Weit unten bedeutet, daß man maximal eine halbe Seite vom Schluß entfernt ist. */
    const auto scroll = browser->page()->scrollPosition();
    const auto pageSize = browser->page()->contentsSize();

    if (scrollToEndAction->isChecked()) {
        scrollBarPos = pageSize.height() * 2;
    } else {
        scrollBarPos = scroll.y();
    }

    lastWindowSize = width() * height();
    browser->setHtml(contents, QUrl::fromLocalFile(QDir::currentPath() + "/"));
    restoreScrollBarPosition();

    setStatus(Status::READY);

    statusBar()->showMessage(QString("Zusammengestellt in %1 Sekunden.").arg(timer.elapsed() / 1000.0f), 3000);
}

QByteArray MainWindow::filterDocument(const QByteArray& document) {
    QByteArray out;

    int index = 0;

    auto peek = [&document, &index](const int offset) -> char {
        if (index + offset >= document.size()) {
            return 0;
        }
        return document.at(index + offset);
    };

    for (index = 0; index < document.size(); ++index) {
        if (    peek(0) == '<' &&
                peek(1) == '!' &&
                peek(2) == '-' &&
                peek(3) == '-' &&
                peek(4) == 'T' &&
                peek(5) == 'A' &&
                peek(6) == 'B' &&
                peek(7) == 'L' &&
                peek(8) == 'E'
                ) {
            index += 9 + 2; // Leerzeichen und "

            QByteArray fileName;
            for (char c = document.at(index); c != L'"'; c = document.at(++index)) {
                fileName += c;
            }

            index += 4; /* "--> */

            // Füge Tabelle an Ort und Stelle ein
            QString absoluteScriptPath = appDir + "markdown-table.py";
            QString absoluteTablePath = QFileInfo(currentSourcePath).absoluteDir().path() + '/' + fileName;

            QProcess process;
            process.start(
                        "python",
                        QStringList() << absoluteScriptPath << absoluteTablePath,
                        QIODevice::ReadWrite);
            process.waitForFinished(-1);
            if (process.exitCode()) {
                QMessageBox error;
                error.critical(nullptr, "Tabellenfehler", process.readAllStandardError());
            }
            out.append(process.readAllStandardOutput());

            // Beobachte Tabelle
            watcher->addPath(absoluteTablePath);
        }
        out.append(document.at(index));
    }

    return out;
}

void MainWindow::exportTo(const QString& path) {
    setStatus(Status::PROCESSING_DOCUMENT);

    QFile inputFile(currentSourcePath);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        QMessageBox error;
        error.critical(nullptr, "Kann das Dokument nicht öffnen", inputFile.errorString());
        setStatus(Status::READY);
        return;
    }

    QByteArray markdown = filterDocument(inputFile.readAll());
    inputFile.close();

    setStatus(Status::COMPILING_DOCUMENT);

    QProcess process;
    process.start("pandoc",
                  QStringList()
                  << "-s"
                  << "--from=markdown"
                  << "--toc"
                  << "-V" << "geometry:a4paper,margin=2cm"
                  << "-o" << path);
    process.write(markdown);
    process.closeWriteChannel();
    process.waitForFinished(-1); // Pandoc solle sich nicht aufhängen
    if (process.exitCode()) {
        QMessageBox error;
        error.critical(nullptr, "Fehler beim Zusammenstellen", process.readAllStandardError());
    }
    setStatus(Status::READY);
}

void MainWindow::setStatus(Status status) {
    switch (status) {
    case Status::READY:
        statusLabel->setText("Fertig.");
        break;
    case Status::PROCESSING_DOCUMENT:
        statusLabel->setText("Verarbeite Dokument...");
        break;
    case Status::COMPILING_DOCUMENT:
        statusLabel->setText("Stelle Dokument zusammen...");
        break;
    }

    app->processEvents();
}
