# The Weakref PECL extension
A weak reference provides a gateway to an object without preventing that object
from being collected by the garbage collector (GC). It allows to associate
information to volatile object. It is useful to associate metadata or cache
information to objects. Indeed, the cache entry should not be preventing the
garbage collection of the object AND the cached info when the object is no
longer used.

More information about releases can be found on http://pecl.php.net/weakref

## Weakref
The weakref class is a simple class that allows to access its referenced object
as long as it exists. Unlike other references, having this Weakref object will
not prevent the object to be collected.

```php
<?php
class MyClass {
    public function __destruct() {
        echo "Destroying object!\n";
    }
}

$o1 = new MyClass;

$r1 = new Weakref($o1);

if ($r1->valid()) { // It does
    echo "Object still exists!\n";
    var_dump($r1->get());
} else {
    echo "Object is dead!\n";
}

unset($o1);

if ($r1->valid()) { // It doesn't
    echo "Object still exists!\n";
    var_dump($r1->get());
} else {
    echo "Object is dead!\n";
}
?>
```

## Weakmap
The Weakmap class is very similar to Weakref, only that it also allows to
associate data to each object. When the target object gets destroyed, the
associated data is automatically freed.

```php
<?php
$wm = new WeakMap();

$o = new StdClass;

class A {
    public function __destruct() {
        echo "Dead!\n";
    }
}

$wm[$o] = new A;

var_dump(count($wm)); // int(1)
echo "Unsetting..\n";
unset($o); // Will destroy the 'new A' object as well
echo "Done\n";
var_dump(count($wm)); // int(0)
?>
```

As of PHP7, iterating WeakMap provides access to both the key (the reference)
and the value:

```php
<?php
$wm = new WeakMap();

$wmk = new StdClass;
$wmv = new StdClass;

$wm[$wmk] = $wmv;

foreach($wm as $k => $v) {
    // $k == $wmk
    // $v == $wmv
}
?>
```
