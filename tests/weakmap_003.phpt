--TEST--
WeakMap: test detach on unset
--FILE--
<?php
$a = new StdClass;
$b = new StdClass;

$wm = new WeakMap;

$wm[$a] = $b;

unset($wm[$a]);
unset($wm);

unset($a);
?>
==END==
--EXPECTF--
==END==
