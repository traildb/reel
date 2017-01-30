
var Pairs uint
var hedgehog_time uint
var window uint

if $hedgehog:
    if hedgehog_time and time_before hedgehog_time window:
        inc Pairs 1
    set hedgehog_time $time
else:
    set hedgehog_time 0
