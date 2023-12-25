# bayan

## Options
`sc`   > directories for scan (\"path\", ..., \"path\")
`unsc` > directories for unscan (\"path\", ..., \"path\")
`l`    > level in tree of directories for scan (0..)
`msf`  > minimal size of file (1..)
`mask` > mask of file for scan (regexp)
`sb`   > size of block in file (bytes)
`hash` > algorithm of hash (md5, crc32)

## Example

bayan --sc "path" "path" --unsc "path" "path" --l 2 --msf 5 --mask "*" --sb 20 --hash md5