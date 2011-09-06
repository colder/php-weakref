--TEST--
Weakref: cloning acquired weakref
--FILE--
<?php

$r = new StdClass;

$wr1 = new WeakRef($r);
var_dump($wr1->acquire());
unset($r);
?>
==END==
--EXPECTF--
bool(true)
==END==
