--TEST--
Weakref: usage
--FILE--
<?php

$r = new StdClass;


$wr1 = new WeakRef($r);
var_dump($wr1->valid());
unset($wr1);

$wr2 = new WeakRef($r);
var_dump($wr2->valid());
unset($wr2);
?>
==END==
--EXPECTF--
bool(true)
bool(true)
==END==
