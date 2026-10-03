#include "app_setup.hpp"
#include "library_bridge.hpp"
#include "player_bridge.hpp"
#include "test_support.hpp"
#include "thumbnail_cache.hpp"

#include <QDir>
#include <QProcess>
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
    std::unique_ptr<PlayerBridge> player_;
    bool realMedia_ = false;
    std::unique_ptr<QQmlApplicationEngine> engine_;
    QQuickWindow* window_ = nullptr;

    // `picture`: the page may show a video frame (on OpenGL), so the black check doesn't apply.
    void screenshot(const QString& name, bool picture = false) {
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
        if (picture) return;
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
        QVERIFY(window_->setProperty("playlistOpen", 0));   // as a sidebar click does
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
        writeFile(media_.filePath("Films/Sicario (2015)/Sicario.mkv"), 4096);   // more by Villeneuve, more in Drama
        for (const char* episode : {"Season 1/Dark S01E01.mkv", "Season 1/Dark S01E02.mkv", "Season 1/Dark S01E03.mkv",
                                    "Season 2/Dark S02E01.mkv", "Season 2/Dark S02E02.mkv"}) {
            writeFile(media_.filePath(QStringLiteral("TV/Dark/") + QLatin1String(episode)), 2048);
        }
        writeImage(media_.filePath("TV/Dark/poster.png"), QColor(30, 30, 30));
        // Real media for the player's screens, when the ffmpeg tool is there (it is in CI):
        // three tones in In Rainbows and a clip for Heat.
        const QString ffmpeg = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
        if (!ffmpeg.isEmpty()) {
            const auto run = [&](const QStringList& args) { return QProcess::execute(ffmpeg, QStringList{"-y", "-loglevel", "error"} + args) == 0; };
            realMedia_ = true;
            for (int i = 1; i <= 3; ++i) {
                realMedia_ &= run({"-f", "lavfi", "-t", "4", "-i", QStringLiteral("sine=frequency=%1").arg(300 + 100 * i), "-c:a", "flac",
                                   media_.filePath(QStringLiteral("Music/Radiohead/In Rainbows/%1.flac").arg(i, 2, 10, QLatin1Char('0')))});
            }
            realMedia_ &= run({"-f", "lavfi", "-t", "4", "-i", "testsrc=size=960x540:rate=10", "-f", "lavfi", "-t", "4", "-i", "anullsrc",
                               "-c:v", "mpeg4", "-q:v", "5", "-pix_fmt", "yuv420p", "-c:a", "aac",
                               "-f", "matroska", media_.filePath("Films/Heat (1995)/Heat.mkv")});
            // Two real episodes for the video page's previous/next episode (copies of Heat's clip).
            for (const char* e : {"TV/Dark/Season 1/Dark S01E01.mkv", "TV/Dark/Season 1/Dark S01E02.mkv"}) {
                QFile::remove(media_.filePath(QLatin1String(e)));
                realMedia_ &= QFile::copy(media_.filePath("Films/Heat (1995)/Heat.mkv"), media_.filePath(QLatin1String(e)));
            }
            // The tool is there but couldn't make them (e.g. an encoder missing from an LGPL
            // build): fail here, not skip the player's screens quietly.
            QVERIFY2(realMedia_, "the ffmpeg tool couldn't make the test media");
        }

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
            } else if (path.startsWith("Sicario")) {
                v.title = "Sicario";
                v.year = 2015;
                v.genres = {"Drama", "Thriller"};
                v.directors = {"Denis Villeneuve"};
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

        // The player, headless (no sound card, no window of its own).
        if (qEnvironmentVariable("TYLI_PLAYER_OUTPUTS") != QLatin1String("noaudio")) qputenv("TYLI_PLAYER_OUTPUTS", "null");
        player_ = std::make_unique<PlayerBridge>(bridge_->databasePath());
        PlayerBridge::setInstance(player_.get());
        QVERIFY(player_->ready());

        engine_ = std::make_unique<QQmlApplicationEngine>();
        engine_->loadFromModule("Tyli", "Main");
        QCOMPARE(engine_->rootObjects().size(), 1);
        window_ = qobject_cast<QQuickWindow*>(engine_->rootObjects().first());
        QVERIFY(window_);
        QVERIFY(QTest::qWaitForWindowExposed(window_));
    }

    void cleanupTestCase() {
        engine_.reset();
        player_.reset();
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
        // The grid fills its width: as many columns as it computed, the last card ending at the
        // right edge, under the header's sort (the user, 2026-10-03: a column was lost to the gap).
        {
            auto* view = qobject_cast<QQuickItem*>(grid);
            const int columns = grid->property("columns").toInt();
            QVERIFY(columns >= 2);
            QVERIFY(QMetaObject::invokeMethod(grid, "forceLayout"));
            QQuickItem* last = nullptr;
            QVERIFY(QMetaObject::invokeMethod(grid, "itemAtIndex", Q_RETURN_ARG(QQuickItem*, last), Q_ARG(int, columns - 1)));
            QVERIFY(last && !last->childItems().isEmpty());
            QQuickItem* card = last->childItems().first();
            QCOMPARE(last->y(), 0.0);  // still on the first row
            const double right = card->mapToItem(view, QPointF(card->width(), 0)).x();
            QVERIFY2(qAbs(right - view->width()) < 1.0, qPrintable(QStringLiteral("last card ends at %1 of %2").arg(right).arg(view->width())));
        }
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
        QVERIFY(QMetaObject::invokeMethod(find("sidebar"), "categoryClicked", Q_ARG(int, categoryId(*bridge_, "Music"))));
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

    // The player's screens with real files (features/player_ui.md §1): the bar, the like,
    // the queue prompt, the queue page, and the video page (black here: no OpenGL).
    void playingMusicAndVideo() {
        if (!realMedia_) QSKIP("the ffmpeg tool isn't installed: no real media to play");
        showCategory("Music");
        call("categoryView", "open", "album", QVariantMap{{"albumArtist", "Radiohead"}, {"album", "In Rainbows"}, {"crumb", "In Rainbows"}});
        QTRY_VERIFY(find("albumView"));
        QVERIFY(QMetaObject::invokeMethod(find("playAlbum"), "clicked"));   // nothing plays yet: no prompt
        QTRY_COMPARE(player_->state(), QStringLiteral("playing"));
        QTRY_COMPARE(player_->title(), QStringLiteral("Track 1"));
        QVERIFY(qobject_cast<QQuickItem*>(find("playerBar"))->isVisible());
        QCOMPARE(player_->queueLength(), 6);
        // Paused, so the 4 s track can't end (and the bar move on) while the test looks at it.
        player_->pause();
        QTRY_COMPARE(player_->state(), QStringLiteral("paused"));
        player_->setLiked(player_->entryId(), true);
        QVERIFY(player_->isLiked(player_->entryId()));
        QTRY_VERIFY(find("playerBar")->property("liked").toBool());   // the bar follows a like made anywhere
        screenshot("player-bar");

        // The expanded player (mockup player-expanded.html): the bar's cover expands it, the
        // queue button opens the panel on the right, the artist link opens the artist and collapses.
        auto* cover = qobject_cast<QQuickItem*>(find("barCover"));
        QVERIFY(cover);
        QTest::mouseClick(window_, Qt::LeftButton, {}, cover->mapToScene(QPointF(cover->width() / 2, cover->height() / 2)).toPoint());
        QTRY_VERIFY(window_->property("expandedOpen").toBool());
        QVERIFY(qobject_cast<QQuickItem*>(find("expandedPlayer"))->isVisible());
        QCOMPARE(player_->artist(), QStringLiteral("Radiohead"));
        QCOMPARE(player_->album(), QStringLiteral("In Rainbows"));
        screenshot("player-expanded");
        QVERIFY(QMetaObject::invokeMethod(find("queueButton"), "clicked"));
        QTRY_VERIFY(qobject_cast<QQuickItem*>(find("queuePanel"))->isVisible());
        QVERIFY(!window_->property("queueOpen").toBool());   // the panel, not the queue page
        screenshot("player-expanded-queue");
        QVERIFY(QMetaObject::invokeMethod(find("closeQueuePanel"), "clicked"));
        QTRY_VERIFY(!qobject_cast<QQuickItem*>(find("queuePanel"))->isVisible());
        QVERIFY(QMetaObject::invokeMethod(find("expandedArtist"), "activated"));
        QTRY_VERIFY(!window_->property("expandedOpen").toBool());
        QTRY_VERIFY(find("artistView") && qobject_cast<QQuickItem*>(find("artistView"))->isVisible());
        // Expanded again: Esc collapses.
        QTest::mouseClick(window_, Qt::LeftButton, {}, cover->mapToScene(QPointF(cover->width() / 2, cover->height() / 2)).toPoint());
        QTRY_VERIFY(window_->property("expandedOpen").toBool());
        QTest::keyClick(window_, Qt::Key_Escape);
        QTRY_VERIFY(!window_->property("expandedOpen").toBool());
        call("categoryView", "open", "album", QVariantMap{{"albumArtist", "Radiohead"}, {"album", "In Rainbows"}, {"crumb", "In Rainbows"}});
        QTRY_VERIFY(find("trackList"));

        // A track click while something plays: the prompt; "add to queue" adds just that track.
        const QVariantList tracks = find("trackList")->property("tracks").toList();
        QVERIFY(QMetaObject::invokeMethod(find("trackList"), "trackClicked", Q_ARG(QVariant, tracks.value(2))));
        QObject* prompt = find("queuePrompt");
        QTRY_VERIFY(prompt->property("opened").toBool());
        screenshot("queue-prompt");
        QVERIFY(QMetaObject::invokeMethod(prompt, "addToQueue"));
        QTRY_COMPARE(player_->queueLength(), 7);
        QTRY_VERIFY(!prompt->property("opened").toBool());

        QVERIFY(window_->setProperty("queueOpen", true));
        QTRY_VERIFY(qobject_cast<QQuickItem*>(find("queuePage"))->isVisible());
        QTRY_COMPARE(find("queueList")->property("count").toInt(), 7);
        screenshot("queue");
        QVERIFY(window_->setProperty("queueOpen", false));

        // A movie: the video page, playing at once (no prompt); leaving it pauses.
        showCategory("Movies");
        call("categoryView", "open", "movie", QVariantMap{{"title", "Heat"}, {"year", 1995}, {"crumb", "Heat"}});
        QTRY_VERIFY(find("movieView"));
        QVERIFY(QMetaObject::invokeMethod(find("playMovie"), "clicked"));
        QTRY_VERIFY(window_->property("videoOpen").toBool());
        QTRY_VERIFY(player_->hasVideo());
        QTRY_COMPARE(player_->state(), QStringLiteral("playing"));
        QCOMPARE(player_->queueLength(), 1);   // the video replaced the queue
        screenshot("video-page", true);
        // Full screen and its icon follow the window both ways (the user, 2026-10-03).
        QObject* fsButton = find("fullScreenButton");
        QVERIFY(fsButton);
        QCOMPARE(fsButton->property("iconName").toString(), QStringLiteral("fullscreen"));
        QVERIFY(QMetaObject::invokeMethod(fsButton, "clicked"));
        QTRY_COMPARE(window_->visibility(), QWindow::FullScreen);
        QCOMPARE(fsButton->property("iconName").toString(), QStringLiteral("fullscreenExit"));
        window_->setVisibility(QWindow::Windowed);   // the window changes by itself
        QTRY_VERIFY(!window_->property("fullScreen").toBool());
        QCOMPARE(fsButton->property("iconName").toString(), QStringLiteral("fullscreen"));
        window_->setVisibility(QWindow::FullScreen);
        QTRY_VERIFY(window_->property("fullScreen").toBool());
        QCOMPARE(fsButton->property("iconName").toString(), QStringLiteral("fullscreenExit"));
        QVERIFY(QMetaObject::invokeMethod(fsButton, "clicked"));
        QTRY_COMPARE(window_->visibility(), QWindow::Windowed);
        QVERIFY(!qobject_cast<QQuickItem*>(find("nextEpisode"))->isVisible());   // a movie: no episodes
        QTest::mouseClick(window_, Qt::BackButton, {}, QPoint(window_->width() / 2, window_->height() / 2));
        QTRY_VERIFY(!window_->property("videoOpen").toBool());
        QTRY_COMPARE(player_->state(), QStringLiteral("paused"));

        // An episode: the season plays as a queue, with previous and next episode.
        showCategory("Shows");
        call("categoryView", "open", "season", QVariantMap{{"show", "Dark"}, {"season", 1}, {"crumb", "Season 1"}});
        QTRY_VERIFY(find("seasonView") && qobject_cast<QQuickItem*>(find("seasonView"))->isVisible());
        QVERIFY(QMetaObject::invokeMethod(find("playSeason"), "clicked"));
        QTRY_VERIFY(window_->property("videoOpen").toBool());
        QTRY_COMPARE(player_->queueLength(), 3);   // the whole season
        QTRY_VERIFY(player_->hasVideo());
        auto* prevEp = qobject_cast<QQuickItem*>(find("previousEpisode"));
        auto* nextEp = qobject_cast<QQuickItem*>(find("nextEpisode"));
        QTRY_VERIFY(nextEp->isVisible());
        QVERIFY(!prevEp->isEnabled());   // the first episode
        QVERIFY(nextEp->isEnabled());
        // The season as a queue, beside the picture: its rows, and a click jumps.
        QVERIFY(QMetaObject::invokeMethod(find("videoQueueButton"), "clicked"));
        QTRY_VERIFY(qobject_cast<QQuickItem*>(find("videoQueuePanel"))->isVisible());
        QTRY_COMPARE(find("videoQueueList")->property("count").toInt(), 3);
        screenshot("video-episode", true);
        QVERIFY(QMetaObject::invokeMethod(find("closeVideoQueue"), "clicked"));
        QTRY_VERIFY(!qobject_cast<QQuickItem*>(find("videoQueuePanel"))->isVisible());
        QVERIFY(QMetaObject::invokeMethod(nextEp, "clicked"));
        QTRY_COMPARE(player_->current(), 1);
        QTRY_VERIFY(prevEp->isEnabled());
        QVERIFY(QMetaObject::invokeMethod(prevEp, "clicked"));
        QTRY_COMPARE(player_->current(), 0);
        QTest::mouseClick(window_, Qt::BackButton, {}, QPoint(window_->width() / 2, window_->height() / 2));
        QTRY_VERIFY(!window_->property("videoOpen").toBool());
        player_->clear();
        QTRY_VERIFY(!player_->active());
        QVERIFY(qmlWarnings.isEmpty());
    }

    // Playlists end to end (features/playlists_ui.md): the sidebar, the add menu on a track, a
    // playlist's page (rename, pin, reorder, remove, play), Liked Songs, and delete.
    void playlistsEndToEnd() {
        if (!realMedia_) QSKIP("the ffmpeg tool isn't installed: no real media to play");
        const auto clickAt = [&](QQuickItem* item) {
            QVERIFY(item);
            QTest::mouseClick(window_, Qt::LeftButton, {}, item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint());
        };
        const auto item = [&](const QString& name) { return qobject_cast<QQuickItem*>(find(name.toUtf8().constData())); };

        // + in the sidebar: a name, ✓, and the new playlist opens.
        clickAt(item("newPlaylistButton"));
        auto* name = item("newPlaylistName");
        QTRY_VERIFY(name->isVisible());
        name->setProperty("text", "Road Trip");
        QTest::qWait(100);   // the field just appeared: let the sidebar lay it out before clicking its ✓
        clickAt(item("newPlaylistNameOk"));
        QTRY_COMPARE(window_->property("view").toString(), QStringLiteral("playlist"));
        QTRY_COMPARE(find("playlistTitle")->property("text").toString(), QStringLiteral("Road Trip"));
        QVERIFY(item("sidebarPlaylist-Road Trip"));
        QCOMPARE(find("playlistRows")->property("count").toInt(), 0);

        // An album's track: its playlist icon, then the playlist in the menu.
        const QVariantList copies = bridge_->albumCopies("Radiohead", "In Rainbows");
        QVERIFY(!copies.isEmpty());
        QVariantList tracks;
        for (const QVariant& t : copies[0].toMap().value("tracks").toList()) tracks << t.toMap().value("entryId");
        QVERIFY(tracks.size() >= 3);
        showCategory("Music");
        call("categoryView", "open", "album", QVariantMap{{"albumArtist", "Radiohead"}, {"album", "In Rainbows"}, {"crumb", "In Rainbows"}});
        // (Pages you came from stay alive, hidden: wait for this page's own row.)
        const QString addName = QStringLiteral("addToPlaylist-%1").arg(tracks[0].toLongLong());
        QTRY_VERIFY(item(addName) && item(addName)->isVisible());
        clickAt(item(addName));
        // The menu is a Popup (not an item): its rows are inside its content item.
        QTRY_VERIFY(find("playlistMenu")->property("opened").toBool());
        auto* menuContent = find("playlistMenu")->property("contentItem").value<QQuickItem*>();
        QVERIFY(menuContent);
        auto* roadRow = qobject_cast<QQuickItem*>(findIn(menuContent, "menuPlaylist-Road Trip"));
        QTRY_VERIFY(roadRow && roadRow->isVisible());
        screenshot("playlist-add-menu");
        clickAt(roadRow);
        QTRY_COMPARE(player_->playlistInfo(player_->playlists()[0].toMap().value("id").toLongLong()).value("tracks").toInt(), 1);
        const qint64 road = player_->playlists()[0].toMap().value("id").toLongLong();
        player_->addToPlaylist(road, {tracks[1], tracks[2]});

        // The page: three rows, from the sidebar.
        clickAt(item("sidebarPlaylist-Road Trip"));
        QTRY_COMPARE(window_->property("view").toString(), QStringLiteral("playlist"));
        QTRY_COMPARE(find("playlistRows")->property("count").toInt(), 3);
        screenshot("playlist");

        // Rename in the field, with its ✓.
        clickAt(item("renamePlaylist"));
        QTRY_VERIFY(item("renameField")->isVisible());
        item("renameField")->setProperty("text", "Road");
        screenshot("playlist-rename");
        clickAt(item("renameFieldOk"));
        QTRY_COMPARE(find("playlistTitle")->property("text").toString(), QStringLiteral("Road"));

        // Pin it (the pin on the right of its sidebar row).
        QTest::mouseMove(window_, item("sidebarPlaylist-Road")->mapToScene(QPointF(20, 10)).toPoint());
        clickAt(item("pin-Road"));
        QTRY_VERIFY(player_->playlists()[0].toMap().value("pinned").toBool());

        // Drag the third row to the top by its handle.
        const auto first = [&] { return player_->playlistEntries(road).first().toLongLong(); };
        auto* grip = item("grip-2");
        auto* top = item("playlistRow-0");
        QVERIFY(grip && top);
        const QPoint from = grip->mapToScene(QPointF(grip->width() / 2, grip->height() / 2)).toPoint();
        const QPoint to = top->mapToScene(QPointF(top->width() / 2, 2)).toPoint();
        QTest::mousePress(window_, Qt::LeftButton, {}, from);
        for (int i = 1; i <= 6; ++i) QTest::mouseMove(window_, from + (to - from) * i / 6, 10);
        QTest::mouseRelease(window_, Qt::LeftButton, {}, to);
        QTRY_COMPARE(first(), tracks[2].toLongLong());

        // × removes a row.
        QTest::mouseMove(window_, item("playlistRow-1")->mapToScene(QPointF(40, 10)).toPoint());
        clickAt(item("rowRemove-1"));
        QTRY_COMPARE(find("playlistRows")->property("count").toInt(), 2);

        // Play it: the sidebar's speaker marks where the music came from.
        QVERIFY(QMetaObject::invokeMethod(find("playPlaylist"), "clicked"));
        if (find("queuePrompt")->property("opened").toBool()) QVERIFY(QMetaObject::invokeMethod(find("queuePrompt"), "clearAndPlay"));
        QTRY_COMPARE(player_->source().value("kind").toString(), QStringLiteral("playlist"));
        QTRY_VERIFY(item("playingFrom-Road") && item("playingFrom-Road")->isVisible());
        screenshot("playlist-playing");
        player_->clear();

        // Liked Songs: the hearts, on their own page.
        player_->setLiked(tracks[0].toLongLong(), true);
        clickAt(item("sidebarLiked"));
        QTRY_COMPARE(find("playlistTitle")->property("text").toString(), QStringLiteral("Liked Songs"));
        QTRY_COMPARE(find("playlistRows")->property("count").toInt(), 1);
        screenshot("liked-songs");
        player_->setLiked(tracks[0].toLongLong(), false);

        // Delete, with its inline confirm: back to where you were.
        clickAt(item("sidebarPlaylist-Road"));
        QTRY_COMPARE(find("playlistTitle")->property("text").toString(), QStringLiteral("Road"));
        clickAt(item("deletePlaylist"));
        QTRY_VERIFY(item("confirmDeletePlaylist")->isVisible());
        clickAt(item("confirmDeletePlaylist"));
        QTRY_VERIFY(player_->playlists().isEmpty());
        QTRY_VERIFY(window_->property("view").toString() != QStringLiteral("playlist"));
        QVERIFY(qmlWarnings.isEmpty());
    }

    void moviesAMovieAndADirector() {
        showCategory("Movies");
        QObject* grid = find("posterGrid");
        QVERIFY(grid);
        QTRY_COMPARE(grid->property("count").toInt(), 3);  // Arrival's two copies are one card
        screenshot("movies");
        // By director and by genre: sections, and the ones with none last (Heat has no genre).
        setState("moviesView", "sortKey", "director");
        QObject* sections = find("videoSections");
        QVERIFY(sections && qobject_cast<QQuickItem*>(sections)->isVisible());
        QVERIFY(!qobject_cast<QQuickItem*>(grid)->isVisible());
        QTRY_COMPARE(sections->property("count").toInt(), 2);   // Denis Villeneuve, Michael Mann
        QTRY_VERIFY(!qobject_cast<QQuickItem*>(find("noneSection"))->isVisible());   // everyone has a director
        screenshot("movies-by-director");
        setState("moviesView", "sortKey", "genre");
        QTRY_COMPARE(sections->property("count").toInt(), 3);   // Drama, Science Fiction, Thriller
        QTRY_VERIFY(qobject_cast<QQuickItem*>(find("noneSection"))->isVisible());    // Heat
        screenshot("movies-by-genre");
        setState("moviesView", "sortKey", "added");
        QTRY_VERIFY(qobject_cast<QQuickItem*>(grid)->isVisible());
        call("categoryView", "open", "movie", QVariantMap{{"title", "Arrival"}, {"year", 2016}, {"crumb", "Arrival"}});
        QTRY_VERIFY(find("movieView"));
        const QVariantList options = find("qualityPicker")->property("options").toList();
        QCOMPARE(options.size(), 2);
        QVERIFY(options[0].toMap().value("label").toString().startsWith("2160p"));
        // Its copies as rows, the one that plays first (the best).
        QTRY_VERIFY(find("copy-0") && find("copy-1"));
        QVERIFY(find("copy-0")->property("current").toBool());
        // More by Villeneuve (Sicario); Drama's only other one is Sicario too, so no row for it.
        QTRY_VERIFY(find("more-director-Denis Villeneuve"));
        QVERIFY(!find("more-genre-Drama"));
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
        setState("showsView", "sortKey", "genre");
        QTRY_COMPARE(find("videoSections")->property("count").toInt(), 1);   // Mystery
        screenshot("shows-by-genre");
        setState("showsView", "sortKey", "added");
        call("categoryView", "open", "show", QVariantMap{{"show", "Dark"}, {"crumb", "Dark"}});
        QTRY_VERIFY(find("showView"));
        // Its seasons as cards, like an artist's albums.
        QTRY_VERIFY(find("season-1") && find("season-2"));
        QCOMPARE(find("season-1")->property("title").toString(), QStringLiteral("Season 1"));
        QVERIFY(find("season-1")->property("subtitle").toString().startsWith(QStringLiteral("3 episodes")));
        screenshot("show");
        // A season, laid out like an album: its episodes as tracks.
        QVERIFY(QMetaObject::invokeMethod(find("season-2"), "activated"));
        QTRY_VERIFY(find("seasonView") && qobject_cast<QQuickItem*>(find("seasonView"))->isVisible());
        QObject* episodes = find("episodeList");
        QVERIFY(episodes);
        QTRY_COMPARE(episodes->property("count").toInt(), 2);  // season 2
        screenshot("season");
        // Back (the show's link) returns to the seasons.
        QVERIFY(QMetaObject::invokeMethod(find("seasonShowLink"), "activated"));
        QTRY_VERIFY(qobject_cast<QQuickItem*>(find("showView"))->isVisible());
        QVERIFY(QMetaObject::invokeMethod(find("season-1"), "activated"));
        QTRY_VERIFY(find("seasonView") && qobject_cast<QQuickItem*>(find("seasonView"))->isVisible());
        QTRY_COMPARE(find("episodeList")->property("count").toInt(), 3);
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
        QTRY_COMPARE(find("searchView")->property("answered").toString(), QStringLiteral("zzzz"));
        QCOMPARE(find("searchView")->property("found").toInt(), 0);
        QVERIFY(window_->setProperty("searchText", "r"));   // one letter: no search, a hint
        QTRY_VERIFY(find("searchView")->property("tooShort").toBool());
        screenshot("search-too-short");

        // A result opened from the search: back (the mouse's button) shows the results again.
        QVERIFY(window_->setProperty("searchText", "radio"));
        QTRY_COMPARE(find("searchView")->property("answered").toString(), QStringLiteral("radio"));
        const QVariantList albums = find("searchView")->property("albums").toMap().value("items").toList();
        QVERIFY(!albums.isEmpty());
        const QStringList values = albums.first().toMap().value("values").toStringList();
        const QVariant params = QVariantMap{{"albumArtist", values[1]}, {"album", values[2]}, {"crumb", values[2]}};
        QVERIFY(QMetaObject::invokeMethod(find("searchView"), "opened", Q_ARG(int, values[0].toInt()), Q_ARG(QString, "album"),
                                          Q_ARG(QVariant, params)));
        QTRY_VERIFY(find("albumView") && qobject_cast<QQuickItem*>(find("albumView"))->isVisible());
        QCOMPARE(window_->property("searchText").toString(), QString());
        QTest::mouseClick(window_, Qt::BackButton, {}, QPoint(window_->width() / 2, window_->height() / 2));
        QTRY_COMPARE(window_->property("searchText").toString(), QStringLiteral("radio"));
        QVERIFY(qobject_cast<QQuickItem*>(find("searchView"))->isVisible());
        QCOMPARE(find("searchView")->property("answered").toString(), QStringLiteral("radio"));   // as they were
        screenshot("search-after-back");
        QVERIFY(window_->setProperty("searchText", ""));
        QVERIFY(qmlWarnings.isEmpty());
    }

    // Search from any page, Settings included; the field's × clears it and the page comes back.
    void searchFromSettingsAndClear() {
        QVERIFY(window_->setProperty("settingsOpen", true));
        setState("settingsView", "currentPage", "general");
        auto* field = qobject_cast<QQuickItem*>(find("searchField"));
        auto* settings = qobject_cast<QQuickItem*>(find("settingsView"));
        QVERIFY(field && settings);
        field->setProperty("text", "");  // earlier steps may have left a search
        QVERIFY(settings->isVisible());
        QVERIFY(!qobject_cast<QQuickItem*>(find("clearSearch"))->isVisible());  // nothing to clear
        field->setProperty("text", "radio");
        QTRY_VERIFY(qobject_cast<QQuickItem*>(find("searchView"))->isVisible());
        QVERIFY(!settings->isVisible());
        QCOMPARE(window_->property("view").toString(), QStringLiteral("search"));
        screenshot("search-over-settings");

        auto* clear = qobject_cast<QQuickItem*>(find("clearSearch"));
        QVERIFY(clear && clear->isVisible());
        // Hovered, its background stays inside the field's outline (the user, 2026-10-03).
        field->forceActiveFocus();
        QTest::mouseMove(window_, clear->mapToScene(QPointF(clear->width() / 2, clear->height() / 2)).toPoint());
        QTRY_VERIFY(clear->property("hovered").toBool());
        const QRectF button = clear->mapRectToScene(QRectF(0, 0, clear->width(), clear->height()));
        const QRectF outline = field->mapRectToScene(QRectF(0, 0, field->width(), field->height()));
        QVERIFY(button.right() <= outline.right() - 2 && button.top() >= outline.top() + 2 && button.bottom() <= outline.bottom() - 2);
        const QImage shot = window_->grabWindow();
        for (int y = int(outline.top()) + 1; y < int(outline.bottom()) - 1; ++y) {  // the right edge: one color, top to bottom
            QCOMPARE(shot.pixelColor(int(outline.right()) - 1, y), shot.pixelColor(int(outline.right()) - 1, int(outline.top()) + 1));
        }
        screenshot("search-clear-hovered");
        QTest::mouseClick(window_, Qt::LeftButton, {}, clear->mapToScene(QPointF(clear->width() / 2, clear->height() / 2)).toPoint());
        QCOMPARE(field->property("text").toString(), QString());
        QCOMPARE(window_->property("searchText").toString(), QString());   // at once, no typing delay
        QVERIFY(settings->isVisible());
        QCOMPARE(settings->property("currentPage").toString(), QStringLiteral("general"));  // where it was
        QVERIFY(field->hasActiveFocus());

        // Back from results over Settings shows Settings; back again leaves them.
        field->setProperty("text", "radio");
        QTRY_COMPARE(window_->property("view").toString(), QStringLiteral("search"));
        QTest::mouseClick(window_, Qt::BackButton, {}, QPoint(window_->width() / 2, window_->height() / 2));
        QTRY_COMPARE(window_->property("view").toString(), QStringLiteral("settings"));
        QCOMPARE(field->property("text").toString(), QString());
        QTest::mouseClick(window_, Qt::BackButton, {}, QPoint(window_->width() / 2, window_->height() / 2));
        QTRY_VERIFY(window_->property("view").toString() != QStringLiteral("settings"));
        QVERIFY(qmlWarnings.isEmpty());
    }

    // With search results showing (over any page), the sidebar's sections and tyli_ go to their
    // page and end the search (the user, 2026-10-03: they didn't).
    void theSidebarAndTheLogoLeaveSearch() {
        auto* field = qobject_cast<QQuickItem*>(find("searchField"));
        const auto clickOn = [&](const char* name) {
            auto* item = qobject_cast<QQuickItem*>(find(name));
            QVERIFY2(item && item->isVisible(), name);
            QTest::mouseClick(window_, Qt::LeftButton, {}, item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint());
        };
        const auto search = [&] {
            field->forceActiveFocus();
            field->setProperty("text", "radio");
            QTRY_COMPARE(window_->property("view").toString(), QStringLiteral("search"));
        };

        search();
        clickOn("sidebarItem-movies");
        QTRY_COMPARE(window_->property("view").toString(), QStringLiteral("category"));
        QCOMPARE(window_->property("currentCategoryId").toInt(), categoryId(*bridge_, "Movies"));
        QCOMPARE(field->property("text").toString(), QString());

        search();
        clickOn("wordmark");
        QTRY_COMPARE(window_->property("view").toString(), QStringLiteral("home"));
        QCOMPARE(field->property("text").toString(), QString());

        // Results over Settings: a section leaves both.
        QVERIFY(window_->setProperty("settingsOpen", true));
        search();
        clickOn("sidebarItem-music");
        QTRY_COMPARE(window_->property("view").toString(), QStringLiteral("category"));
        QCOMPARE(window_->property("currentCategoryId").toInt(), categoryId(*bridge_, "Music"));
        QVERIFY(!window_->property("settingsOpen").toBool());

        // The bar's queue button opens the queue over results and over Settings, and closes it.
        search();
        QVERIFY(QMetaObject::invokeMethod(find("playerBar"), "queueToggled"));
        QTRY_COMPARE(window_->property("view").toString(), QStringLiteral("queue"));
        QCOMPARE(field->property("text").toString(), QString());
        QVERIFY(window_->setProperty("settingsOpen", true));
        QCOMPARE(window_->property("view").toString(), QStringLiteral("settings"));
        QVERIFY(QMetaObject::invokeMethod(find("playerBar"), "queueToggled"));
        QCOMPARE(window_->property("view").toString(), QStringLiteral("queue"));
        QVERIFY(QMetaObject::invokeMethod(find("playerBar"), "queueToggled"));
        QVERIFY(window_->property("view").toString() != QStringLiteral("queue"));
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
        QVERIFY(find("aboutLine")->property("text").toString().endsWith(QStringLiteral(" · by abhinavp06")));
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
