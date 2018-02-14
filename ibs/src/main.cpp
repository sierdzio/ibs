/*******************************************************************************
Copyright (C) 2017 Milo Solutions
Contact: https://www.milosolutions.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/


/*
  TEMPLATE main.cpp by Milo Solutions. Copyright 2016
*/

#include <QCoreApplication>
#include <QLoggingCategory>

#include <QString>
#include <QCommandLineParser>
#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>

#include "globals.h"
#include "projectmanager.h"

// Prepare logging categories. Modify these to your needs
//Q_DECLARE_LOGGING_CATEGORY(core) // already declared in MLog header
Q_LOGGING_CATEGORY(coreMain, "core.main")

/*!
  Main routine. Remember to update the application name and initialise logger
  class, if present.
  */
int main(int argc, char *argv[]) {
    //MiloLog::instance();
    // Set up basic application data. Modify this to your needs
    QCoreApplication app(argc, argv);

    QElapsedTimer timer;
    timer.start();

    app.setApplicationVersion(APP_VERSION);
    app.setOrganizationName("");
    app.setOrganizationDomain("sierdzio.com");
    app.setApplicationName("ibs");
    //logger()->enableLogToFile(app.applicationName());
    qCInfo(coreMain) << "\n\tName:" << app.applicationName()
                     << "\n\tOrganisation:" << app.organizationName()
                     << "\n\tDomain:" << app.organizationDomain()
                     << "\n\tVersion:" << app.applicationVersion()
                     << "\n\tSHA:" << GIT_COMMIT_ID;


    QCommandLineParser parser;
    parser.setApplicationDescription("Test helper");
    parser.addHelpOption();
    parser.addVersionOption();

    const char *scope = "main";
    parser.addPositionalArgument("input", QCoreApplication::translate(scope, "Input file, usually main.cpp"), "[input]");

    parser.addOptions({
        {{"r", "run"},
        QCoreApplication::translate(scope, "Run the executable immediately after building")},
        {"qt-dir",
        QCoreApplication::translate(scope, "Specify Qt directory for Qt apps"),
        QCoreApplication::translate(scope, "Qt dir")},
        {"clean",
        QCoreApplication::translate(scope, "Clear build directory")}
    });

    // Process the actual command line arguments given by the user
    parser.process(app);

    const QStringList args = parser.positionalArguments();

    qDebug() << "Arguments:" << args;

    const bool run = parser.isSet("run");
    const bool clean = parser.isSet("clean");
    const QString qtDir(parser.value("qt-dir"));
    QString file;

    if (!args.isEmpty())
        file = args.at(0);

    if (clean) {
        ProjectManager manager(file);
        manager.loadCache();
        QObject::connect(&manager, &ProjectManager::finished, &app, &QCoreApplication::quit);
        QTimer::singleShot(1, &manager, &ProjectManager::clean);
        int result = app.exec();
        qInfo() << "Cleaning took:" << timer.elapsed() << "ms";
        return result;
    } else {
        ProjectManager manager(file);
        manager.setQtDir(qtDir);
        QObject::connect(&manager, &ProjectManager::finished, &app, &QCoreApplication::quit);
        QTimer::singleShot(1, &manager, &ProjectManager::start);

        if (run) {
            qInfo() << "Running compiled binary";
        }

        int result = app.exec();
        qInfo() << "Build took:" << timer.elapsed() << "ms";
        return result;
    }

    return 1;
}
