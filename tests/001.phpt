--TEST--
Check if pdo_taosw is loaded
--SKIPIF--
<?php
if (!extension_loaded('pdo_taosw')) {
	echo 'skip';
}
?>
--FILE--
<?php
echo 'The extension "pdo_taosw" is available';
?>
--EXPECT--
The extension "pdo_taosw" is available
