#!/usr/bin/env bats

load test_helper

@test "gratuit" {
	true
}

@test "20x" {
	run rm count
	run ./tally cat tests/nevermind : cat : cat : cat : cat : cat : cat : cat : cat : cat : cat : cat : cat : cat : cat : cat : cat : cat : cat : cat : wc -c
	check 0 "1922"
	diff -u - <(sort -n "count") <<FIN
1 : 1922
2 : 1922
3 : 1922
4 : 1922
5 : 1922
6 : 1922
7 : 1922
8 : 1922
9 : 1922
10 : 1922
11 : 1922
12 : 1922
13 : 1922
14 : 1922
15 : 1922
16 : 1922
17 : 1922
18 : 1922
19 : 1922
20 : 1922
FIN
}

@test "largerfile" {
	run rm count
	run ./tally cat cmd.png : cat : grep END : wc -w
	check 0 "5"
	diff -u - <(sort -n "count") <<FIN
1 : 14539
2 : 14539
3 : 37
FIN
}
