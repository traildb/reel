
var Color item
var Hedgehogs uint

if Color $color:
    # this is evaluated in the child context
    if $hedgehog:
        inc Hedgehogs 1
else:
    # this is evaluated in the parent context
    if $color:
        fork $color:
            send Color $color
