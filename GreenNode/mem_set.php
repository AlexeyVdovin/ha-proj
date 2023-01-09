<?php
$m = new Memcached();
$m->addServer('/run/memcached/memcached.socket', 0);

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
