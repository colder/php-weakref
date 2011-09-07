--TEST--
WeakMap: usage
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
echo "Unsetting..\n";
unset($o);
echo "Done\n";
var_dump(count($wm));
?>
==END==
--EXPECTF--
int(1)
Unsetting..
Dead!
Done
int(0)
==END==
