#!/usr/bin/env tclsh
# Run with TCLLIBPATH pointing at the build and a real model path.
if {$argc != 1} {error "usage: options.tcl MODEL"}
package require tclwhisper
set h [whisper::init [lindex $argv 0]]
set checked 0
proc check {h pcm options expected} {
    set code [catch {whisper::transcribe $h $pcm {*}$options} result]
    if {$expected eq "OK"} {
        if {$code || $result ne ""} {error "unexpected result for $options: $result"}
    } else {
        if {!$code || [lrange $::errorCode 0 2] ne $expected} {
            error "unexpected error for $options: $result"
        }
    }
    incr ::checked
}
try {
    foreach options {{} {-language es} {-language spanish} {-language auto}
        {-n_threads 1} {-n_threads 2} {-n_threads 4} {-n_threads 8}
        {-n_threads 2147483647} {-language es -n_threads 4}
        {-n_threads 4 -language es}
        {-initial_prompt {Iik', VFR, heading}} {-initial_prompt {}}
        {-language spanish -initial_prompt {Iik', VFR}}
        {-initial_prompt {Iik', VFR} -language spanish}
        {-n_threads 4 -initial_prompt {Iik', VFR}}
        {-initial_prompt {Iik', VFR} -n_threads 4}
        {-language spanish -n_threads 4 -initial_prompt {Iik', VFR}}
        {-initial_prompt {Iik', VFR} -n_threads 4 -language spanish}} {
        check $h {} $options OK
    }
    foreach options {{-unknown x} {-language es -unknown x} {-n_threads 2 -unknown x}
        {-unknown x -initial_prompt foo} {-initial_prompt foo -unknown x}
        {-initial_prompt foo -n_threads 2 -unknown x}} {
        foreach pcm {{} x} {
            check $h $pcm $options {TCLWHISPER OPTION UNKNOWN}
        }
    }
    foreach options {{-language} {-n_threads} {-language es -n_threads}
        {-n_threads 2 -language} {-initial_prompt}
        {-language spanish -initial_prompt}
        {-n_threads 4 -initial_prompt}} {
        check $h {} $options {TCL WRONGARGS}
    }
    foreach options {{-language es -language en} {-n_threads 1 -n_threads 2}
        {-language es -n_threads 2 -language en}
        {-n_threads 2 -language es -n_threads 4}
        {-initial_prompt foo -initial_prompt bar}
        {-language spanish -initial_prompt foo -initial_prompt bar}
        {-initial_prompt foo -n_threads 4 -initial_prompt bar}} {
        check $h {} $options {TCLWHISPER OPTION DUPLICATE}
    }
    check $h x {-initial_prompt} {TCL WRONGARGS}
    check $h x {-initial_prompt foo -initial_prompt bar} {TCLWHISPER OPTION DUPLICATE}
    check $h {} {-initial_prompt foo -n_threads 0} {TCLWHISPER N_THREADS INVALID}
    check $h {} {-initial_prompt foo -language nonexistent} {TCLWHISPER LANGUAGE INVALID}
    check invalidHandle {} {-language spanish -n_threads 4 -initial_prompt foo} {TCLWHISPER HANDLE INVALID}
    foreach value {0 -1 abc {} 1.5 2147483648 4294967297 9223372036854775808 18446744073709551617 -18446744073709551615} {
        foreach pcm {{} x} {
            check $h $pcm [list -n_threads $value] {TCLWHISPER N_THREADS INVALID}
        }
    }
    foreach value {{} ES {es } nonexistent} {
        foreach options [list [list -language $value] [list -n_threads 2 -language $value]] {
            check $h {} $options {TCLWHISPER LANGUAGE INVALID}
        }
    }
} finally {whisper::free $h}
puts "PASS $checked option checks"
