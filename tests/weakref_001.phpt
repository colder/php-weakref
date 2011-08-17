--TEST--
Weakref: usage
--FILE--
<?php
$o = new StdClass;

$wr = new WeakRef($o);

var_dump($wr->valid(), $wr->get());
unset($o);
var_dump($wr->valid(), $wr->get());
?>
==END==
--EXPECTF--
bool(true)
object(stdClass)#1 (0) {
}
bool(false)
NULL
==END==
