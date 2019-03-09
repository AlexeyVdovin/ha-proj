<?php
$m = new Memcached();
$m->addServer('localhost', 11211);

$m->set('A', 99);

?>
