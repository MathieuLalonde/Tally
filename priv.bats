#!/usr/bin/env bats

load test_helper

@test "cadeau-noel" {
	run ./tally wc -l tests/nevermind
	check 0 "125 tests/nevermind"
}

@test "cadeau-nouvel-an" {
	run rm count
	run ./tally echo hello world! : wc -c
	check 0 "13"
}

@test "nada" {
	run ./tally sleep 1 : sleep 2 : sleep 3
	check 0 ""
}

@test "fail" {
	run ./tally fail
	check 127 ""
}

@test "cat" {
    run rm count
	run ./tally cat tests/nevermind : grep left : cat : cat : cat : cat : cat : cat : cat : cat : wc -c
	check 0 "42"
	diff -u - <(sort -n "count") <<FIN
1 : 1922
2 : 42
3 : 42
4 : 42
5 : 42
6 : 42
7 : 42
8 : 42
9 : 42
10 : 42
FIN
}

@test "stderr" {
    run rm count
	run bash -c "./tally cat tests/nevermind : grep left : wc -c fail 2> /dev/null"
	check 1 ""
	diff -u - <(sort -n "count") <<FIN
1 : 1922
2 : 42
FIN
}

@test "tee" {
    run rm count
	run ./tally cat tests/nevermind : grep left : tee transit : wc -c
	check 0 "42"
	diff -u - <(sort -n "count") <<FIN
1 : 1922
2 : 42
3 : 42
FIN
	diff -u - transit <<FIN
I left behind
I left behind
I left behind
FIN
}

@test "kill" {
	run ./tally bash -c 'kill $$'
	check 143 ""
}

@test "signal" {
    run rm count
	run ./signal
	check 130 ""
}

@test "fermeture-fd-simple" {
	run ./tally ls /proc/self/fd/ : wc -l
	check 0 "5"
}
