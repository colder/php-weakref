--TEST--
Weakref: usage
--FILE--
<?php
$o = new StdClass;

$wr = new WeakRef($o);
$wr->acquire();
$wr->acquire();
var_dump($wr->valid(), $wr->get());
unset($o);

$wr->release();
var_dump($wr->valid(), $wr->get());

$wr->release();
var_dump($wr->valid(), $wr->get());
?>
==END==
--EXPECTF--
bool(true)
object(stdClass)#1 (0) {
}
bool(true)
object(stdClass)#1 (0) {
}
bool(false)
NULL
==END==
