<?php

$_original_functions=get_defined_functions();
$_original_globals=1;
$_original_globals=array_keys($GLOBALS);

class check {
  function classes($classes) {
    if (! is_array($classes)) $classes=array($classes);
    $missing=array();
    foreach($classes as $class) if (! class_exists($class)) $missing[]=$class;
    if ($missing) return check::fail("Classes missing: %s",join(",",$missing));
    return TRUE;
  }

  function get_extra_functions($ref=FALSE) {
    static $extra;
    global $_original_functions;
    if ($ref===FALSE) $f=$_original_functions;
    if (! is_array($extra)) {
      $df=get_defined_functions();
      $df=array_flip($df[internal]);
      foreach($_original_functions[internal] as $func) unset($df[$func]);
      $extra=array_keys($df);
    }
    return $extra;
  }

  function get_extra_globals($ref=FALSE) {
    static $extra;
    global $_original_globals;
    if ($ref===FALSE) $ref=$_original_globals;
    if (! is_array($extra)) {
      $df=array_flip(array_keys($GLOBALS));
      foreach($_original_globals as $func) unset($df[$func]);
      $extra=array_keys($df);
    }
    return $extra;
  }

  function classname($string,$object) {
    if (! $string==($classname=get_class($object))) return check::fail("Object: $object is of class %s not class %s",$classname,$string);
    return TRUE;
  }

  function classmethods($classname,$methods) {
    if (is_object($classname)) $classname=get_class($classname);
    $classmethods=array_flip(get_class_methods($classname));
    $missing=array();
    $extra=array();
    foreach($methods as $method) {
      if (! isset($classmethods[$method])) $missing[]=$method;
      else unset($classmethods[$method]);
    }
    $extra=array_keys($classmethods);
    if ($missing) $message[]="does not have these methods:\n  ".join(",",$missing);
    if ($extra) $message[]="does have these extra methods:\n  ".join(",",$extra);
    if ($message) {
      return check::fail("Class %s %s\nFull class list:\n  %s\n",$classname,join("\nbut ",$message),join("\n  ",get_class_methods($classname)));
    }
    return TRUE;
  }

  function is_a($a,$b) {
    if (is_object($a)) $a=strtolower(get_class($a));
    if (is_object($b)) $a=strtolower(get_class($b));
    $parents=array();
    $c=$a;
    while($c!=$b && $c) {
      $parents[]=$c;
      $c=strtolower(get_parent_class($c));
    }
    if ($c!=$b) return check::fail("Class $a does not inherit from class $b\nHierachy:\n  %s\n",join("\n  ",$parents));
    return TRUE;
  }

  function classparent($a,$b) {
    if (is_object($a)) $a=get_class($a);
    if (is_object($b)) $a=get_class($b);
    $parent=get_parent_class($a);

    if ($parent!=$b) return check::fail("Class $a parent not actually $b but $parent");
    return TRUE;
  }

  function functions($functions) {
    if (! is_array($functions)) $functions=array($functions);
    $message=array();
    $missing=array();
    $extra=array_flip(check::get_extra_functions());

    foreach ($functions as $func) {
      if (! function_exists($func)) $missing[]=$func;
      else unset($extra[$func]);
    }
    if ($missing) $message[]=sprintf("Functions missing: %s",join(",",$missing));
    if ($extra) $message[]=sprintf("These extra methods are defined: %s",join(",",array_keys($extra)));
    if ($message) return check::fail(join("\n  ",$message));
    return TRUE;    
  }

  function globals($globals) {
    if (! is_array($globals)) $globals=array($globals);
    $message=array();
    $missing=array();
    $extra=array_flip(check::get_extra_globals());

    foreach ($globals as $glob) {
      if (! isset($GLOBALS[$glob])) $missing[]=$glob;
      else unset($extra[$glob]);
    }
    if ($missing) $message[]=sprintf("Globals missing: %s",join(",",$missing));
    if ($extra) $message[]=sprintf("These extra globals are defined: %s",join(",",array_keys($extra)));
    if ($message) return check::fail(join("\n  ",$message));
    return TRUE;    

  }

  function functionref($a,$type,$message) {
    if (! eregi("^_[a-f0-9]+$type$",$a)) return check::fail($message);
    return TRUE;
  }

  function equal($a,$b,$message) {
    if (! ($a===$b)) return check::fail($message);
    return TRUE;
  }

  function resource($a,$b,$message) {
    $resource=check::var_dump($a);
    if (! eregi("^resource\([0-9]+\) of type \($b\)",$resource)) return check::fail($message);
    return TRUE;
  }

  function var_dump($arg) {
    ob_start();
    var_dump($arg);
    $result=ob_get_contents();
    ob_end_clean();
    return $result;
  }

  function fail($pattern) {
    $args=func_get_args();
    print("Failed on: ".call_user_func_array("sprintf",$args)."\n");
    exit(1);
  }

  function done() {
    print $_SERVER[argv][0]." ok\n";
  }
}
?>
