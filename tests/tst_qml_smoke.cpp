#include "app_setup.hpp"
#include "library_bridge.hpp"
#include "test_support.hpp"
#include "thumbnail_cache.hpp"

#include <QDir>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QQuickView>
#include <QScopeGuard>
#include <QStyleHints>
#include <QWheelEvent>
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

// Loads Main.qml offscreen over a seeded library (files on disk, made-up metadata),
// visits every category, media page, and Settings page, and saves a screenshot of each
// to build/screenshots/ for review.
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

    // Pages you came from stay alive, hidden, under the one shown (CategoryView's stack), so
    // a visible match wins over a hidden one with the same name.
    QObject* find(const char* objectName) {
        QObject* hidden = nullptr;
        for (QObject* object : window_->findChildren<QObject*>(objectName)) {
            const auto* item = qobject_cast<QQuickItem*>(object);
            if (!item || item->isVisible()) return object;
            if (!hidden) hidden = object;
        }
        QObject* object = findIn(window_->contentItem(), objectName);
        if (!object) object = hidden;
        if (!object) qWarning("no object named %s", objectName);
        return object;
    }

    // A QML function on a named object (e.g. CategoryView.open(page, params)).
    void call(const char* objectName, const char* function, const QVariant& a = {}, const QVariant& b = {}) {
        QObject* object = find(objectName);
        QVERIFY(object);
        if (!a.isValid()) QVERIFY(QMetaObject::invokeMethod(object, function));
        else QVERIFY(QMetaObject::invokeMethod(object, function, Q_ARG(QVariant, a), Q_ARG(QVariant, b)));
    }

    void showCategory(const QString& name) {
        QVERIFY(window_->setProperty("settingsOpen", false));
        QVERIFY(window_->setProperty("currentCategoryId", categoryId(*bridge_, name)));
        call("categoryView", "home");  // also when it was already the category shown
        QTest::qWait(50);
    }

    static void writeImage(const QString& path, QColor color) {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QImage image(64, 96, QImage::Format_RGB32);
        image.fill(color);
        QVERIFY(image.save(path));
    }

private slots:
    void initTestCase() {
        setupApplication();
        QStandardPaths::setTestModeEnabled(true);  // preferences live in the temporary data folder anyway
        previousHandler = qInstallMessageHandler(collectWarnings);

        bridge_ = std::make_unique<LibraryBridge>(data_.path());
        LibraryBridge::setInstance(bridge_.get());
        QVERIFY(bridge_->ready());

        // Music: curated albums (one in two qualities, one with a cover image), plus a bulk folder
        // whose removals get held, and a folder that goes offline.
        const QStringList albums{"Radiohead/A Moon Shaped Pool", "Radiohead/A Moon Shaped Pool [24-96]", "Radiohead/In Rainbows",
                                 "Björk/Homogenic", "Massive Attack/Mezzanine", "Pink Floyd/The Dark Side of the Moon"};
        for (const QString& album : albums) {
            for (int i = 1; i <= 6; ++i) writeFile(media_.filePath(QStringLiteral("Music/%1/%2.flac").arg(album).arg(i, 2, 10, QLatin1Char('0'))));
        }
        writeImage(media_.filePath("Music/Radiohead/A Moon Shaped Pool/cover.png"), QColor(150, 110, 60));
        writeTree(media_.filePath("Music/bulk"), 300);
        writeTree(media_.filePath("Drive/Live"), 10);
        // Movies (one in two qualities, with a poster) and a show with two seasons.
        writeFile(media_.filePath("Films/Arrival (2016)/Arrival 1080p.mkv"), 4096);
        writeFile(media_.filePath("Films/Arrival (2016)/Arrival 2160p.mkv"), 8192);
        writeImage(media_.filePath("Films/Arrival (2016)/poster.png"), QColor(40, 70, 120));
        writeFile(media_.filePath("Films/Heat (1995)/Heat.mkv"), 4096);
        for (const char* episode : {"Season 1/Dark S01E01.mkv", "Season 1/Dark S01E02.mkv", "Season 1/Dark S01E03.mkv",
                                    "Season 2/Dark S02E01.mkv", "Season 2/Dark S02E02.mkv"}) {
            writeFile(media_.filePath(QStringLiteral("TV/Dark/") + QLatin1String(episode)), 2048);
        }
        writeImage(media_.filePath("TV/Dark/poster.png"), QColor(30, 30, 30));

        const int music = categoryId(*bridge_, "Music");
        QVERIFY(addFolder(*bridge_, music, media_.filePath("Music"), "Music (E:)").first);
        QVERIFY(addFolder(*bridge_, music, media_.filePath("Drive/Live"), "Live (F:)").first);
        QVERIFY(addFolder(*bridge_, categoryId(*bridge_, "Movies"), media_.filePath("Films"), "Films").first);
        QVERIFY(addFolder(*bridge_, categoryId(*bridge_, "Shows"), media_.filePath("TV"), "TV").first);
        QVERIFY(!sync(*bridge_, -1).isEmpty());

        // Metadata, as the metadata pass would store it.
        seedAudio(bridge_->databasePath(), [](const hoardor::file::Entry& e) -> std::optional<hoardor::audio::TrackInfo> {
            const QString path = QString::fromStdString(e.relative_path);
            const QStringList parts = path.split('/');
            if (parts.size() != 3 || parts[0] == "bulk") return std::nullopt;
            const bool hires = parts[1].endsWith("[24-96]");
            const std::string album = QString(parts[1]).remove(" [24-96]").toStdString();
            const int number = QFileInfo(parts[2]).baseName().toInt();
            const int year = album == "A Moon Shaped Pool" ? 2016 : album == "In Rainbows" ? 2007 : album == "Homogenic" ? 1997
                           : album == "Mezzanine" ? 1998 : 1973;
            return track(parts[0].toStdString(), album, number <= 3 ? 1 : 2, number, "Track " + std::to_string(number), year,
                         hires ? 24 : 16, hires ? 96000 : 44100);
        });
        seedVideo(bridge_->databasePath(), [](const hoardor::file::Entry& e, const std::vector<hoardor::file::Entry>& companions)
                                               -> std::optional<std::pair<hoardor::video::VideoInfo, qint64>> {
            hoardor::video::VideoInfo v;
            const QString path = QString::fromStdString(e.relative_path);
            qint64 poster = 0;
            for (const auto& c : companions) {
                if (QString::fromStdString(c.relative_path).endsWith("poster.png")) poster = c.id;
            }
            v.duration_ms = 3'000'000;
            v.width = 1920;
            v.height = 1080;
            v.video_codec = "h264";
            if (path.startsWith("Arrival")) {
                v.title = "Arrival";
                v.year = 2016;
                v.genres = {"Science Fiction", "Drama"};
                v.directors = {"Denis Villeneuve"};
                v.plot = "A linguist works with the military to communicate with alien lifeforms.";
                v.source = hoardor::video::Source::Nfo;
                if (path.contains("2160p")) {
                    v.width = 3840;
                    v.height = 2160;
                    v.hdr = "HDR10";
                    v.video_codec = "hevc";
                }
            } else if (path.startsWith("Heat")) {
                v.title = "Heat";
                v.year = 1995;
                v.directors = {"Michael Mann"};
            } else {
                v.type = hoardor::video::Type::Episode;
                v.show = "Dark";
                v.season = path.contains("Season 2") ? 2 : 1;
                v.episode = QFileInfo(path).baseName().right(2).toInt();
                v.title = "Episode " + std::to_string(v.episode);
                v.year = 2017;
                v.genres = {"Mystery"};
                v.directors = {"Baran bo Odar"};
            }
            return std::make_pair(v, poster);
        });
        // Thumbnails, as after a metadata pass (synchronously here).
        const auto made = ThumbnailCache::build(bridge_->thumbnails()->folder(), bridge_->databasePath());
        QVERIFY2(made.made >= 3, qPrintable(QStringLiteral("made %1 failed %2 files %3").arg(made.made).arg(made.failed).arg(QDir(bridge_->thumbnails()->folder()).entryList(QDir::Files).join(","))));  // the album cover and two posters
        emit bridge_->thumbnails()->changed();
        emit bridge_->contentChanged();

        // Held removals in Music (bulk), and the live folder goes offline.
        for (int i = 0; i < 200; ++i) {
            QFile::remove(media_.filePath(QStringLiteral("Music/bulk/album %1/track %2.flac").arg(i / 100).arg(i, 5, 10, QLatin1Char('0'))));
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
        QVERIFY(qmlWarnings.isEmpty());
    }

    void musicAlbumsInEverySort() {
        showCategory("Music");
        QObject* grid = find("albumGrid");
        QVERIFY(grid);
        QTRY_COMPARE(grid->property("count").toInt(), 5);  // the two copies of one album are one card
        for (const char* sort : {"az", "year", "artist", "added"}) {
            setState("musicView", "sortKey", sort);
            screenshot(QStringLiteral("music-sort-%1").arg(sort));
        }
        setState("musicView", "sortKey", "artist");
        QObject* sections = find("artistSections");
        QVERIFY(sections);
        QTRY_COMPARE(sections->property("count").toInt(), 4);
        setState("musicView", "sortKey", "added");
        QVERIFY(qmlWarnings.isEmpty());
    }

    void anAlbumWithTwoQualities() {
        showCategory("Music");
        call("categoryView", "open", "album", QVariantMap{{"albumArtist", "Radiohead"}, {"album", "A Moon Shaped Pool"}, {"crumb", "A Moon Shaped Pool"}});
        QTRY_VERIFY(find("albumView"));
        QObject* picker = find("qualityPicker");
        QVERIFY(picker);
        const QVariantList options = picker->property("options").toList();
        QCOMPARE(options.size(), 2);
        QCOMPARE(options[0].toMap().value("label").toString(), QStringLiteral("24/96 FLAC"));  // best first
        screenshot("album");
        QVERIFY(QMetaObject::invokeMethod(picker, "picked", Q_ARG(int, 1)));  // as a click in the menu would
        QTRY_COMPARE(find("albumView")->property("copyIndex").toInt(), 1);
        emit bridge_->contentChanged();  // new content (tags read): the choice stays
        QTest::qWait(50);
        QCOMPARE(find("albumView")->property("copyIndex").toInt(), 1);
        screenshot("album-16-44");
        QVERIFY(qmlWarnings.isEmpty());
    }

    void artistGenreAndYearPages() {
        showCategory("Music");
        call("categoryView", "open", "artist", QVariantMap{{"name", "Radiohead"}, {"crumb", "Radiohead"}});
        QTRY_VERIFY(find("artistView"));
        screenshot("artist");
        call("categoryView", "open", "genre", QVariantMap{{"value", "Art Rock"}, {"crumb", "Art Rock"}});
        QTRY_VERIFY(find("musicFilterView"));
        screenshot("genre");
        call("categoryView", "open", "year", QVariantMap{{"value", 2016}, {"crumb", "2016"}});
        screenshot("year");
        call("categoryView", "back");
        call("categoryView", "back");
        QTRY_VERIFY(find("artistView"));
        QVERIFY(qmlWarnings.isEmpty());
    }

    // Going back (the back arrow, a breadcrumb, a mouse side button) shows the page exactly as
    // it was left: the same grid object, scrolled where it was. Forward brings the page back.
    void backKeepsThePageAndItsScrollPosition() {
        const QSize size = window_->size();
        window_->resize(640, 420);  // small enough for the album grid to scroll
        const auto restore = qScopeGuard([&] { window_->resize(size); });
        showCategory("Music");
        auto* grid = qobject_cast<QQuickItem*>(find("albumGrid"));
        QVERIFY(grid);
        QTRY_COMPARE(grid->property("count").toInt(), 5);
        QTRY_VERIFY(grid->property("contentHeight").toReal() > grid->height() + 60);
        grid->setProperty("contentY", 60);
        QObject* view = find("categoryView");

        call("categoryView", "open", "album", QVariantMap{{"albumArtist", "Radiohead"}, {"album", "A Moon Shaped Pool"}, {"crumb", "A Moon Shaped Pool"}});
        QTRY_VERIFY(find("albumView"));
        QVERIFY(!grid->isVisible());
        call("categoryView", "back");
        QTRY_VERIFY(grid->isVisible());
        QCOMPARE(find("albumGrid"), grid);  // the same page, not a new one
        QCOMPARE(grid->property("contentY").toReal(), 60.0);

        // Forward reopens the album; a breadcrumb back home keeps the grid too.
        QVERIFY(view->property("canGoForward").toBool());
        call("categoryView", "forward");
        QTRY_VERIFY(find("albumView") && qobject_cast<QQuickItem*>(find("albumView"))->isVisible());
        QVERIFY(!view->property("canGoForward").toBool());
        const QVariant homeCrumb = QVariantMap{{"depth", 0}};
        QVERIFY(QMetaObject::invokeMethod(view, "goToCrumb", Q_ARG(QVariant, homeCrumb)));
        QTRY_VERIFY(grid->isVisible());
        QCOMPARE(grid->property("contentY").toReal(), 60.0);

        // Clicking the category in the sidebar goes home the same way.
        call("categoryView", "forward");
        QTRY_VERIFY(!grid->isVisible());
        auto* sidebarList = qobject_cast<QQuickItem*>(find("categoryList"));
        QVERIFY(sidebarList);
        QVERIFY(QMetaObject::invokeMethod(sidebarList->parentItem(), "categoryClicked", Q_ARG(int, categoryId(*bridge_, "Music"))));
        QTRY_VERIFY(grid->isVisible());
        QCOMPARE(grid->property("contentY").toReal(), 60.0);

        // A new page clears forward, as in a browser.
        call("categoryView", "open", "artist", QVariantMap{{"name", "Radiohead"}, {"crumb", "Radiohead"}});
        QVERIFY(!view->property("canGoForward").toBool());
        call("categoryView", "back");
        QVERIFY(qmlWarnings.isEmpty());
    }

    // The mouse's side buttons (and Alt+← / Alt+→) go back and forward from anywhere in the window.
    void mouseSideButtonsGoBackAndForward() {
        showCategory("Music");
        QObject* view = find("categoryView");
        call("categoryView", "open", "artist", QVariantMap{{"name", "Radiohead"}, {"crumb", "Radiohead"}});
        QTRY_VERIFY(find("artistView"));
        const QPoint middle(window_->width() / 2, window_->height() / 2);  // over the page's content
        QTest::mouseClick(window_, Qt::BackButton, {}, middle);
        QTRY_COMPARE(view->property("routes").toList().size(), 1);
        QTest::mouseClick(window_, Qt::ForwardButton, {}, middle);
        QTRY_COMPARE(view->property("routes").toList().size(), 2);
        QTest::keyClick(window_, Qt::Key_Left, Qt::AltModifier);
        QTRY_COMPARE(view->property("routes").toList().size(), 1);
        QTest::keyClick(window_, Qt::Key_Right, Qt::AltModifier);
        QTRY_COMPARE(view->property("routes").toList().size(), 2);

        // From Settings, back returns to the page you were on.
        QVERIFY(window_->setProperty("settingsOpen", true));
        QTest::mouseClick(window_, Qt::BackButton, {}, middle);
        QTRY_VERIFY(!window_->property("settingsOpen").toBool());
        QCOMPARE(view->property("routes").toList().size(), 2);
        QVERIFY(qmlWarnings.isEmpty());
    }

    // One wheel notch scrolls the "scroll speed" preference times Qt's default (2× unless set).
    void theWheelScrollsAsFarAsTheSettingSays() {
        QQuickView view;
        QQmlComponent component(view.engine());
        component.setData("import QtQuick\nListView { width: 300; height: 300; model: 500; delegate: Item { width: 300; height: 40 } }", QUrl());
        auto* list = qobject_cast<QQuickItem*>(component.create());
        QVERIFY(list);
        view.setContent(QUrl(), &component, list);
        view.resize(300, 300);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        const QPointF at(150, 150);
        QWheelEvent notch(at, view.mapToGlobal(at), QPoint(), QPoint(0, -120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QGuiApplication::sendEvent(&view, &notch);
        const qreal plain = QGuiApplication::styleHints()->wheelScrollLines() * 24;  // Qt's distance per notch
        QCOMPARE(wheelSpeed(), 2.0);
        QTRY_COMPARE(qRound(list->property("contentY").toReal()), qRound(plain * wheelSpeed()));
    }

    // tyli_ opens the (blank, for now) home page; a category or back leaves it.
    void theWordmarkOpensHome() {
        showCategory("Music");
        QObject* wordmark = find("wordmark");
        QVERIFY(wordmark);
        auto* item = qobject_cast<QQuickItem*>(wordmark);
        QTest::mouseClick(window_, Qt::LeftButton, {}, item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint());
        QTRY_VERIFY(window_->property("homeOpen").toBool());
        QVERIFY(qobject_cast<QQuickItem*>(find("homeView"))->isVisible());
        QVERIFY(!qobject_cast<QQuickItem*>(find("categoryView"))->isVisible());
        screenshot("home");
        QTest::mouseClick(window_, Qt::BackButton, {}, QPoint(window_->width() / 2, window_->height() / 2));
        QTRY_VERIFY(!window_->property("homeOpen").toBool());
        QVERIFY(qobject_cast<QQuickItem*>(find("categoryView"))->isVisible());
        QVERIFY(qmlWarnings.isEmpty());
    }

    void moviesAMovieAndADirector() {
        showCategory("Movies");
        QObject* grid = find("posterGrid");
        QVERIFY(grid);
        QTRY_COMPARE(grid->property("count").toInt(), 2);  // Arrival's two copies are one card
        screenshot("movies");
        call("categoryView", "open", "movie", QVariantMap{{"title", "Arrival"}, {"year", 2016}, {"crumb", "Arrival"}});
        QTRY_VERIFY(find("movieView"));
        const QVariantList options = find("qualityPicker")->property("options").toList();
        QCOMPARE(options.size(), 2);
        QVERIFY(options[0].toMap().value("label").toString().startsWith("2160p"));
        screenshot("movie");
        call("categoryView", "open", "director", QVariantMap{{"name", "Denis Villeneuve"}, {"crumb", "Denis Villeneuve"}});
        QTRY_VERIFY(find("personView"));
        screenshot("director");
        QVERIFY(qmlWarnings.isEmpty());
    }

    void showsAndAShow() {
        showCategory("Shows");
        QObject* grid = find("posterGrid");
        QVERIFY(grid);
        QTRY_COMPARE(grid->property("count").toInt(), 1);
        screenshot("shows");
        call("categoryView", "open", "show", QVariantMap{{"show", "Dark"}, {"crumb", "Dark"}});
        QTRY_VERIFY(find("showView"));
        QObject* episodes = find("episodeList");
        QVERIFY(episodes);
        QTRY_COMPARE(episodes->property("count").toInt(), 3);  // season 1
        screenshot("show");
        setState("showView", "seasonIndex", 1);
        QTRY_COMPARE(episodes->property("count").toInt(), 2);
        QVERIFY(qmlWarnings.isEmpty());
    }

    void searchAcrossTheLibrary() {
        showCategory("Music");
        QVERIFY(window_->setProperty("searchText", "radio"));
        QTRY_VERIFY(find("searchView") && find("searchView")->property("found").toInt() > 0);
        screenshot("search");
        QVERIFY(window_->setProperty("searchText", "arri"));  // a prefix of "Arrival"
        QTRY_VERIFY(find("searchView")->property("found").toInt() > 0);
        screenshot("search-movie");
        QVERIFY(window_->setProperty("searchText", "zzzz"));
        QTRY_COMPARE(find("searchView")->property("found").toInt(), 0);
        QVERIFY(window_->setProperty("searchText", ""));
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
        // Further down: sync, interface (scroll speed), track columns.
        auto* speed = qobject_cast<QQuickItem*>(find("scrollSpeed"));
        QVERIFY(speed);
        QObject* flick = speed;
        while (flick && !flick->inherits("QQuickFlickable")) flick = flick->parent();
        QVERIFY(flick);
        flick->setProperty("contentY", speed->mapToItem(qobject_cast<QQuickItem*>(flick)->property("contentItem").value<QQuickItem*>(), QPointF()).y() - 300);
        screenshot("settings-general-lower");

        // A track column toggles from a click on its name, not only on the [ ] (the user, 2026-10-03).
        auto* size = qobject_cast<QQuickItem*>(find("column-size"));
        QVERIFY(size);
        QVERIFY(!bridge_->trackColumns().contains("size"));
        const QPoint onName = size->mapToScene(QPointF(size->width() - 10, size->height() / 2)).toPoint();
        QVERIFY(window_->contentItem()->contains(onName));
        QTest::mouseClick(window_, Qt::LeftButton, {}, onName);
        QTRY_VERIFY(bridge_->trackColumns().contains("size"));
        QVERIFY(size->property("checked").toBool());
        QTest::mouseClick(window_, Qt::LeftButton, {}, onName);
        QTRY_VERIFY(!bridge_->trackColumns().contains("size"));
        QVERIFY(!size->property("checked").toBool());
        flick->setProperty("contentY", 0);
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
