
load [file dirname [info script]]/jsmin.so

set jsm {}

proc jsminwrap {s} {
	if {![dict exists $::jsm $s]} {
		dict set ::jsm $s [jsmin $s]
	}
	return [dict get $::jsm $s]
}

proc jsminwrapf {s f} {
	if {![dict exists $::jsm $f]} {
		dict set ::jsm $f [jsmin $s]
	}
	return [dict get $::jsm $f]
}

set f [open [lindex $argv 0]]
set dat [read $f]

set out [jsmin $dat]
puts $out

puts "[lindex $argv 0] ratio: [format "%3.2f" [expr {double([string length [jsmin $dat]])/[string length $dat]}]]"

puts -nonewline jsmin:
puts [time {jsmin $dat} 1000]

puts -nonewline jsminwrap:
puts [time {jsminwrap $dat} 1000]

# puts -nonewline jsminwrapf:
# puts [time {jsminwrapf $dat [lindex $argv 0]} 1000]
