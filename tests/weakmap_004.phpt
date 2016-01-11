--TEST--
WeakMap: test detach on unset
--FILE--
<?php
$a = new StdClass;
$b = new StdClass;
$c = new StdClass;
$d = new StdClass;

$wm = new WeakMap;

$wm[$a] = $b;
$wm[$c] = $b;
$wm[$d] = $b;

foreach($wm as $k => $v) {
    var_dump($k);
    var_dump($v);
}

unset($a);

foreach($wm as $k => $v) {
    var_dump($k);
    var_dump($v);
}
?>
==END==
--EXPECTF--
object(stdClass)#1 (0) {
}
object(stdClass)#2 (0) {
}
object(stdClass)#3 (0) {
}
object(stdClass)#2 (0) {
}
object(stdClass)#4 (0) {
}
object(stdClass)#2 (0) {
}
object(stdClass)#3 (0) {
}
object(stdClass)#2 (0) {
}
object(stdClass)#4 (0) {
}
object(stdClass)#2 (0) {
}
==END==
