# What TYLI needs from Plex for posters (hoardor features/posters.md §3), checked on this PC.
# Read-only: it runs SELECT queries with Plex's own SQLite tool and lists files; nothing is changed.
# Run in PowerShell:  powershell -ExecutionPolicy Bypass -File plex_probe.ps1   (or paste it in)

$plex = (Get-ItemProperty 'HKCU:\Software\Plex, Inc.\Plex Media Server' -ErrorAction SilentlyContinue).LocalAppDataPath
if (-not $plex) { $plex = $env:LOCALAPPDATA }
$plex = Join-Path $plex 'Plex Media Server'
$db = Join-Path $plex 'Plug-in Support\Databases\com.plexapp.plugins.library.db'
$sqlite = @("$env:ProgramFiles\Plex\Plex Media Server\Plex SQLite.exe",
            "${env:ProgramFiles(x86)}\Plex\Plex Media Server\Plex SQLite.exe") | Where-Object { Test-Path $_ } | Select-Object -First 1
"Plex folder:   $plex"
"database:      $(Test-Path $db)"
"Plex SQLite:   $sqlite"
"custom folder: $((Get-ItemProperty 'HKCU:\Software\Plex, Inc.\Plex Media Server' -ErrorAction SilentlyContinue).LocalAppDataPath)"
if (-not (Test-Path $db) -or -not $sqlite) { return }

"`n-- poster kinds (movies = 1, shows = 2, seasons = 3)"
& $sqlite -readonly $db "SELECT metadata_type, substr(user_thumb_url, 1, instr(user_thumb_url, '://') + 2) AS kind, count(*) FROM metadata_items WHERE metadata_type IN (1, 2, 3) GROUP BY 1, 2;"

"`n-- three movies and three episodes: the file, its poster, and whether the image is on disk"
$rows = & $sqlite -readonly -separator '|' $db @"
SELECT 'Movies', p.file, m.hash, m.user_thumb_url FROM media_parts p JOIN media_items i ON i.id = p.media_item_id
  JOIN metadata_items m ON m.id = i.metadata_item_id WHERE m.metadata_type = 1 LIMIT 3;
SELECT 'TV Shows', p.file, sh.hash, sh.user_thumb_url FROM media_parts p JOIN media_items i ON i.id = p.media_item_id
  JOIN metadata_items e ON e.id = i.metadata_item_id JOIN metadata_items se ON se.id = e.parent_id
  JOIN metadata_items sh ON sh.id = se.parent_id WHERE e.metadata_type = 4 LIMIT 3;
"@
foreach ($row in $rows) {
    $kind, $file, $hash, $url = $row -split '\|', 4
    $bundle = Join-Path $plex "Metadata\$kind\$($hash.Substring(0, 1))\$($hash.Substring(1)).bundle"
    $image = switch -Regex ($url) {
        '^metadata://(.*)' { Join-Path $bundle "Contents\_combined\$($Matches[1])"; break }
        '^upload://(.*)'   { Join-Path $bundle "Uploads\$($Matches[1])"; break }
        '^media://(.*)'    { Join-Path $plex "Media\localhost\$($Matches[1])"; break }
        default            { '' }
    }
    "`n$file`n  poster: $url`n  image on disk: $(if ($image) { Test-Path -LiteralPath $image } else { 'n/a' })"
    $posters = Join-Path $bundle 'Contents\_combined\posters'
    if (Test-Path -LiteralPath $posters) { "  in the bundle: $((Get-ChildItem -LiteralPath $posters | Select-Object -First 3 | ForEach-Object Name) -join ', ')" }
    else { "  no posters folder in $bundle" }
}
