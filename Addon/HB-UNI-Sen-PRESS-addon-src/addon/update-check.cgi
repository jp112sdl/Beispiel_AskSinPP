#!/bin/tclsh
set checkURL    "https://raw.githubusercontent.com/jp112sdl/HB-UNI-Sen-PRESS/master/Addon/HB-UNI-Sen-PRESS-addon-src/addon/VERSION"
set downloadURL "https://github.com/jp112sdl/HB-UNI-Sen-PRESS/raw/master/Addon/HB-UNI-Sen-PRESS-addon.tgz"

catch {
  set input $env(QUERY_STRING)
  set pairs [split $input &]
  foreach pair $pairs {
    if {0 != [regexp "^(\[^=]*)=(.*)$" $pair dummy varname val]} {
      set $varname $val
    }
  }
}

if { [info exists cmd ] && $cmd == "download"} {
  puts "<html><head><meta http-equiv='refresh' content='0; url=$downloadURL' /></head></html>"
} else {
  catch {
    set newversion [ exec /usr/bin/wget -qO- --no-check-certificate $checkURL ]
  }
  if { [info exists newversion] } {
    puts $newversion
  } else {
    puts "n/a"
  }
}
