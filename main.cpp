#include "MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Werkzeug zur Dokumenterstellung mit Markdown");
    parser.addHelpOption();
    parser.addPositionalArgument("datei", "Zu beobachtende Datei");
    parser.process(app);

    const QStringList args = parser.positionalArguments();

    MainWindow window(nullptr, &app);

    if (!args.empty())
        window.watchDocument(args.at(0));

    window.show();

    return app.exec();
}
