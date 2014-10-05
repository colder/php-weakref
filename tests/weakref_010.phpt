--TEST--
Weakref: Crash
--FILE--
<?php
class RefHolder {
  function __construct($o) {
    $this->o = new WeakRef($o);
  }
  function __destruct() {
    echo "Destroying RefHolder\n";

  }
}


$o = new StdClass();

$map = new WeakMap();
$map[$o] = "";
echo "Storing...\n";
$map[$o] = new RefHolder($o);

echo "Unsetting...\n";
unset($o);
echo "OK\n";
?>
--EXPECT--
Storing...
Unsetting...
Destroying RefHolder
OK
