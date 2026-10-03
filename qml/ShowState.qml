import QtQuick
import Tyli
import "Format.js" as Format

// One show's episodes and their copies (Library.showCopies), the chosen quality (remembered
// per show, shared by the show page and its season pages), and what you watched. `season`
// narrows the episodes and the qualities to one season (allSeasons: the whole show).
QtObject {
    id: state

    readonly property int allSeasons: -1000
    property string show
    property int categoryId: -1
    property int season: allSeasons
    property var info: Library.showCopies(show, categoryId)
    readonly property var allEpisodes: info.episodes || []
    readonly property int coverEntry: info.coverEntry || 0
    readonly property var episodesHere: season === allSeasons ? allEpisodes : allEpisodes.filter(e => e.season === season)

    // Seasons in order, specials (0) and unknown (-1) last.
    readonly property var seasons: Format.unique(allEpisodes.map(e => e.season))
        .sort((a, b) => (a > 0 ? a : 100000 - a) - (b > 0 ? b : 100000 - b))
    function seasonTitle(s) { return s > 0 ? "Season " + s : s === 0 ? "Specials" : "Other episodes" }

    // Qualities, best first: the show's, or those of this season's episodes.
    readonly property var qualities: {
        if (season === allSeasons)
            return info.qualities || []
        const out = []
        for (const q of (info.qualities || [])) {
            const n = episodesHere.filter(e => e.copies.some(c => c.label === q.label)).length
            if (n > 0)
                out.push({ label: q.label, episodes: n, total: episodesHere.length })
        }
        return out
    }
    readonly property string qualityKey: "quality/show/" + encodeURIComponent(show)
    property string chosen: Library.preference(qualityKey, "")
    readonly property int qualityIndex: Math.max(0, qualities.findIndex(q => q.label === chosen))
    readonly property string quality: qualities[qualityIndex] ? qualities[qualityIndex].label : ""
    function choose(index) {
        chosen = qualities[index].label
        Library.setPreference(qualityKey, chosen)
    }

    // Each episode in the chosen quality, or its best copy when it has none in that quality.
    function pick(e) {
        const exact = e.copies.find(c => c.label === state.quality)
        return (exact || e.copies[0]).item
    }
    function picked(list) { return list.map(e => pick(e)) }

    // What you watched: entryId -> {position, duration, viewed}; re-read when playback saves.
    readonly property var watched: {
        const all = Player.revision >= 0 ? picked(allEpisodes) : []   // reads revision: see PlayerBar
        const out = {}
        for (const s of Player.itemStates(all.map(e => e.entryId))) out[s.entryId] = s
        return out
    }
    function partway(e) { const s = watched[e.entryId]; return s && !s.viewed && s.position > 0 && s.duration > 0 }
    function viewed(e) { const s = watched[e.entryId]; return s !== undefined && s.viewed }
    // How much of a season you've seen: viewed episodes, plus how far into the one you're in.
    function seasonProgress(s) {
        const eps = picked(allEpisodes.filter(e => e.season === s))
        if (eps.length === 0)
            return 0
        let done = 0
        for (const e of eps) {
            const w = watched[e.entryId]
            if (w && w.viewed) done += 1
            else if (w && w.duration > 0) done += Math.min(1, w.position / w.duration)
        }
        return done / eps.length
    }

    // An episode plays with the rest of its season after it (features/player_ui.md, decision P2).
    function playFrom(owner, episode) {
        const ids = picked(allEpisodes.filter(e => e.season === episode.season)).map(e => e.entryId)
        owner.play(ids, Math.max(0, ids.indexOf(episode.entryId)), [], "", true)
    }
    // The play button: the episode you're partway through, else the next unwatched one (of this
    // season, or of the show).
    function playNext(owner) {
        const all = picked(episodesHere)
        if (all.length === 0)
            return
        playFrom(owner, all.find(e => partway(e)) || all.find(e => !viewed(e)) || all[0])
    }

    property Connections refresh: Connections {
        target: Library
        function onContentChanged() { state.info = Library.showCopies(state.show, state.categoryId) }
    }
}
