<?php
$m = new Memcached();
$m->addServer('localhost', 11211);

$n = trim($argv[1]);
$v = trim($argv[2]);
$t = time();

if(empty($v) || (substr($n, 0, 2) != '0x'))
{
    echo "Error: Invalid arguments\n";
    exit(1);
}

$m->set($n, $v);
$m->set('t_'.$n, $t);

$min = $m->get('min_'.$n);
if(($min === FALSE) || ($v < $min))
{
    $m->set('min_'.$n, $v);
    $m->set('mint_'.$n, $t);
}

$max = $m->get('max_'.$n);
if(($max === FALSE) || ($v > $max))
{
    $m->set('max_'.$n, $v);
    $m->set('maxt_'.$n, $t);
}

$avg = $m->get('avg_'.$n);
$a = explode(',', $avg);
if(count($a) > 4) array_shift($a);
array_push($a, $v);
$m->set('avg_'.$n, implode(',', $a));

exit(0);
?>
