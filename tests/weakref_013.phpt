--TEST--
Weakref: usage
--FILE--
<?php
$wr = [];
for($i = 0; $i < 2; $i++) {
    $c = new stdClass();
    $wr[$i] = new Weakref($c);
    $wr[$i]->acquire();
}
unset($wr[0]);
?>
==END==
--EXPECTF--
==END==

