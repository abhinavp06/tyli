# Lists every folder and file under a folder, with file sizes, into a text file on the Desktop.
# Read-only: it changes nothing. Used to share a library's layout (names, folders) for debugging.
#
# Run: open PowerShell, change the two lines below, paste the whole script.

$Root = 'D:\SHOWS'          # <- your folder
$Name = 'shows_list.txt'    # <- the output file's name (it goes on your Desktop)

$out  = Join-Path ([Environment]::GetFolderPath('Desktop')) $Name
$Root = $Root.TrimEnd('\')

# -LiteralPath: PowerShell treats [ and ] as wildcards otherwise, and release names are full of them.
Get-ChildItem -LiteralPath $Root -Recurse -Force -ErrorAction SilentlyContinue -ErrorVariable problems |
  Sort-Object FullName |
  ForEach-Object {
    $rel = $_.FullName.Substring($Root.Length + 1)
    if ($_.PSIsContainer) { "DIR          $rel\" }
    else { '{0,10:N0} KB  {1}' -f [math]::Ceiling($_.Length / 1KB), $rel }
  } | Out-File -LiteralPath $out -Encoding utf8

if ($problems) {
    "`nCOULD NOT READ:" | Out-File -LiteralPath $out -Append -Encoding utf8
    $problems | ForEach-Object { $_.TargetObject } | Out-File -LiteralPath $out -Append -Encoding utf8
}
"Wrote $out"
