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
