<?php
$m = new Memcached();
$m->addServer('localhost', 11211);

$n = trim($argv[1]);
$v = trim($argv[2]);

if(!isset($n))
{
    echo "Error: Invalid arguments\n";
    exit(1);
}

$val = $m->get($n);

if(empty($val)) $val = $v;

echo $val."\n";
exit(0);
?>
