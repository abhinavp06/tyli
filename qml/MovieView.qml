import QtQuick
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// One movie: the poster and facts (year, genres, directors, length, drive; links), the
// plot, and the quality picker (best copy by default) and play at the top right.
ColumnLayout {
    id: page
    objectName: "movieView"

    property var owner
    readonly property var params: owner.route.params.title !== undefined ? owner.route.params : ({ title: "", year: 0 })
    property var copies: Library.movieCopies(params.title, params.year, owner.categoryId)
    readonly property string qualityKey: "quality/movie/" + encodeURIComponent(params.title + "\u001f" + params.year)
    property string chosen: Library.preference(qualityKey, "")
    readonly property int copyIndex: Math.max(0, copies.findIndex(c => c.label === chosen))
    readonly property var item: copies[copyIndex] ? copies[copyIndex].item : null

    Connections {
        target: Library
        function onContentChanged() { page.copies = Library.movieCopies(page.params.title, page.params.year, page.owner.categoryId) }
    }

    spacing: 18

    BrowseHeader {
        Layout.fillWidth: true
        crumbs: page.owner.crumbs()
        showSort: false
        onBack: page.owner.back()
        onCrumbClicked: (crumb) => page.owner.goToCrumb(crumb)
    }

    HeroHeader {
        Layout.fillWidth: true
        poster: true
        placeholder: "movies"
        source: Library.thumbnail(page.params.coverEntry || (page.item ? page.item.posterEntry : 0))
        online: page.item ? page.item.online : true
        label: "movie"
        title: page.params.title

        Flow {
            Layout.fillWidth: true
            spacing: 14
            LinkText {
                visible: page.params.year > 0
                text: page.params.year
                onActivated: page.owner.open("year", { value: page.params.year, crumb: String(page.params.year) })
            }
            Text { visible: page.item !== null && page.item.duration > 0; text: page.item ? Format.runtime(page.item.duration) : ""; color: Theme.textDim }
            Repeater {
                model: page.item ? page.item.genres : []
                LinkText {
                    required property string modelData
                    text: modelData
                    onActivated: page.owner.open("genre", { value: modelData, crumb: modelData })
                }
            }
        }
        Flow {
            Layout.fillWidth: true
            spacing: 14
            visible: page.item !== null && page.item.directors.length > 0
            Text { text: "directed by"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall }
            Repeater {
                model: page.item ? page.item.directors : []
                LinkText {
                    required property string modelData
                    text: modelData
                    onActivated: page.owner.open("director", { name: modelData, crumb: modelData })
                }
            }
        }
        Text {
            Layout.fillWidth: true
            Layout.maximumWidth: 760
            visible: text !== ""
            text: page.item ? page.item.plot : ""
            color: Theme.textDim
            wrapMode: Text.Wrap
            maximumLineCount: 5
            elide: Text.ElideRight
        }
        Row {
            spacing: 14
            visible: page.item !== null
            Row {
                spacing: 6
                Icon { name: "drive"; size: 14; anchors.verticalCenter: parent.verticalCenter }
                LinkText {
                    text: page.item ? Library.folderLabel(page.item.rootId) : ""
                    font.pixelSize: Theme.fontSizeSmall
                    onActivated: page.owner.folderSettingsRequested(page.item.rootId)
                }
            }
            Text {
                text: !page.item ? "" : page.item.source === "nfo" ? "info from its .nfo" : page.item.source === "tags" ? "info from its tags" : "info from its file name"
                color: Theme.textFaint
                font.pixelSize: Theme.fontSizeSmall
            }
        }

        actions: [
            QualityPicker {
                objectName: "qualityPicker"
                options: page.copies.map(c => ({ label: c.label, detail: Format.bytes(c.item.size) + (c.item.online ? "" : " · offline") }))
                currentIndex: page.copyIndex
                onPicked: (index) => {
                    page.chosen = page.copies[index].label
                    Library.setPreference(page.qualityKey, page.chosen)
                }
            },
            PlayButton { tip: "play" }
        ]
    }

    Item { Layout.fillHeight: true }
}
