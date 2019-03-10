<?php
$m = new Memcached();
$m->addServer('localhost', 11211);

$G1_FREEZ = $m->get('G1_FREEZ'); // 1, 0
$G1_COOL = $m->get('G1_COOL');   // ON, OFF, AUTO
$G1_HEAT = $m->get('G1_HEAT');   // ON, OFF, AUTO

$G2_FREEZ = $m->get('G2_FREEZ'); // 1, 0
$G2_COOL = $m->get('G2_COOL');   // ON, OFF, AUTO
$G2_HEAT = $m->get('G2_HEAT');   // ON, OFF, AUTO

if(empty($G1_FREEZ)) $G1_FREEZ = 0;
if(empty($G1_COOL)) $G1_COOL = 'AUTO';
if(empty($G1_HEAT)) $G1_HEAT = 'AUTO';
if(empty($G2_FREEZ)) $G2_FREEZ = 0;
if(empty($G2_COOL)) $G2_COOL = 'AUTO';
if(empty($G2_HEAT)) $G2_HEAT = 'AUTO';



echo 'OK';
?>
