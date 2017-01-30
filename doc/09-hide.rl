
var Color item
var Hedgehogs uint
var _HIDE uint

begin:
    if Color:
        set _HIDE 0
    else:
        set _HIDE 1

if Color $color:
    # this is evaluated in the child context
    if $hedgehog:
        inc Hedgehogs 1
else:
    # this is evaluated in the parent context
    if $color:
        fork $color:
            send Color $color
