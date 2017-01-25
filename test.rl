
var is_child uint
var Key item
var Num_worlds uint
var Num_trails uint
var _HIDE uint

begin:
    if is_child:
        inc Num_trails 1
        set _HIDE 0
    else:
        set _HIDE 1

if is_child:
    if $second_field $second_field='world':
        inc Num_worlds 1
else:
    fork $first_field:
        send Key $first_field
        send is_child 1
