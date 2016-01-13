--TEST--
Weakref: Destroying the weakref and its object after a fatal error
--SKIPIF--
<?php if(!function_exists('pcntl_fork')) echo "skip can't test for returnvalue"; ?>
--FILE--
<?php
	class A {
		function __destruct() {
			printf("Destroy A\n");
		}
	}
	class B {
		public $ref;

		function __construct($a) {
			$this->ref = new WeakRef($a);
		}

		function __destruct() {
			printf("Destroy B\n");
			var_dump($this->ref->valid());
		}
	}
	function doit() {
		$a = new A();
		$b = new B($a);
		crash();
	}
	$pid = pcntl_fork();
	if($pid == 0) {
		doit();
		exit(0);
	}
	pcntl_waitpid($pid, $status);
	if(pcntl_wifsignaled($status)) {
		echo "Killed: ". pcntl_wtermsig($status) ."\n";
	} elseif(pcntl_wifexited($status)) {
		echo "Exit: ". pcntl_wexitstatus($status) ."\n";
	} else {
		echo "Weird.\n";
	}
?>
--EXPECTF--
Destroy A
Destroy B
bool(false)

Fatal error: Uncaught Error: Call to undefined function crash() in %s:%d
Stack trace:
#0 %s(%d): doit()
#1 {main}
  thrown in %s on line %d
Exit: 255
