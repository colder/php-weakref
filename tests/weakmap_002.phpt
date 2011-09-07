--TEST--
WeakMap: unsetting
--FILE--
<?php
$wm = new WeakMap();

$o = new StdClass;

class A {
    public function __destruct() {
        echo "Dead!\n";
    }
}

$wm[$o] = new A;

var_dump(count($wm));
unset($wm[$o]);
var_dump(count($wm));
?>
==END==
--EXPECTF--
int(1)
Dead!
int(0)
==END==
