#pragma once

#include "library_bridge.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTest>

// Helpers shared by the suites: media trees on disk, and waiting for the bridge's
// background work.
namespace tyli::test {

inline void writeFile(const QString& path, qint64 size = 16) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(path));
    file.write(QByteArray(int(size), 'x'));
}

// `count` .flac files under `root`, 100 per sub-folder.
inline void writeTree(const QString& root, int count) {
    for (int i = 0; i < count; ++i) {
        writeFile(QStringLiteral("%1/album %2/track %3.flac").arg(root).arg(i / 100).arg(i, 5, 10, QLatin1Char('0')));
    }
}

inline int categoryId(LibraryBridge& bridge, const QString& name) {
    for (const QVariant& c : bridge.categories()) {
        if (c.toMap().value("name").toString() == name) return c.toMap().value("id").toInt();
    }
    return -1;
}

inline QVariantMap folder(LibraryBridge& bridge, const QString& name) {
    for (const QVariant& f : bridge.folders()) {
        if (f.toMap().value("name").toString() == name) return f.toMap();
    }
    return {};
}

// addFolder, waiting for its result. Returns {ok, message}.
inline std::pair<bool, QString> addFolder(LibraryBridge& bridge, int categoryId, const QString& path,
                                          const QString& name = {}, bool useMarker = true) {
    QSignalSpy done(&bridge, &LibraryBridge::folderOperationFinished);
    bridge.addFolder(categoryId, path, name, useMarker);
    if (!done.wait(10000)) return {false, QStringLiteral("timed out")};
    return {done.first().at(0).toBool(), done.first().at(1).toString()};
}

// requestSync, waiting until the bridge is idle again. Returns the report.
inline QVariantMap sync(LibraryBridge& bridge, int categoryId) {
    QSignalSpy finished(&bridge, &LibraryBridge::syncFinished);
    if (!bridge.requestSync(categoryId)) return {};
    if (!finished.wait(20000)) return {};
    if (!QTest::qWaitFor([&] { return !bridge.syncing(); }, 5000)) return {};
    return finished.first().at(0).toMap();
}

inline QVariantMap folderReport(const QVariantMap& report, int index = 0) {
    return report.value("folders").toList().value(index).toMap();
}

}
