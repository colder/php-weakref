--TEST--
WeakMap: cloning
--FILE--
<?php
$a = new StdClass;
$b = new StdClass;
$c = new StdClass;
$d = new StdClass;
$e = new StdClass;

$wm1 = new WeakMap;

$wm1[$a] = $b;
$wm1[$c] = $b;
$wm1[$d] = $b;

$wm2 = clone $wm1;

$wm1[$e] = $b;

unset($a);


var_dump($wm1->count());
var_dump($wm2->count());
?>
==END==
--EXPECTF--
int(3)
int(2)
==END==
