<?php
$m = new Memcached();
$m->addServer('localhost', 11211);

$n = trim($argv[1]);
$v = trim($argv[2]);

if(!isset($n) || (!isset($v)))
{
    echo "Error: Invalid arguments\n";
    exit(1);
}

$m->set($n, $v);
exit(0);
?>
