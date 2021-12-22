#!/bin/sh
test -z "$HD" && HD=../haxdiff
TMP=/tmp/haxdiff.test.$$
tmp() {
	echo $TMP.$testno
}
md5() {
	md5sum "$1"|cut -d " " -f 1
}
fs() {
	wc -c "$1"|cut -d " " -f 1
}
equal() {
	test $(md5 "$1") = $(md5 "$2")
}
equal_size() {
	test $(fs "$1") = $(fs "$2")
}
cleanup() {
	rm -f $(tmp).1 $(tmp).2 $(tmp).3 $(tmp).4
}
test_equal() {
	if equal "$1" "$2" ; then
		cleanup
	else
		echo "test $testno failed."
		echo "inspect $(tmp).* for analysis"
	fi
}
test_equal_size() {
	if equal_size "$1" "$2" ; then
		cleanup
	else
		echo "test $testno failed."
		echo "inspect $(tmp).* for analysis"
	fi
}
dotest() {
	[ -z "$testno" ] && testno=0
	testno=$((testno + 1))
	echo "running test $testno ($1)"
}

patched=a1024_mod2.dat
dotest "$patched"
$HD d a1024.dat "$patched" > $(tmp).1
$HD p a1024.dat $(tmp).2 < $(tmp).1
test_equal $(tmp).2 "$patched"

patched=a1024_mod2_exp.dat
dotest "$patched"
$HD d a1024.dat "$patched" > $(tmp).1
$HD p a1024.dat $(tmp).2 < $(tmp).1
test_equal $(tmp).2 "$patched"

patched=a1024_mod2_trunc.dat
dotest "$patched"
$HD d a1024.dat "$patched" > $(tmp).1
$HD p a1024.dat $(tmp).2 < $(tmp).1
test_equal $(tmp).2 "$patched"

patched=a1024_mod3.dat
dotest "$patched"
$HD d a1024.dat "$patched" > $(tmp).1
$HD p a1024.dat $(tmp).2 < $(tmp).1
test_equal $(tmp).2 "$patched"

patched=a1024_mod3_exp.dat
dotest "$patched"
$HD d a1024.dat "$patched" > $(tmp).1
$HD p a1024.dat $(tmp).2 < $(tmp).1
test_equal $(tmp).2 "$patched"

patched=a1024_mod3_trunc.dat
dotest "$patched"
$HD d a1024.dat "$patched" > $(tmp).1
$HD p a1024.dat $(tmp).2 < $(tmp).1
test_equal $(tmp).2 "$patched"

## testing corrupt input

patched=a1024_mod2.dat
dotest "$patched corrupt"
$HD d a1024.dat "$patched" > $(tmp).1
$HD P a1024_corrupt.dat $(tmp).2 2>/dev/null < $(tmp).1
test_equal $(tmp).2 "$patched"

patched=a1024_mod2_exp.dat
dotest "$patched corrupt"
$HD d a1024.dat "$patched" > $(tmp).1
$HD P a1024_corrupt.dat $(tmp).2 2>/dev/null < $(tmp).1
test_equal $(tmp).2 "$patched"

patched=a1024_mod2_trunc.dat
dotest "$patched corrupt"
$HD d a1024.dat "$patched" > $(tmp).1
$HD P a1024_corrupt.dat $(tmp).2 2>/dev/null < $(tmp).1
test_equal $(tmp).2 "$patched"

## test corruption failure

patched=a1024_mod2.dat
dotest "$patched corrupt fail"
$HD d a1024.dat "$patched" > $(tmp).1
$HD p a1024_corrupt.dat $(tmp).2 2>/dev/null < $(tmp).1 && echo "test $testno failed." || cleanup

patched=a1024_mod2_exp.dat
dotest "$patched corrupt fail"
$HD d a1024.dat "$patched" > $(tmp).1
$HD p a1024_corrupt.dat $(tmp).2 2>/dev/null < $(tmp).1 && echo "test $testno failed." || cleanup

patched=a1024_mod2_trunc.dat
dotest "$patched corrupt fail"
$HD d a1024.dat "$patched" > $(tmp).1
$HD p a1024_corrupt.dat $(tmp).2 2>/dev/null < $(tmp).1 && echo "test $testno failed." || cleanup
