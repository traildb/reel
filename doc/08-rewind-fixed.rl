
var last_is_yellow uint
var prev_is_yellow uint
var Blues uint

begin:
    unset last_is_yellow
    unset prev_is_yellow

if last_is_yellow and if $color $color='blue':
    inc Blues 1
if $color $color='yellow':
    set prev_is_yellow 1
else:
    set prev_is_yellow 0

end:
    if prev_is_yellow and not if last_is_yellow:
        set last_is_yellow 1
        rewind
