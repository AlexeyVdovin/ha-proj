<?php
$DEBUG = 1;

// 1W seril # for temperature sensors
$G1_ground['id'] = $m->get('G1_GROUND'); // '0x28e81d7791170295';
$G1_air['id']    = $m->get('G1_AIR'); // '0x28a51b779113020d';
$G2_ground['id'] = $m->get('G2_GROUND'); // '0x28e00646920402a2';
$G2_air['id']    = $m->get('G2_AIR'); // '0x2821cc46920302fb';

if(!isset($G1_ground['id']) || !isset($G1_air['id']) || !isset($G2_ground['id']) || !isset($G2_air['id']))
{
    echo "Error: Temperature sensors are not defined."
    exit(1);
}

$m = new Memcached();
$m->addServer('localhost', 11211);

$G1_FREEZ = $m->get('G1_FREEZ'); // 1, 0
$G1_COOL = $m->get('G1_COOL');   // ON, OFF, AUTO
$G1_HEAT = $m->get('G1_HEAT');   // ON, OFF, AUTO

$G2_FREEZ = $m->get('G2_FREEZ'); // 1, 0
$G2_COOL = $m->get('G2_COOL');   // ON, OFF, AUTO
$G2_HEAT = $m->get('G2_HEAT');   // ON, OFF, AUTO

if(!isset($G1_FREEZ)) $G1_FREEZ = 0;
if(!isset($G1_COOL)) $G1_COOL = 'AUTO';
if(!isset($G1_HEAT)) $G1_HEAT = 'AUTO';
if(!isset($G2_FREEZ)) $G2_FREEZ = 0;
if(!isset($G2_COOL)) $G2_COOL = 'AUTO';
if(!isset($G2_HEAT)) $G2_HEAT = 'AUTO';

$avg = null;
$v = $m->get($G1_ground['id']);
if(isset($v))
{
    $a = explode(',', $v);
    if(count($a) > 0) $avg = array_sum($a)/count($a);
}
$G1_ground['avg'] = $avg;
if($DEBUG) echo "G1 Ground avg: $avg\n";

$avg = null;
$v = $m->get($G1_air['id']);
if(isset($v))
{
    $a = explode(',', $v);
    if(count($a) > 0) $avg = array_sum($a)/count($a);
}
$G1_air['avg'] = $avg; 
if($DEBUG) echo "G1 Air avg: $avg\n";

$avg = null;
$v = $m->get($G2_ground['id']);
if(isset($v))
{
    $a = explode(',', $v);
    if(count($a) > 0) $avg = array_sum($a)/count($a);
}
$G2_ground['avg'] = $avg; 
if($DEBUG) echo "G2 Ground avg: $avg\n";

$avg = null;
$v = $m->get($G2_air['id']);
if(isset($v))
{
    $a = explode(',', $v);
    if(count($a) > 0) $avg = array_sum($a)/count($a);
}
$G2_air['avg'] = $avg; 
if($DEBUG) echo "G2 Air avg: $avg\n";

// Business logic
$G1_heater = $m->get('G1_HEATER');
if($G1_HEAT == 'AUTO')
{
    if($G1_heater == 0)
    {
        if($G1_air['avg'] > $G1_ground['avg'] + 5000) $G1_heater = 1;
        if($G1_FREEZ == 1 && $G1_air['avg'] < 5000 && $G1_ground['avg'] > 12000) $G1_heater = 1;
    }
    else
    {
        if($G1_air['avg'] < $G1_ground['avg'] + 2000) $G1_heater = 0;
        if($G1_FREEZ == 1 && ($G1_air['avg'] > 7000 || $G1_ground['avg'] < 10000)) $G1_heater = 0;
    }    
}
else
{
    $G1_heater = ($G1_HEAT == 'ON') ? 1 : 0;
}
$m->set('G1_HEATER', $G1_heater);
if($DEBUG) echo "G1 Heater: ".($G1_heater == 1 ? 'ON' : 'OFF')."\n";


$G2_heater = $m->get('G2_HEATER');
if($G2_HEAT == 'AUTO')
{
    if($G2_heater == 0)
    {
        if($G2_air['avg'] > $G2_ground['avg'] + 5000) $G2_heater = 1;
        if($G2_FREEZ == 1 && $G2_air['avg'] < 5000 && $G2_ground['avg'] > 12000) $G2_heater = 1;
    }
    else
    {
        if($G2_air['avg'] < $G2_ground['avg'] + 2000) $G2_heater = 0;
        if($G2_FREEZ == 1 && ($G2_air['avg'] > 7000 || $G2_ground['avg'] < 10000)) $G2_heater = 0;
    }    
}
else
{
    $G2_heater = ($G2_HEAT == 'ON') ? 1 : 0;
}
$m->set('G2_HEATER', $G2_heater);
if($DEBUG) echo "G2 Heater: ".($G2_heater == 1 ? 'ON' : 'OFF')."\n";

$G1_vent = $m->get('G1_VENT');
if($G1_COOL == 'AUTO')
{
    if($G1_vent == 0)
    {
        if($G1_air['avg'] > 32000) $G1_vent = 1;
    }
    else
    {
        if($G1_air['avg'] < 30000) $G1_vent = 0;
    }
}
else
{
    $G1_vent = ($G1_COOL == 'ON') ? 1 : 0;
}
$m->set('G1_VENT', $G1_vent);
if($DEBUG) echo "G1 Vent: ".($G1_vent == 1 ? 'ON' : 'OFF')."\n";

$G2_vent = $m->get('G2_VENT');
if($G2_COOL == 'AUTO')
{
    if($G2_vent == 0)
    {
        if($G2_air['avg'] > 32000) $G2_vent = 1;
    }
    else
    {
        if($G2_air['avg'] < 30000) $G2_vent = 0;
    }
}
else
{
    $G2_vent = ($G2_COOL == 'ON') ? 1 : 0;
}
$m->set('G2_VENT', $G2_vent);
if($DEBUG) echo "G2 Vent: ".($G2_vent == 1 ? 'ON' : 'OFF')."\n";

exit(0);
?>
