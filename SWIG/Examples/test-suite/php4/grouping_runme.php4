<?php
// Sample test file

require "tests.php4";
require "grouping.php";

check::functions(array("test1","test2","do_unary","negate","test3_set","test3_get"));
check::equal(5,test1(5),"5==test1(5)");
check::resource(test2(7),"_p_int","_p_int==test2(7)");
check::globals(array("GROUPING_LOADED__"));
#check::equal(36,$test3,'36==$test3');
check::equal(37,test3_get(),'37==test3_get()');
test3_set(38);
check::equal(38,test3_get(),'38==test3_get() after test3_set(37)');

check::equal(-5,negate(5),"-5==negate(5)");
check::functionref(NEGATE,"_p_UnaryOp","NEGATE=_p_UnaryOp");
check::equal(7,do_unary(-7,NEGATE),"7=do_unary(-7,NEGATE)");

check::done();
?>
