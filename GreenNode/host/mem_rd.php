<?php
$m = new Memcached();
$m->addServer('localhost', 11211);

$val = $m->get('A');

echo $val."\n";
?>
