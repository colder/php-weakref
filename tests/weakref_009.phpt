--TEST--
Weakref: zval splitting causes crash
--FILE--
<?php
$obj = new StdClass();
$ref = new WeakRef($obj);
echo "get...\n";
$obj2 = $ref->get();
echo "unset1...\n";
unset($obj);
echo "unset2...\n";
unset($ref);
echo "done...\n";
?>
--EXPECTF--
get...
unset1...
unset2...
done...
