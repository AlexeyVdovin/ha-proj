<?php
$DEBUG = 1;

$m = new Memcached();
$m->addServer('localhost', 11211);

// 1W seril # for temperature sensors
$G1_ground['id'] = $m->get('G1_GROUND');
$G1_air['id']    = $m->get('G1_AIR');
$G2_ground['id'] = $m->get('G2_GROUND');
$G2_air['id']    = $m->get('G2_AIR');

if(empty($G1_ground['id']) || empty($G1_air['id']) || empty($G2_ground['id']) || empty($G2_air['id']))
{
    echo "Error: Temperature sensors are not defined.\n";
    exit(1);
}

function set_max($m, $id)
{
    $v = $m->get($id);
    $t = $m->get('t_'.$id);
    $m->set('max_'.$id, $v);
    $m->set('maxt_'.$id, $t);
}

set_max($m, $G1_ground['id']);
set_max($m, $G1_air['id']);
set_max($m, $G2_ground['id']);
set_max($m, $G2_air['id']);

exit(0);
?>
