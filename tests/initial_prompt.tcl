#!/usr/bin/env tclsh
# Optional model-dependent Slice 5 probe; not part of `make test`.
if {$argc != 2 && !($argc == 3 && [lindex $argv 2] in {--short-error --auto --compat})} {
    error "usage: initial_prompt.tcl MODEL AUDIO_DIRECTORY ?--short-error|--auto|--compat?"
}
package require tclwhisper

set prompt "Iik', squawk, ILS, NOTAM, waypoint, VFR, heading, KREPE, Chihuahua."
set expected {
    test1.f32 { y confirma el Squawk 7700}
    test2.f32 { Estamos establecidos en el ILS de la pista 2-1.}
    test3.f32 { Revisa el Notam para Chihuahua.}
    test4.f32 { El siguiente Waypoint es Kilo Romeo Echo Papa Echo}
    test5.f32 { Confirma BFR y mantén Herring 270.}
}

proc readPcm {path} {
    set channel [open $path rb]
    try {return [read $channel]} finally {close $channel}
}

proc runCall {handle pcm mode prompt} {
    set options {-language spanish -n_threads 4}
    if {$mode eq "prompt"} {
        lappend options -initial_prompt $prompt
    } elseif {$mode eq "empty"} {
        lappend options -initial_prompt ""
    }
    set start [clock microseconds]
    set text [whisper::transcribe $handle $pcm {*}$options]
    set seconds [expr {([clock microseconds] - $start) / 1000000.0}]
    return [list $text $seconds]
}

if {$argc == 3} {
    if {[lindex $argv 2] eq "--compat"} {
        set pcm [readPcm [file join [lindex $argv 1] audio1.f32]]
        set handle [whisper::init [lindex $argv 0]]
        try {
            foreach {mode options} {
                default {}
                language {-language spanish}
                threads {-n_threads 4}
                combined {-language spanish -n_threads 4}
            } {
                set start [clock microseconds]
                set text [whisper::transcribe $handle $pcm {*}$options]
                set seconds [expr {([clock microseconds] - $start) / 1000000.0}]
                puts [list audio1.f32 $mode $seconds $text]
                flush stdout
            }
        } finally {
            whisper::free $handle
        }
        exit 0
    }
    if {[lindex $argv 2] eq "--auto"} {
        set pcm [readPcm [file join [lindex $argv 1] test5.f32]]
        set handle [whisper::init [lindex $argv 0]]
        try {
            foreach mode {baseline prompt} {
                set options {-language auto -n_threads 4}
                if {$mode eq "prompt"} {lappend options -initial_prompt $prompt}
                set start [clock microseconds]
                set text [whisper::transcribe $handle $pcm {*}$options]
                set seconds [expr {([clock microseconds] - $start) / 1000000.0}]
                puts [list test5.f32 auto $mode $seconds $text]
                flush stdout
            }
        } finally {
            whisper::free $handle
        }
        exit 0
    }
    set channel [open [file join [lindex $argv 1] test5.f32] rb]
    try {set pcm [read $channel 80]} finally {close $channel}
    set handle [whisper::init [lindex $argv 0]]
    try {
        set code [catch {
            whisper::transcribe $handle $pcm -language auto -n_threads 4 \
                -initial_prompt $prompt
        } result options]
        puts [list short_error $code $result [dict get $options -errorcode]]
        if {!$code || [dict get $options -errorcode] ne
                {TCLWHISPER TRANSCRIBE FAILED}} {
            error "expected a whisper_full failure for 20 samples"
        }
    } finally {
        whisper::free $handle
    }
    exit 0
}

set handle [whisper::init [lindex $argv 0]]
set audioDirectory [lindex $argv 1]
try {
    foreach {audio baseline} $expected {
        set pcm [readPcm [file join $audioDirectory $audio]]
        set modes [expr {$audio eq "test5.f32" ? {prompt baseline empty} : {baseline prompt}}]
        foreach mode $modes {
            lassign [runCall $handle $pcm $mode $prompt] text seconds
            dict set observed $audio $mode $text
            puts [list $audio $mode $seconds $text]
            flush stdout
        }
        if {[dict get $observed $audio baseline] ne $baseline} {
            error "baseline mismatch for $audio"
        }
    }
    if {[dict get $observed test5.f32 prompt] ne
            { Confirma VFR y mantén heading 270.}} {
        error "test5 prompted result did not reproduce the domain-term correction"
    }
    if {[dict get $observed test5.f32 empty] ne
            [dict get $observed test5.f32 baseline]} {
        error "empty prompt differed from omission on test5"
    }
    puts "PASS five baseline comparisons, test5 correction, empty-prompt equivalence, prompt-to-no-prompt nonpersistence"
} finally {
    whisper::free $handle
}
