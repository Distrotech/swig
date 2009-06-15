<?php

require "tests.php";
require "director_finalizer.php";

// No new functions
check::functions(array(foo_orstatus,deletefoo,getstatus,launder,resetstatus));
// No new classes
check::classes(array(director_finalizer,Foo));
// now new vars
check::globals(array());

class MyFoo extends Foo {
  function __destruct() {
    $this->orStatus(2);
    deleteFoo($this);
  }
}

resetStatus();

$a = new MyFoo();
unset($a);

check::equal(getStatus(), 3, "getStatus() failed #1");

resetStatus();

$a = new MyFoo();
launder($a);

check::equal(getStatus(), 0, "getStatus() failed #2");

unset($a);

check::equal(getStatus(), 3, "getStatus() failed #3");

resetStatus();

$a = new MyFoo();
$a->thisown = 1;
deleteFoo($a);

check::equal(getStatus(), 3, "getStatus() failed #4");

resetStatus();

$a = new MyFoo();
$a->thisown = 1;
deleteFoo(launder($a));

check::equal(getStatus(), 3, "getStatus() failed #4");

resetStatus();

check::done();
?>
