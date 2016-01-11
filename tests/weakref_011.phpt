--TEST--
Weakref: cloning
--FILE--
<?php
$o = new StdClass;

$wr1 = new WeakRef($o);
$wr2 = clone $wr1;

unset($o);

var_dump($wr1->valid());
var_dump($wr2->valid());
?>
==END==
--EXPECTF--
bool(false)
bool(false)
==END==
