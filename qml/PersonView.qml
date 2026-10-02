import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tyli
import "Format.js" as Format

// A director's catalog: their movies, then the shows they directed episodes of.
ColumnLayout {
    id: page
    objectName: "personView"

    property var owner
    readonly property string name: owner.route.params.name || ""

    spacing: 0

    GroupModel {
        id: movies
        engine: "video"
        groupBy: ["title", "year"]
        filter: ({ director: page.name, type: 1 })
        order: "year"
    }
    GroupModel {
        id: shows
        engine: "video"
        groupBy: ["show"]
        filter: ({ director: page.name, type: 2 })
    }

    BrowseHeader {
        Layout.fillWidth: true
        crumbs: page.owner.crumbs()
        label: "director"
        title: page.name
        subtitle: [movies.total > 0 ? Format.plural(movies.total, "movie") : "", shows.total > 0 ? Format.plural(shows.total, "show") : ""].filter(s => s).join(" · ")
        showSort: false
        onBack: page.owner.back()
        onCrumbClicked: (crumb) => page.owner.goToCrumb(crumb)
    }

    Flickable {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        contentHeight: content.implicitHeight + 24
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: TyScrollBar {}
        Column {
            id: content
            width: parent.width
            topPadding: 18
            spacing: 14
            Text { text: "movies"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall; visible: movies.count > 0 }
            Flow {
                width: parent.width
                spacing: 18
                Repeater {
                    model: movies
                    CoverCard {
                        required property var model
                        width: 150
                        poster: true
                        placeholder: "movies"
                        source: model.thumbnail
                        online: model.online
                        title: model.values[0]
                        subtitle: [model.values[1] > 0 ? model.values[1] : "", Format.resolution(model.maxHeight)].filter(s => s).join(" · ")
                        onActivated: page.owner.open("movie", { title: model.values[0], year: Number(model.values[1]), coverEntry: model.coverEntry, crumb: model.values[0] })
                    }
                }
            }
            Text { text: "episodes"; color: Theme.textDim; font.pixelSize: Theme.fontSizeSmall; visible: shows.count > 0; topPadding: 10 }
            Flow {
                width: parent.width
                spacing: 18
                Repeater {
                    model: shows
                    CoverCard {
                        required property var model
                        width: 150
                        poster: true
                        placeholder: "shows"
                        source: model.thumbnail
                        online: model.online
                        title: model.values[0]
                        subtitle: Format.plural(model.count, "episode")
                        onActivated: page.owner.open("show", { show: model.values[0], coverEntry: model.coverEntry, crumb: model.values[0] })
                    }
                }
            }
        }
    }
}
