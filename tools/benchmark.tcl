#!/usr/bin/env tclsh

proc usage {} {
    puts stderr "usage: tclsh tools/benchmark.tcl MODEL PCM ?LANGUAGE? ?RUNS?"
    puts stderr "       LANGUAGE defaults to default; RUNS defaults to 5"
    exit 2
}

if {$argc < 2 || $argc > 4} {
    usage
}

set model [lindex $argv 0]
set pcmFile [lindex $argv 1]
set language [expr {$argc >= 3 ? [lindex $argv 2] : "default"}]
set runs [expr {$argc >= 4 ? [lindex $argv 3] : 5}]

if {![string is integer -strict $runs] || $runs <= 0} {
    puts stderr "RUNS must be a positive integer"
    exit 2
}

set scriptDir [file dirname [file normalize [info script]]]
set sourceRoot [file dirname $scriptDir]
lappend auto_path $sourceRoot
package require tclwhisper

set channel [open $pcmFile rb]
set pcm [read $channel]
close $channel

set byteLength [string length $pcm]
if {$byteLength % 4 != 0} {
    puts stderr "PCM size must be a multiple of 4 bytes"
    exit 2
}
if {$byteLength == 0} {
    puts stderr "PCM must be non-empty for an RTF benchmark"
    exit 2
}

set audioSeconds [expr {$byteLength / 4.0 / 16000.0}]
set initStart [clock microseconds]
set handle [whisper::init $model]
set initSeconds [expr {([clock microseconds] - $initStart) / 1000000.0}]

puts "tclwhisper_version=[whisper::version]"
puts "language=$language"
puts "runs=$runs"
puts "pcm_bytes=$byteLength"
puts "audio_seconds=$audioSeconds"
puts "init_seconds=$initSeconds"

set timings {}
try {
    for {set run 1} {$run <= $runs} {incr run} {
        set start [clock microseconds]
        if {$language eq "default"} {
            set text [whisper::transcribe $handle $pcm]
        } else {
            set text [whisper::transcribe $handle $pcm -language $language]
        }
        set seconds [expr {([clock microseconds] - $start) / 1000000.0}]
        set rtf [expr {$seconds / $audioSeconds}]
        lappend timings $seconds
        puts "run=$run seconds=$seconds rtf=$rtf chars=[string length $text] text=<$text>"
    }
} finally {
    whisper::free $handle
}

set sorted [lsort -real $timings]
set minimum [lindex $sorted 0]
set maximum [lindex $sorted end]
set total 0.0
foreach value $timings {
    set total [expr {$total + $value}]
}
set average [expr {$total / $runs}]
if {$runs % 2} {
    set median [lindex $sorted [expr {$runs / 2}]]
} else {
    set middle [expr {$runs / 2}]
    set median [expr {([lindex $sorted [expr {$middle - 1}]] +
                       [lindex $sorted $middle]) / 2.0}]
}

puts "summary_runs=$runs"
puts "summary_min_seconds=$minimum"
puts "summary_max_seconds=$maximum"
puts "summary_average_seconds=$average"
puts "summary_median_seconds=$median"
