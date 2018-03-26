#include "projectmanager.h"
#include "fileparser.h"
#include "commandparser.h"

#include <QProcess>
#include <QFileInfo>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QCoreApplication>

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFile>

// TODO: add categorized logging!
#include <QDebug>

ProjectManager::ProjectManager(const Flags &flags, QObject *parent)
    : QObject(parent), mFlags(flags)
{
    mQtDir = mFlags.qtDir();
}

ProjectManager::~ProjectManager()
{
}

void ProjectManager::setQtDir(const QString &qtDir)
{
    if (qtDir == mQtDir) {
        return;
    }

    qInfo() << "Setting Qt directory to:" << qtDir;
    mQtDir = qtDir;

    // TODO: upgrade mQtLibs and mQtIncludes
}

QString ProjectManager::qtDir() const
{
    return mQtDir;
}

void ProjectManager::start()
{
    QByteArray tempScopeId;

    // First, check if any files need to be recompiled
    if (mCacheEnabled) {

        for (const auto &scope : mScopes.values()) {
            for (const auto &cached : scope.parsedFiles()) {
                // Check if object file exists. If somebody removed it, or used
                // --clean, then we have to recompile!

                if (mIsError)
                    return;

                if (isFileDirty(cached.path, mFlags.quickMode(), scope)) {
                    if (cached.type == FileInfo::Cpp) {
                        parseFile(cached.path);
                    } else if (cached.type == FileInfo::QRC) {
                        onRunTool(scope.id(), Tags::rcc, QStringList({ cached.path }));
                    }
                } else if (!cached.objectFile.isEmpty()) {
                    // There should be an object file on disk - let's check
                    const QFileInfo objFile(cached.objectFile);
                    if (!objFile.exists()) {
                        qDebug() << "Object file missing - recompiling";
                        compile(scope.id(), cached.path);
                    }
                } else if (!cached.generatedObjectFile.isEmpty()) {
                    // There should be an object file on disk - let's check
                    const QFileInfo objFile(cached.generatedObjectFile);
                    if (!objFile.exists()) {
                        qDebug() << "Generated object file missing - recompiling";
                        // TODO: also check and regenerate the generated file before
                        // compiling it!

                        const QFileInfo genFile(cached.generatedFile);
                        if (!genFile.exists()) {
                            qDebug() << "Generated file missing - regenerating";
                            if (cached.type == FileInfo::Cpp) {
                                // Moc file needs to be regenerated
                                onRunMoc(scope.id(), cached.path);
                            } else if (cached.type == FileInfo::QRC) {
                                // QRC c++ file needs to be regenerated
                                onRunTool(scope.id(), Tags::rcc, QStringList({ cached.path }));
                            }
                        }

                        //compile(cached.generatedFile);
                    }
                }
            }
        }
    } else {
        Scope scope(mFlags.inputFile(), mFlags.relativePath());

        if (mFlags.autoIncludes())
            scope.autoScanForIncludes();


        onParseRequest(scope.id(), mFlags.inputFile());
        tempScopeId = scope.id();
    }

    // When all jobs are done, notify main.cpp that we can quit
    connect(this, &ProjectManager::jobQueueEmpty, this, &ProjectManager::finished);
    // Parsing done, link it!
    link(tempScopeId);
    saveCache();
}

void ProjectManager::clean()
{
    if (!mQtModules.isEmpty()) {
        qInfo() << "Cleaning MOC and QRC files";
        const QString moc("moc_predefs.h");
        if (QFile::exists(moc)) {
            QFile::remove(moc);
        }
    }

    for (const auto &scope : mScopes.values()) {
        for (const auto &info : scope.parsedFiles()) {
            if (!info.objectFile.isEmpty())
                removeFile(info.objectFile);
            if (!info.generatedFile.isEmpty())
                removeFile(info.generatedFile);
            if (!info.generatedObjectFile.isEmpty())
                removeFile(info.generatedObjectFile);
        }
    }

    emit finished();
}

void ProjectManager::onError(const QString &error)
{
    qCritical() << "Error!" << error;
    mIsError = true;
}

/*!
 * Save necessary build info into IBS cache file
 */
void ProjectManager::saveCache() const
{
    QFile file(Tags::ibsCacheFileName);

    if (file.exists())
        file.remove();

    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        qFatal("Could not open IBS cache file for writing!");
    }

    QJsonObject mainObject;

    mainObject.insert(Tags::targetName, mTargetName);
    mainObject.insert(Tags::targetType, mTargetType);
    mainObject.insert(Tags::targetLib, mTargetLibType);
    mainObject.insert(Tags::qtDir, mQtDir);
    mainObject.insert(Tags::inputFile, mFlags.inputFile());
    mainObject.insert(Tags::qtModules, QJsonArray::fromStringList(mQtModules));
    mainObject.insert(Tags::defines, QJsonArray::fromStringList(mCustomDefines));
    //mainObject.insert(Tags::includes, QJsonArray::fromStringList(mCustomIncludes));
    mainObject.insert(Tags::libs, QJsonArray::fromStringList(mCustomLibs));

    QJsonArray scopesArray;
    for (const auto &scope : mScopes.values()) {
        scopesArray.append(scope.toJson());
    }
    mainObject.insert(Tags::scopes, scopesArray);

    // TODO: save also all tools that need to be run!

    const QJsonDocument document(mainObject);
    // TODO: switch back to binary format after testing
    //const auto data = document.toBinaryData();
    const auto data = document.toJson();
    const auto result = file.write(data);

    if (data.size() != result) {
        qWarning() << "Not all cache data has been saved." << data.size()
                   << "vs" << result;
    }
}

/*!
 * Checks if \a file has changed since last compilation. Returns true if it has,
 * or if it is not in cache.
 *
 * If returns true, \a file will be recompiled.
 */
bool ProjectManager::isFileDirty(const QString &file, const bool isQuickMode,
                                 const Scope &scope) const
{
    const QFileInfo realFile(file);

    if (!realFile.exists()) {
        qInfo() << "File has vanished!" << file;
    }

    const FileInfo cachedFile(scope.parsedFile(file));

    if (realFile.created() != cachedFile.dateCreated) {
        qDebug() << "Different creation date. Recompiling."
                 << file << realFile.created() << cachedFile.dateCreated;
        return true;
    }

    if (realFile.lastModified() != cachedFile.dateModified) {
        qDebug() << "Different modification date. Recompiling."
                 << file << realFile.lastModified() << cachedFile.dateModified;
        return true;
    }

    if (!isQuickMode) {
        // Deep verification is off - for now
//        QFile fileData(file);
//        fileData.open(QFile::ReadOnly | QFile::Text);
//        const auto checksum = QCryptographicHash::hash(fileData.readAll(), QCryptographicHash::Sha1);
//        if (checksum != cachedFile.checksum) {
//            qDebug() << "Different checksum. Recompiling."
//                     << file << checksum << cachedFile.checksum;
//            return true;
//        }
    }

    return false;
}

/*!
 * Load necessary build info from IBS cache file
 */
void ProjectManager::loadCache()
{
    QFile file(Tags::ibsCacheFileName);

    if (!file.exists()) {
        qInfo("Cannot find IBS cache file");
        return;
    }

    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qFatal("Could not open IBS cache file for reading!");
    }

    qInfo() << "Loading ibs cache file" << Tags::ibsCacheFileName;

    //const auto document = QJsonDocument::fromBinaryData(file.readAll());
    const auto document = QJsonDocument::fromJson(file.readAll());
    const QJsonObject mainObject(document.object());

    onTargetName(mainObject.value(Tags::targetName).toString());
    onTargetType(mainObject.value(Tags::targetType).toString());
    mTargetLibType = mainObject.value(Tags::targetLib).toString();
    mQtDir = mainObject.value(Tags::qtDir).toString();
    onQtModules(jsonArrayToStringList(mainObject.value(Tags::qtModules).toArray()));
    onDefines(jsonArrayToStringList(mainObject.value(Tags::defines).toArray()));
    //onIncludes(jsonArrayToStringList(mainObject.value(Tags::includes).toArray()));
    onLibs(jsonArrayToStringList(mainObject.value(Tags::libs).toArray()));

    const QJsonArray scopesArray = mainObject.value(Tags::scopes).toArray();
    for (const auto &scopeJson : scopesArray) {
        Scope scope = Scope::fromJson(scopeJson.toObject());
        mScopes.insert(scope.id(), scope);
    }

    if (mFlags.inputFile().isEmpty()) {
        mFlags.setInputFile(mainObject.value(Tags::inputFile).toString());
    } else {
        // TODO: if input file was specified and is diferent than the one in cache
        // we need to invalidate the cache!
    }


    mCacheEnabled = true;
}

void ProjectManager::loadCommands()
{
    if (mFlags.commands().isEmpty())
        return;

    qInfo() << "Loading commands from command line";

    CommandParser parser(mFlags.commands());
    connect(&parser, &CommandParser::error, this, &ProjectManager::error);
    connect(&parser, &CommandParser::targetName, this, &ProjectManager::onTargetName);
    connect(&parser, &CommandParser::targetType, this, &ProjectManager::onTargetType);
    connect(&parser, &CommandParser::qtModules, this, &ProjectManager::onQtModules);
    connect(&parser, &CommandParser::defines, this, &ProjectManager::onDefines);
    connect(&parser, &CommandParser::includes, this, &ProjectManager::onIncludes);
    connect(&parser, &CommandParser::libs, this, &ProjectManager::onLibs);
    connect(&parser, &CommandParser::runTool, this, &ProjectManager::onRunTool);
    parser.parse();
}

void ProjectManager::onParsed(const QByteArray &scopeId,
                              const QString &file, const QString &source,
                              const QByteArray &checksum,
                              const QDateTime &modified,
                              const QDateTime &created)
{
    // Update parsed file info
    Scope scope = mScopes.value(scopeId);
    FileInfo info = scope.parsedFile(file);
    info.type = FileInfo::Cpp;
    info.path = file;
    info.checksum = checksum;
    info.dateModified = modified;
    info.dateCreated = created;

    // Compile source file, if present
    if (!source.isEmpty() and source == file) {
        info.objectFile = compile(scopeId, source);
    }

    // TODO: switch to pointers and modify in-place?
    scope.insertParsedFile(info);
    mScopes.insert(scopeId, scope);
}

void ProjectManager::onParseRequest(const QByteArray &scopeId, const QString &file, const bool force)
{
    if (mIsError)
        return;

    Scope scope = mScopes.value(scopeId);

    // Skip files which we have parsed already
    if (!force and scope.isParsed(file))
        return;

    // Find file in include dirs
    const QString selectedFile(scope.findFile(file));

    if (selectedFile.isEmpty()) {
        qWarning() << "Could not find file:" << file;
        return;
    }

    // Skip again, because name could have changed
    if (!force and scope.isParsed(selectedFile))
        return;

    // Prevent file from being parsed twice
    FileInfo info;
    info.path = selectedFile;
    scope.insertParsedFile(info);
    mScopes.insert(scopeId, scope);
    parseFile(selectedFile);
}

bool ProjectManager::onRunMoc(const QByteArray &scopeId, const QString &file)
{
    if (mIsError)
        return false;

    if (mQtDir.isEmpty()) {
        emit error("Can't run MOC because Qt dir is not set. See 'ibs --help' for "
                   "more info.");
    }

    if (mQtIsMocInitialized == false) {
        if (initializeMoc(scopeId) == false)
            return false;
    }

    const QFileInfo header(file);
    const QString mocFile("moc_" + header.baseName() + ".cpp");
    const QString compiler(mQtDir + "/bin/moc");
    const QString predefs("moc_predefs.h");

    QStringList arguments;
    arguments.append(mQtDefines);
    arguments.append({ "--include", predefs });
    arguments.append(mQtIncludes);
    // TODO: GCC includes!
    arguments.append({ file, "-o", mocFile });

    MetaProcess mp;
    mp.file = mocFile;
    mp.dependsOn.append(findDependency(predefs));
    // Generate MOC file
    runProcess(compiler, arguments, mp);

    // TODO: use a RAII ScopeLocker to make sure we don't forget to re-insert
    // it into the hash
    Scope scope = mScopes.value(scopeId);
    FileInfo info = scope.parsedFile(file);
    info.path = mFlags.relativePath() + "/" + file;
    info.generatedFile = mocFile;
    // Compile MOC file
    info.generatedObjectFile = compile(scopeId, mocFile);

    // TODO: IMPORTANT! Old code used 'file' as key for parsed file hash here,
    // new code uses 'info.path' instead, and they are different!
    scope.insertParsedFile(info);
    mScopes.insert(scopeId, scope);
    return true;
}

void ProjectManager::onTargetName(const QString &target)
{
    qInfo() << "Setting target name:" << target;
    mTargetName = target;
}

void ProjectManager::onTargetType(const QString &type)
{
    qInfo() << "Setting target type:" << type;
    mTargetType = type;
}

void ProjectManager::onQtModules(const QStringList &modules)
{
    QStringList mod(mQtModules);
    mod += modules;
    mod.removeDuplicates();

    if (mod != mQtModules) {
        qInfo() << "Updating required Qt module list:" << mod;
        updateQtModules(mod);
    }
}

void ProjectManager::onDefines(const QStringList &defines)
{
    mCustomDefines += defines;
    mCustomDefines.removeDuplicates();
    for (const auto &define : qAsConst(mCustomDefines)) {
        const QString def("-D" + define);
        if (!mCustomDefineFlags.contains(def)) {
            mCustomDefineFlags.append(def);
        }
    }
    qInfo() << "Updating custom defines:" << mCustomDefines;
}

void ProjectManager::onIncludes(const QByteArray &scopeId, const QStringList &includes)
{
    Scope scope = mScopes.value(scopeId);
    scope.addIncludePaths(includes);
    mScopes.insert(scopeId, scope);
}

void ProjectManager::onLibs(const QStringList &libs)
{
    mCustomLibs += libs;
    mCustomLibs.removeDuplicates();
    qInfo() << "Updating custom libs:" << mCustomLibs;
}

void ProjectManager::onRunTool(const QByteArray &scopeId, const QString &tool,
                               const QStringList &args)
{
    if (mIsError)
        return;

    if (tool == Tags::rcc) {
        // -name qml qml.qrc -o qrc_qml.cpp
        for (const auto &qrcFile : qAsConst(args)) {
            const QFileInfo file(mFlags.relativePath() + "/" + qrcFile);
            const QString cppFile("qrc_" + file.baseName() + ".cpp");
            const QStringList arguments { "-name", file.baseName(),
                        mFlags.relativePath() + "/" + qrcFile,
                        "-o", cppFile };

            qDebug() << "Running tool: rcc" << mFlags.relativePath() + "/" + qrcFile << cppFile;

            MetaProcess mp;
            mp.file = cppFile;
            runProcess(mQtDir + "/bin/" + tool, arguments, mp);

            Scope scope = mScopes.value(scopeId);
            FileInfo info = scope.parsedFile(qrcFile);
            info.type = FileInfo::QRC;
            info.path = mFlags.relativePath() + "/" + qrcFile;
            info.dateModified = file.lastModified();
            info.dateCreated = file.created();
            info.generatedFile = cppFile;
            info.generatedObjectFile = compile(scopeId, cppFile);
            scope.insertParsedFile(info);
        }
    } else if (tool == Tags::uic) {
        // TODO: add uic support
    } else {
        // TODO: add any tool support
    }
}

void ProjectManager::onProcessErrorOccurred(QProcess::ProcessError _error)
{
    auto process = qobject_cast<QProcess *>(sender());

    if (process == nullptr) {
        emit error("Process finished, but could not be accessed. :-(");
        return;
    }

    // Remove process from the queue
    for (int i = 0; i < mProcessQueue.count(); ++i) {
        if (process == mProcessQueue.at(i).process) {
            mProcessQueue.remove(i);
            break;
        }
    }

    emit error(QString("Process %1:%2 error occurred: %3")
               .arg(process->program(),
                    QString::number(mRunningJobs.indexOf(process)),
                    QString(_error))
               );
}

void ProjectManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    auto process = qobject_cast<QProcess *>(sender());

    if (process == nullptr) {
        emit error("Process finished, but could not be accessed. :-(");
        return;
    }

    if (exitCode != 0) {
        emit error(QString("Process %1:%2 finished with exit code %3 and status %4")
                   .arg(process->program(),
                        QString::number(mRunningJobs.indexOf(process)),
                        QString::number(exitCode), QString::number(exitStatus)));
    }

    // Remove process from the queue
    for (int i = 0; i < mProcessQueue.count(); ++i) {
        if (process == mProcessQueue.at(i).process) {
            mProcessQueue.remove(i);
            break;
        }
    }

    mRunningJobs.removeOne(process);
    delete process; // Dangerous? Use QSharedPointer instead?
    runNextProcess();
}

/*!
 * Compiles \a file and returns the name of the generated object file.
 */
QString ProjectManager::compile(const QByteArray &scopeId, const QString &file)
{
    if (mIsError)
        return QString();

    const QFileInfo info(file);
    const QString objectFile(info.baseName() + ".o");
    const QString compiler("g++");

    if (!mQtModules.isEmpty()) {
        if (mQtDir.isEmpty()) {
            qFatal("Qt dir not set, but this is a Qt project! Specify Qt dir "
                   "with --qt-dir argument");
        }
    }

    const Scope scope = mScopes.value(scopeId);

    //qInfo() << "Compiling:" << file << "into:" << objectFile;
    // TODO: add ProjectManager class and schedule compilation there (threaded!).
    QStringList arguments { "-c", "-pipe", "-g", "-D_REENTRANT", "-fPIC", "-Wall", "-W", };

    arguments.append(mCustomDefineFlags); // TODO: scope!
    arguments.append(mQtDefines); // TODO: scope!
    arguments.append(mQtIncludes); // TODO: scope!
    arguments.append(scope.customIncludeFlags());
    arguments.append({ "-o", objectFile, file });

    MetaProcess mp;
    mp.file = objectFile;
    mp.dependsOn = findDependencies(file);

    runProcess(compiler, arguments, mp);
    return objectFile;
}

void ProjectManager::link(const QByteArray &scopeId)
{
    if (mIsError)
        return;

    const Scope scope = mScopes.value(scopeId);
    QStringList objectFiles;
    for (const auto &info : scope.parsedFiles()) {
        if (!info.objectFile.isEmpty())
            objectFiles.append(info.objectFile);
        if (!info.generatedObjectFile.isEmpty())
            objectFiles.append(info.generatedObjectFile);
    }

    // TODO: add dependent scopes (from other subprojects)

    //qInfo() << "Linking:" << objectFiles;
    const QString compiler("g++");
    QStringList arguments;

    if (mTargetType == Tags::targetLib) {
        if (mTargetLibType == Tags::targetLibDynamic) {
            arguments.append({ "-shared", "-Wl,-soname,lib" + mTargetName + ".so.1",
                               "-o", mFlags.prefix() + "/" + "lib" + mTargetName + ".so.1.0.0"});
        }
    } else {
        arguments.append({ "-o", mFlags.prefix() + "/" + mTargetName });
    }

    arguments.append(objectFiles);

    if (!mQtModules.isEmpty()) {
        if (mQtDir.isEmpty()) {
            qFatal("Qt dir not set, but this is a Qt project! Specify Qt dir "
                   "with --qt-dir argument");
        }

        arguments.append(mQtLibs);
    }

    arguments.append(mCustomLibs);

    MetaProcess mp;
    mp.file = mTargetName;
    mp.dependsOn = findAllDependencies();
    runProcess(compiler, arguments, mp);
}

void ProjectManager::parseFile(const QString &file)
{
    FileParser parser(file, mCustomIncludes);
    connect(&parser, &FileParser::error, this, &ProjectManager::error);
    connect(&parser, &FileParser::parsed, this, &ProjectManager::onParsed);
    connect(&parser, &FileParser::parseRequest, this, &ProjectManager::onParseRequest);
    connect(&parser, &FileParser::runMoc, this, &ProjectManager::onRunMoc);
    connect(&parser, &FileParser::targetName, this, &ProjectManager::onTargetName);
    connect(&parser, &FileParser::targetType, this, &ProjectManager::onTargetType);
    connect(&parser, &FileParser::qtModules, this, &ProjectManager::onQtModules);
    connect(&parser, &FileParser::defines, this, &ProjectManager::onDefines);
    connect(&parser, &FileParser::includes, this, &ProjectManager::onIncludes);
    connect(&parser, &FileParser::libs, this, &ProjectManager::onLibs);
    connect(&parser, &FileParser::runTool, this, &ProjectManager::onRunTool);

    parser.parse();
}

void ProjectManager::updateQtModules(const QStringList &modules)
{
    mQtModules = modules;
    mQtIncludes.clear();
    mQtLibs.clear();
    mQtDefines.clear();

    for(const QString &module : qAsConst(mQtModules)) {
        mQtDefines.append("-DQT_" + module.toUpper() + "_LIB");
    }

    mQtIncludes.append("-I" + mQtDir + "/include");
    mQtIncludes.append("-I" + mQtDir + "/mkspecs/linux-g++");

    // TODO: pre-capitalize module letters to do both loops faster
    for(const QString &module : qAsConst(mQtModules)) {
        const QString dir("-I" + mQtDir + "/include/Qt");
        if (module == Tags::quickcontrols2) {
            mQtIncludes.append(dir + "QuickControls2");
        } else if (module == Tags::quickwidgets) {
            mQtIncludes.append(dir + "QuickWidgets");
        } else {
            mQtIncludes.append(dir + capitalizeFirstLetter(module));
        }
    }

    mQtLibs.append("-Wl,-rpath," + mQtDir + "/lib");
    mQtLibs.append("-L" + mQtDir + "/lib");

    for(const QString &module : qAsConst(mQtModules)) {
        // TODO: use correct mkspecs
        // TODO: use qmake -query to get good paths
        const QString lib("-lQt5");
        if (module == Tags::quickcontrols2) {
            mQtLibs.append(lib + "QuickControls2");
        } else if (module == Tags::quickwidgets) {
            mQtLibs.append(lib + "QuickWidgets");
        } else {
            mQtLibs.append(lib + capitalizeFirstLetter(module));
        }
    }

    mQtLibs.append("-lpthread");
}

bool ProjectManager::initializeMoc(const QByteArray &scopeId)
{
    qInfo() << "Initializig MOC";
    const QString compiler("g++");
    const QString predefs("moc_predefs.h");
    const QStringList arguments({ "-pipe", "-g", "-Wall", "-W", "-dM", "-E",
                            "-o", predefs,
                            mQtDir + "/mkspecs/features/data/dummy.cpp" });

    FileInfo info;
    info.path = predefs;
    info.generatedFile = predefs;
    Scope scope = mScopes.value(scopeId);
    scope.insertParsedFile(info);
    mScopes.insert(scopeId, scope);
    // TODO: is predefs info lost? Check dependency resolving
    //mParsedFiles.insert(predefs, info);

    MetaProcess mp;
    mp.file = predefs;
    runProcess(compiler, arguments, mp);
    mQtIsMocInitialized = true;
    return mQtIsMocInitialized;
}

/*!
 * TODO: run asynchronously in a thread pool.
 */
void ProjectManager::runProcess(const QString &app, const QStringList &arguments,
                                MetaProcess mp)
{
    auto process = new QProcess();

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProjectManager::onProcessFinished);

    connect(process, &QProcess::errorOccurred,
            this, &ProjectManager::onProcessErrorOccurred);

    process->setProcessChannelMode(QProcess::ForwardedChannels);
    process->setProgram(app);
    process->setArguments(arguments);

    mp.process = process;
    mProcessQueue.append(mp);
    runNextProcess();
}

void ProjectManager::runNextProcess()
{
    // TODO: use these counts to create a progress bar as in cmake!
    //qDebug() << "Running jobs:" << mRunningJobs.count() << "max jobs:" << mFlags.jobs << "process queue" << mProcessQueue.count();

    // Run next process if max number of jobs is not exceeded
    if ((mProcessQueue.count() > 0) and (mRunningJobs.count() < mFlags.jobs())) {
        for (int i = 0; i < mProcessQueue.count() and (mRunningJobs.count() < mFlags.jobs()); ++i) {
            const auto & mp = mProcessQueue.at(i);
            if (!mp.canRun() or mp.process->state() == QProcess::Running
                    or mp.process->state() == QProcess::Starting) {
                continue;
            }

            mRunningJobs.append(mp.process);
            qInfo() << "Running next process:" << i << mRunningJobs.last()->program() << mRunningJobs.last()->arguments().join(" ");
            mRunningJobs.last()->start();
        }

        // Start working asap
        QCoreApplication::instance()->processEvents();
    }

    if (mProcessQueue.isEmpty())
        emit jobQueueEmpty();
}

ProcessPtr ProjectManager::findDependency(const QString &file) const
{
    for (const MetaProcess &mp : qAsConst(mProcessQueue)) {
        if (mp.file == file)
            return mp.process;
    }

    return ProcessPtr();
}

QVector<ProcessPtr> ProjectManager::findDependencies(const QString &file) const
{
    QString dependencies;
    QVector<ProcessPtr> result;
    for (const MetaProcess &mp : qAsConst(mProcessQueue)) {
        if (mp.file == file) {
            dependencies += mp.process->arguments().last() + ", ";
            result.append(mp.process);
        }
    }

    //qDebug() << "File" << file << "depends on:" << dependencies;

    return result;
}

QVector<ProcessPtr> ProjectManager::findAllDependencies() const
{
    QVector<ProcessPtr> result;
    for (const MetaProcess &mp : qAsConst(mProcessQueue)) {
        result.append(mp.process);
    }

    return result;
}

QString ProjectManager::capitalizeFirstLetter(const QString &string) const
{
    return (string[0].toUpper() + string.mid(1));
}

//QString ProjectManager::findFile(const QString &file, const QStringList &includeDirs) const
//{
//    QString result;
//    if (file.contains(mFlags.relativePath()))
//        result = file;
//    else
//        result = mFlags.relativePath() + "/" + file;

//    // Search through include paths
//    if (QFileInfo(result).exists()) {
//        //qDebug() << "RETURNING:" << result;
//        return result;
//    }

//    for (const QString &inc : qAsConst(includeDirs)) {
//        const QString tempResult(mFlags.relativePath() + "/" + inc + "/" + file);
//        if (QFileInfo(tempResult).exists()) {
//            result = tempResult;
//            break;
//        }
//    }

//    //qDebug() << "FOUND:" << result;
//    return result;
//}

QStringList ProjectManager::jsonArrayToStringList(const QJsonArray &array) const
{
    QStringList result;

    for (const auto &value : array) {
        result.append(value.toString());
    }

    return result;
}

void ProjectManager::removeFile(const QString &path) const
{
    if (QFile::exists(path)) {
        qInfo() << "Removing:" << path;
        QFile::remove(path);
    }
}
