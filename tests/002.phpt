--TEST--
pdo_taosw_test1() Basic test
--SKIPIF--
<?php
if (!extension_loaded('pdo_taosw')) {
	echo 'skip';
}
?>
--FILE--
<?php
$ret = pdo_taosw_test1();

var_dump($ret);
?>
--EXPECT--
The extension pdo_taosw is loaded and working!
NULL
