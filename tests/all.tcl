# Run this to run all the tests
package prefer latest
package require Tcl 8.5
package require tcltest 2.2
tcltest::configure {*}$argv -testdir [file dir [info script]]
tcltest::runAllTests
