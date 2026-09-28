#!/usr/bin/env tclsh

if {$argc < 2 || $argc > 3} {
    puts stderr "usage: tclsh tests/integration.tcl MODEL PCM ?LANGUAGE?"
    exit 2
}

set model [lindex $argv 0]
set pcmFile [lindex $argv 1]
set language [expr {$argc == 3 ? [lindex $argv 2] : "default"}]

set scriptDir [file dirname [file normalize [info script]]]
lappend auto_path [file dirname $scriptDir]
package require tclwhisper

set channel [open $pcmFile rb]
set pcm [read $channel]
close $channel

set handle [whisper::init $model]
try {
    if {$language eq "default"} {
        set text [whisper::transcribe $handle $pcm]
    } else {
        set text [whisper::transcribe $handle $pcm -language $language]
    }
    puts $text
} finally {
    whisper::free $handle
}
