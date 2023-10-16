--TEST--
XHProf: Test Sampling Interval
Author: longxinhui
--INI--
xhprof.sampling_interval = 400000
--SKIPIF--
<?php
if (substr(PHP_OS, 0, 3) == 'WIN') {
    print 'skip';
}
?>
--FILE--
<?php

include_once dirname(__FILE__).'/common.php';

function foo() {
   // sleep 0.8 seconds
   usleep(800000);
}

function bar() {
   foo();
}

function goo() {
    bar();
}

// call goo() once
xhprof_sample_enable();
goo();
$output = xhprof_sample_disable();
$interval = array_keys($output);
$interval_time = sprintf("%.6f", $interval[1] - $interval[0]);
if ($interval_time == '0.400000') {
    echo 'Test passed';
} else {
    var_dump($output);
    echo $interval_time;
}

echo "\n";

?>
--EXPECTF--
Test passed
