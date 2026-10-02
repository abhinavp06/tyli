#include "app_setup.hpp"
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
        QTest::qWait(80);  // let bindings and layouts settle
        const QImage image = window_->grabWindow().convertToFormat(QImage::Format_RGB32);
        QVERIFY(!image.isNull());
        QDir().mkpath(TYLI_SCREENSHOT_DIR);
        QVERIFY(image.save(QStringLiteral(TYLI_SCREENSHOT_DIR "/%1.png").arg(name)));

        // The theme is pure black (features/theme.md §2): text, lines, and icons are the
        // only non-black pixels, so a grey surface sneaking back in fails here.
        qint64 black = 0;
        for (int y = 0; y < image.height(); ++y) {
            const QRgb* line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
            for (int x = 0; x < image.width(); ++x) black += (line[x] & 0xFFFFFF) == 0;
        }
        const double share = double(black) / (qint64(image.width()) * image.height());
        QVERIFY2(share > 0.80, qPrintable(QStringLiteral("%1: only %2% of the pixels are pure black").arg(name).arg(share * 100, 0, 'f', 1)));
    }

    void setState(const char* objectName, const char* property, const QVariant& value) {
        QObject* object = find(objectName);
        QVERIFY(object);
        QVERIFY(object->setProperty(property, value));
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
        setupApplication();
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

    void everyCategoryViewLoads() {
        const QVariantList categories = bridge_->categories();
        QVERIFY(categories.size() >= 2);
        for (const QVariant& c : categories) {
            const QVariantMap category = c.toMap();
            QVERIFY(window_->setProperty("currentCategoryId", category.value("id")));
            screenshot("category-" + category.value("name").toString().toLower());
        }
        // Music's files, across its two folders (one offline), page from the database.
        QVERIFY(window_->setProperty("currentCategoryId", categoryId(*bridge_, "Music")));
        QObject* fileList = find("fileList");
        QVERIFY(fileList);
        QTRY_VERIFY(fileList->property("count").toInt() > 0);
        QVERIFY(qmlWarnings.isEmpty());
    }

    void theSidebarCollapses() {
        QVERIFY(window_->setProperty("sidebarCollapsed", true));
        screenshot("sidebar-collapsed");
        QVERIFY(window_->setProperty("sidebarCollapsed", false));
        QVERIFY(qmlWarnings.isEmpty());
    }

    void settingsLibraryLoads() {
        QVERIFY(window_->setProperty("settingsOpen", true));
        setState("settingsView", "currentPage", "library");
        setState("librarySettings", "selectedCategoryId", -1);
        screenshot("settings-library-all");
        setState("librarySettings", "selectedCategoryId", categoryId(*bridge_, "Music"));
        screenshot("settings-library-music");

        // A folder's details open inline, and the add-folder row.
        const int musicFolder = folder(*bridge_, "Music (E:)").value("id").toInt();
        setState("librarySettings", "expanded", QVariantMap{{QString::number(musicFolder), true}});
        setState("librarySettings", "addingFolder", true);
        screenshot("settings-library-details-and-add");
        setState("librarySettings", "addingFolder", false);
        QVERIFY(qmlWarnings.isEmpty());
    }

    void settingsGeneralLoads() {
        QVERIFY(window_->setProperty("settingsOpen", true));
        setState("settingsView", "currentPage", "general");
        screenshot("settings-general");
        QVERIFY(window_->setProperty("settingsOpen", false));
        QVERIFY(qmlWarnings.isEmpty());
    }

    void aNewCategoryAppearsInTheSidebar() {
        QVERIFY(bridge_->addCategory("Podcasts", {"audio"}));
        QObject* list = find("categoryList");
        QVERIFY(list);
        QTRY_COMPARE(list->property("count").toInt(), bridge_->categories().size());
        QVERIFY(qmlWarnings.isEmpty());
    }

    void noQmlWarnings() {
        if (!qmlWarnings.isEmpty()) qWarning().noquote() << qmlWarnings.join("\n");
        QVERIFY(qmlWarnings.isEmpty());
    }
};

QTEST_MAIN(TestQmlSmoke)
#include "tst_qml_smoke.moc"
