import QtQuick
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// One album: the art and its facts (artist, year, genres, length, drive; each a link),
// the quality picker and play at the top right, then the chosen copy's tracks.
ColumnLayout {
    id: page
    objectName: "albumView"

    property var owner
    property var route: ({ page: "home", params: {} })  // this page's own: set by CategoryView
    property int depth: 0
    readonly property var params: route.params.album !== undefined ? route.params : ({ albumArtist: "", album: "" })
    property var copies: Library.albumCopies(params.albumArtist, params.album, owner.categoryId)
    // The quality you picked for this album (remembered), else the best copy.
    readonly property string qualityKey: "quality/album/" + encodeURIComponent(params.albumArtist + "\u001f" + params.album)
    property string chosen: Library.preference(qualityKey, "")
    readonly property int copyIndex: Math.max(0, copies.findIndex(c => c.label === chosen))
    readonly property var copy: copies[copyIndex] || ({ tracks: [], label: "", trackCount: 0, totalTracks: 0 })
    readonly property var tracks: copy.tracks
    readonly property var first: tracks.length > 0 ? tracks[0] : null
    readonly property var genres: Format.unique(Format.flatten(tracks, t => t.genres))
    readonly property int year: tracks.reduce((y, t) => Math.max(y, t.year), 0)
    readonly property string date: first && first.date.length > 4 ? first.date : ""
    readonly property int discs: new Set(tracks.map(t => t.disc)).size
    readonly property int length: tracks.reduce((sum, t) => sum + t.duration, 0)
    // The grid's cover, else the first thumbnail among every copy's tracks (art can sit with any copy).
    readonly property string cover: {
        const given = page.params.coverEntry ? Library.thumbnail(page.params.coverEntry) : ""
        if (given !== "")
            return given
        for (const c of copies) {
            for (const t of c.tracks) {
                const url = Library.thumbnail(t.entryId)
                if (url !== "")
                    return url
            }
        }
        return ""
    }

    Connections {
        target: Library
        function onContentChanged() { page.copies = Library.albumCopies(page.params.albumArtist, page.params.album, page.owner.categoryId) }
    }

    spacing: Theme.px(18)

    BrowseHeader {
        Layout.fillWidth: true
        crumbs: page.owner.crumbs(page.depth)
        showSort: false
        title: ""
        onBack: page.owner.back()
        onCrumbClicked: (crumb) => page.owner.goToCrumb(crumb)
    }

    HeroHeader {
        Layout.fillWidth: true
        source: page.cover
        online: page.first ? page.first.online : true
        label: "album"
        title: page.params.album

        Flow {
            Layout.fillWidth: true
            spacing: Theme.px(14)
            LinkText {
                text: page.params.albumArtist
                onActivated: page.owner.open("artist", { name: page.params.albumArtist, crumb: page.params.albumArtist })
            }
            LinkText {
                visible: page.year > 0
                text: page.year
                onActivated: page.owner.open("year", { value: page.year, crumb: String(page.year) })
            }
            Text { visible: page.date !== ""; text: "released " + page.date; color: Theme.textDim }
            Repeater {
                model: page.genres
                LinkText {
                    required property string modelData
                    text: modelData
                    onActivated: page.owner.open("genre", { value: modelData, crumb: modelData })
                }
            }
        }
        Flow {
            Layout.fillWidth: true
            spacing: Theme.px(14)
            Text {
                color: Theme.textDim
                text: Format.plural(page.copy.trackCount, "track") + (page.copy.complete ? "" : " of " + page.copy.totalTracks)
                      + " · " + Format.duration(page.length) + (page.discs > 1 ? " · " + page.discs + " discs" : "")
            }
            Text {
                visible: Library.reading
                text: "reading tags · this album may still grow"
                color: Theme.textFaint
                font.pixelSize: Theme.fontSizeSmall
            }
            Row {
                spacing: Theme.px(6)
                visible: page.first !== null
                Icon { name: "drive"; size: Theme.px(14); anchors.verticalCenter: parent.verticalCenter }
                LinkText {
                    objectName: "driveLink"
                    text: page.first ? Library.folderLabel(page.first.rootId) : ""
                    font.pixelSize: Theme.fontSizeSmall
                    onActivated: page.owner.folderSettingsRequested(page.first.rootId)
                }
            }
        }

        actions: [
            QualityPicker {
                objectName: "qualityPicker"
                options: page.copies.map(c => ({ label: c.label, detail: (c.complete ? Format.plural(c.trackCount, "track") : c.trackCount + " of " + c.totalTracks + " tracks") + " · " + Format.bytes(c.size) }))
                currentIndex: page.copyIndex
                onPicked: (index) => {
                    page.chosen = page.copies[index].label
                    Library.setPreference(page.qualityKey, page.chosen)
                }
            },
            PlayButton {
                objectName: "playAlbum"
                tip: "play album"
                onClicked: {
                    const ids = page.tracks.map(t => t.entryId)
                    page.owner.play(ids, 0, ids, "“" + page.params.album + "”", false)
                }
            }
        ]
    }

    TrackList {
        objectName: "trackList"
        Layout.fillWidth: true
        Layout.fillHeight: true
        tracks: page.tracks
        // The album from this track, in the copy shown; "add to queue" adds just this track.
        onTrackClicked: (track) => {
            const ids = page.tracks.map(t => t.entryId)
            page.owner.play(ids, Math.max(0, ids.indexOf(track.entryId)), [track.entryId], "“" + track.title + "”", false)
        }
    }
}
