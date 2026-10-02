#include "library_bridge.hpp"
#include "test_support.hpp"

#include <QDir>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(TyliPlugin)

using namespace tyli;
using namespace tyli::test;

namespace {

// Every QML warning (binding errors, type errors, missing properties) fails the test.
QStringList qmlWarnings;
QtMessageHandler previousHandler = nullptr;

void collectWarnings(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    const bool fromQml = (context.file && QString::fromUtf8(context.file).contains(".qml")) ||
                         QString::fromUtf8(context.category).startsWith("qml") ||
                         QString::fromUtf8(context.category).startsWith("js") || message.contains(".qml:");
    if (type >= QtWarningMsg && fromQml) qmlWarnings << message;
    previousHandler(type, context, message);
}

}  // namespace

// Loads Main.qml offscreen over a seeded library, visits every tab and Settings
// sub-tab, and saves a screenshot of each to build/screenshots/ for review.
class TestQmlSmoke : public QObject {
    Q_OBJECT

private:
    QTemporaryDir data_;
    QTemporaryDir media_;
    std::unique_ptr<LibraryBridge> bridge_;
    std::unique_ptr<QQmlApplicationEngine> engine_;
    QQuickWindow* window_ = nullptr;

    void screenshot(const QString& name) {
        QTest::qWait(50);  // let bindings and layouts settle
        const QImage image = window_->grabWindow();
        QVERIFY(!image.isNull());
        QDir().mkpath(TYLI_SCREENSHOT_DIR);
        QVERIFY(image.save(QStringLiteral(TYLI_SCREENSHOT_DIR "/%1.png").arg(name)));
    }

    // Items made by a Repeater aren't QObject children of the window, so walk the visual tree too.
    static QObject* findIn(QQuickItem* item, const QString& objectName) {
        if (item->objectName() == objectName) return item;
        if (QObject* child = item->findChild<QObject*>(objectName)) return child;
        for (QQuickItem* child : item->childItems()) {
            if (QObject* found = findIn(child, objectName)) return found;
        }
        return nullptr;
    }

    QObject* find(const char* objectName) {
        QObject* object = window_->findChild<QObject*>(objectName);
        if (!object) object = findIn(window_->contentItem(), objectName);
        if (!object) qWarning("no object named %s", objectName);
        return object;
    }

private slots:
    void initTestCase() {
        QQuickStyle::setStyle("Fusion");
        previousHandler = qInstallMessageHandler(collectWarnings);

        bridge_ = std::make_unique<LibraryBridge>(data_.path());
        LibraryBridge::setInstance(bridge_.get());
        QVERIFY(bridge_->ready());

        // A seeded library: two music folders (one goes offline, one has held removals), one film folder.
        writeTree(media_.filePath("Music"), 450);
        writeTree(media_.filePath("Drive/Live"), 10);
        writeFile(media_.filePath("Films/Film (2020)/Film.mkv"), 4096);
        writeFile(media_.filePath("Films/Film (2020)/Film.en.srt"), 64);
        const int music = categoryId(*bridge_, "Music");
        QVERIFY(addFolder(*bridge_, music, media_.filePath("Music"), "Music (E:)").first);
        QVERIFY(addFolder(*bridge_, music, media_.filePath("Drive/Live"), "Live (F:)").first);
        QVERIFY(addFolder(*bridge_, categoryId(*bridge_, "Movies"), media_.filePath("Films"), "Films").first);
        QVERIFY(!sync(*bridge_, -1).isEmpty());
        for (int i = 0; i < 200; ++i) {
            QFile::remove(media_.filePath(QStringLiteral("Music/album %1/track %2.flac").arg(i / 100).arg(i, 5, 10, QLatin1Char('0'))));
        }
        QVERIFY(QDir().rename(media_.filePath("Drive"), media_.filePath("Unplugged")));
        QVERIFY(!sync(*bridge_, music).isEmpty());

        engine_ = std::make_unique<QQmlApplicationEngine>();
        engine_->loadFromModule("Tyli", "Main");
        QCOMPARE(engine_->rootObjects().size(), 1);
        window_ = qobject_cast<QQuickWindow*>(engine_->rootObjects().first());
        QVERIFY(window_);
        QVERIFY(QTest::qWaitForWindowExposed(window_));
    }

    void cleanupTestCase() {
        engine_.reset();
        bridge_.reset();
        qInstallMessageHandler(previousHandler);
    }

    void everySectionLoads() {
        QObject* tabs = find("mainTabs");
        QVERIFY(tabs);
        const QVariantList categories = bridge_->categories();
        QVERIFY(categories.size() >= 2);
        for (int i = 0; i < categories.size(); ++i) {
            tabs->setProperty("currentIndex", i);
            screenshot(QStringLiteral("section-%1-%2").arg(i).arg(categories[i].toMap().value("name").toString()));
        }
        tabs->setProperty("currentIndex", 0);
        // The file list of the first Music folder pages from the database.
        QObject* fileList = find("fileList");
        QVERIFY(fileList);
        QTRY_VERIFY(fileList->property("count").toInt() > 0);
        QVERIFY(qmlWarnings.isEmpty());
    }

    void everySettingsTabLoads() {
        QObject* settingsButton = find("settingsButton");
        QObject* subTabs = find("settingsTabs");
        QVERIFY(settingsButton && subTabs);
        settingsButton->setProperty("checked", true);
        const QStringList names{"folders", "categories", "scanning", "sync"};
        for (int i = 0; i < names.size(); ++i) {
            subTabs->setProperty("currentIndex", i);
            screenshot("settings-" + names[i]);
        }
        settingsButton->setProperty("checked", false);
        QVERIFY(qmlWarnings.isEmpty());
    }

    void aNewCategoryAppearsAsATab() {
        QVERIFY(bridge_->addCategory("Podcasts", {"audio"}));
        QObject* tabs = find("mainTabs");
        QTRY_COMPARE(tabs->property("count").toInt(), bridge_->categories().size());
        QVERIFY(qmlWarnings.isEmpty());
    }

    void noQmlWarnings() {
        if (!qmlWarnings.isEmpty()) qWarning().noquote() << qmlWarnings.join("\n");
        QVERIFY(qmlWarnings.isEmpty());
    }
};

QTEST_MAIN(TestQmlSmoke)
#include "tst_qml_smoke.moc"
