.pragma library

// Display helpers shared by the screens. Formatting only: no library logic.

function count(n) {
    return Number(n).toLocaleString(Qt.locale(), "f", 0)
}

function plural(n, word) {
    return count(n) + " " + word + (Number(n) === 1 ? "" : "s")
}

function bytes(n) {
    const units = ["B", "KB", "MB", "GB", "TB"]
    let value = Number(n)
    let unit = 0
    while (value >= 1024 && unit < units.length - 1) {
        value /= 1024
        ++unit
    }
    return (unit === 0 ? value : value.toFixed(1)) + " " + units[unit]
}

// "just now", "5 min ago", "3 h ago", "2 days ago"; "never" for 0.
function ago(ms) {
    if (!ms)
        return "never"
    const seconds = Math.max(0, (Date.now() - ms) / 1000)
    if (seconds < 60)
        return "just now"
    if (seconds < 3600)
        return Math.floor(seconds / 60) + " min ago"
    if (seconds < 86400)
        return Math.floor(seconds / 3600) + " h ago"
    const days = Math.floor(seconds / 86400)
    return days + (days === 1 ? " day ago" : " days ago")
}

// A category's icon: by its name when it's a familiar one, otherwise by the kinds it accepts.
function categoryIcon(category) {
    if (!category)
        return "folder"
    const byName = { music: "music", audio: "music", podcasts: "music", movies: "movies", films: "movies",
                     shows: "shows", tv: "shows", series: "shows", books: "books", comics: "books", blogs: "books" }
    const name = category.name.toLowerCase()
    if (byName[name])
        return byName[name]
    const kinds = category.kinds || []
    if (kinds.includes("audio"))
        return "music"
    if (kinds.includes("video"))
        return "movies"
    if (kinds.includes("text"))
        return "books"
    return "folder"
}

// 245000 ms -> "4:05"; an hour or more -> "1:02:05".
function duration(ms) {
    const total = Math.round(Number(ms) / 1000)
    const h = Math.floor(total / 3600), m = Math.floor((total % 3600) / 60), s = total % 60
    const ss = (s < 10 ? "0" : "") + s
    return h > 0 ? h + ":" + (m < 10 ? "0" : "") + m + ":" + ss : m + ":" + ss
}

// 7260000 ms -> "2h 1m"; under an hour -> "52m".
function runtime(ms) {
    const minutes = Math.round(Number(ms) / 60000)
    return minutes >= 60 ? Math.floor(minutes / 60) + "h " + (minutes % 60) + "m" : minutes + "m"
}

// 44100 -> "44.1", 96000 -> "96".
function kilohertz(rate) {
    if (!rate)
        return "?"
    const k = Number(rate) / 1000
    return (Math.round(k * 10) / 10).toString()
}

// A resolution badge: "4K", "1080p", "720p".
function resolution(height) {
    if (height >= 2000)
        return "4K"
    if (height >= 1000)
        return "1080p"
    if (height >= 700)
        return "720p"
    return height > 0 ? height + "p" : ""
}

// Every value of every item's list, in order (Qt's lists from C++ have no flatMap).
function flatten(list, valuesOf) {
    const out = []
    for (let i = 0; i < list.length; ++i) {
        const values = valuesOf(list[i])
        for (let j = 0; j < values.length; ++j)
            out.push(values[j])
    }
    return out
}

// Distinct values, first occurrence order.
function unique(values) {
    const out = []
    for (let i = 0; i < values.length; ++i) {
        if (out.indexOf(values[i]) < 0)
            out.push(values[i])
    }
    return out
}
