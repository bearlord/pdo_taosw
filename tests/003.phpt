--TEST--
pdo_taosw_test2() Basic test
--SKIPIF--
<?php
if (!extension_loaded('pdo_taosw')) {
	echo 'skip';
}
?>
--FILE--
<?php
var_dump(pdo_taosw_test2());
var_dump(pdo_taosw_test2('PHP'));
?>
--EXPECT--
string(11) "Hello World"
string(9) "Hello PHP"
