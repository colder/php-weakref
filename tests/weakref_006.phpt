--TEST--
Weakref: Destroying the weakref within the std dtor of the object
--FILE--
<?php

class A {
    private $wr = null;
    public function __construct() {
        $this->wr = new WeakRef($this);
    }
    public function __destruct() {
        unset($this->wr);
    }
}

$a = new A;
unset($a);
?>
==END==
--EXPECTF--
==END==
