#!/usr/bin/env bats

load test_helper

@test "simple" {
    run rm count
	run ./tally wc -c tests/nevermind
	check 0 "1922 tests/nevermind"
	run ls count
	check 2 "ls: cannot access 'count': No such file or directory"
}

@test "redirection" {
	run rm count
	run ./tally echo bonjour le monde : wc -c
	check 0 "17"
	diff -u - <(sort -n "count") <<FIN
1 : 17
FIN
}

@test "stderr" {
	run rm count
	run ./tally bash -c 'echo bonjour le monde >&2' % true
	check 0 "bonjour le monde"
}


@test "multiple" {
    run rm count
	run ./tally cat tests/nevermind : grep -v le : grep n : wc -l
	check 0 "64"
	diff -u - <(sort -n "count") <<FIN
1 : 1922
2 : 1727
3 : 1070
FIN
}

@test "false" {
    run rm count
	run ./tally cat tests/nevermind : grep left : false
	check 1 ""
	diff -u - <(sort -n "count") <<FIN
1 : 1922
2 : 42
FIN
}

@test "stdin" {
    run rm count
	run bash -c "./tally < tests/nevermind cat : grep the : wc -c"
	check 0 "243"
	diff -u - <(sort -n "count") <<FIN
1 : 1922
2 : 243
FIN
}

@test "stdout" {
    run rm count
	run bash -c "./tally > /dev/null cat tests/nevermind : grep left : wc -c"
	check 0 ""
	diff -u - <(sort -n "count") <<FIN
1 : 1922
2 : 42
FIN
}

@test "middle-pause" {
    run rm count
	run ./tally cat tests/nevermind : grep the : sleep 1 : grep -v the tests/nevermind : wc -c
	check 0 "1679"
	diff -u - <(sort -n "count") <<FIN
1 : 1922
2 : 243
4 : 1679
FIN
}

@test "fail" {
    run rm count
	run ./tally cat tests/nevermind : cmd-fail
	check 127 ""
}

@test "tordu" {
    run rm count
	run ./tally cat tests/nevermind : grep left : ./tally wc -c
	check 0 "42"
	diff -u - <(sort -n "count") <<FIN
1 : 1922
2 : 42
FIN
}
