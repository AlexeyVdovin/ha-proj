<?php
$DEBUG = 1;

$m = new Memcached();
$m->addServer('localhost', 11211);

// 1W serial # for temperature sensors
$G1_ground['id']   = $m->get('G1_GROUND');
$G1_ground['corr'] = explode(',', $m->get('G1_GROUND_CORR'));
$G1_air['id']      = $m->get('G1_AIR');
$G1_air['corr']    = explode(',', $m->get('G1_AIR_CORR'));
$G2_ground['id']   = $m->get('G2_GROUND');
$G2_ground['corr'] = explode(',', $m->get('G2_GROUND_CORR'));
$G2_air['id']      = $m->get('G2_AIR');
$G2_air['corr']    = explode(',', $m->get('G2_AIR_CORR'));

if(empty($G1_ground['id']) || empty($G1_air['id']) || empty($G2_ground['id']) || empty($G2_air['id']))
{
    echo "Error: Temperature sensors are not defined.\n";
    exit(1);
}

$G1_FREEZ = $m->get('G1_FREEZ'); // 1, 0
$G1_VENT_C = $m->get('G1_VENT_C');   // ON, OFF, AUTO
$G1_HEAT_C = $m->get('G1_HEAT_C');   // ON, OFF, AUTO
$G1_CIRC_C = $m->get('G1_CIRC_C');   // ON, OFF, AUTO

$G2_FREEZ = $m->get('G2_FREEZ'); // 1, 0
$G2_VENT_C = $m->get('G2_VENT_C');   // ON, OFF, AUTO
$G2_HEAT_C = $m->get('G2_HEAT_C');   // ON, OFF, AUTO
$G2_CIRC_C = $m->get('G2_CIRC_C');   // ON, OFF, AUTO

if($G1_FREEZ === FALSE) $G1_FREEZ = 0;
if($G1_VENT_C === FALSE) $G1_VENT_C = 'AUTO';
if($G1_HEAT_C === FALSE) $G1_HEAT_C = 'AUTO';
if($G1_CIRC_C === FALSE) $G1_CIRC_C = 'AUTO';

if($G2_FREEZ === FALSE) $G2_FREEZ = 0;
if($G2_VENT_C === FALSE) $G2_VENT_C = 'AUTO';
if($G2_HEAT_C === FALSE) $G2_HEAT_C = 'AUTO';
if($G2_CIRC_C === FALSE) $G2_CIRC_C = 'AUTO';

$c = $G1_ground['corr'];
$avg = (int)(($m->get($G1_ground['id']) + $c[0])*$c[1]);
$v = $m->get('avg_'.$G1_ground['id']);
if($v != FALSE)
{
    $a = explode(',', $v);
    if(count($a) > 0) $avg = (int)((array_sum($a)/count($a) + $c[0])*$c[1]);
}
$G1_ground['avg'] = $avg;
if($DEBUG) echo "G1 Ground avg: $avg\n";

$c = $G1_air['corr'];
$avg = $m->get($G1_air['id']) + $c[0])*$c[1]);
$v = $m->get('avg_'.$G1_air['id']);
if($v != FALSE)
{
    $a = explode(',', $v);
    if(count($a) > 0) $avg = (int)((array_sum($a)/count($a) + $c[0])*$c[1]);
}
$G1_air['avg'] = $avg; 
if($DEBUG) echo "G1 Air avg: $avg\n";

$c = $G2_ground['corr'];
$avg = $m->get($G2_ground['id']) + $c[0])*$c[1]);
$v = $m->get('avg_'.$G2_ground['id']);
if($v != FALSE)
{
    $a = explode(',', $v);
    if(count($a) > 0) $avg = (int)((array_sum($a)/count($a) + $c[0])*$c[1]);
}
$G2_ground['avg'] = $avg; 
if($DEBUG) echo "G2 Ground avg: $avg\n";

$c = $G2_air['corr'];
$avg = $m->get($G2_air['id']) + $c[0])*$c[1]);
$v = $m->get('avg_'.$G2_air['id']);
if($v != FALSE)
{
    $a = explode(',', $v);
    if(count($a) > 0) $avg = (int)((array_sum($a)/count($a) + $c[0])*$c[1]);
}
$G2_air['avg'] = $avg; 
if($DEBUG) echo "G2 Air avg: $avg\n";

// Business logic
$G1_circ = $m->get('G1_CIRC');
if($G1_circ === FALSE) $G1_circ = 0;
if($G1_CIRC_C == 'AUTO')
{
    if($G1_circ == 0)
    {
        if($G1_air['avg'] > $G1_ground['avg'] + 6000 && $G1_ground['avg'] < 25000) $G1_circ = 1;
        if($G1_FREEZ == 1 && $G1_air['avg'] < 5000 && $G1_ground['avg'] > 12000) $G1_circ = 1;
    }
    else
    {
        if($G1_air['avg'] < $G1_ground['avg'] + 2000 || $G1_ground['avg'] > 28000) $G1_circ = 0;
        if($G1_FREEZ == 1 && ($G1_air['avg'] > 7000 || $G1_ground['avg'] < 10000)) $G1_circ = 0;
    }    
}
else
{
    $G1_circ = ($G1_CIRC_C == 'ON') ? 1 : 0;
}
$m->set('G1_CIRC', $G1_circ);
if($DEBUG) echo "G1 Circulation: ".($G1_circ == 1 ? 'ON' : 'OFF')."\n";

$G1_heat = $m->get('G1_HEAT');
if($G1_heat === FALSE) $G1_heat = 0;
if($G1_HEAT_C == 'AUTO')
{
    if($G1_heat == 0)
    {
        if($G1_air['avg'] < 14000) $G1_heat = 1;
    }
    else
    {
        if($G1_air['avg'] > 15000) $G1_heat = 0;
    }    
}
else
{
    $G1_heat = ($G1_HEAT_C == 'ON') ? 1 : 0;
}
$m->set('G1_HEAT', $G1_heat);
if($DEBUG) echo "G1 Heater: ".($G1_heat == 1 ? 'ON' : 'OFF')."\n";


$G2_circ = $m->get('G2_CIRC');
if($G2_circ === FALSE) $G2_circ = 0;
if($G2_CIRC_C == 'AUTO')
{
    if($G2_circ == 0)
    {
        if($G2_air['avg'] > $G2_ground['avg'] + 6000 && $G2_ground['avg'] < 18000) $G2_circ = 1;
        if($G2_FREEZ == 1 && $G2_air['avg'] < 5000 && $G2_ground['avg'] > 12000) $G2_circ = 1;
    }
    else
    {
        if($G2_air['avg'] < $G2_ground['avg'] + 2000 || $G2_ground['avg'] > 21000) $G2_circ = 0;
        if($G2_FREEZ == 1 && ($G2_air['avg'] > 7000 || $G2_ground['avg'] < 10000)) $G2_circ = 0;
    }    
}
else
{
    $G2_circ = ($G2_CIRC_C == 'ON') ? 1 : 0;
}
$m->set('G2_CIRC', $G2_circ);
if($DEBUG) echo "G2 Circulation: ".($G2_circ == 1 ? 'ON' : 'OFF')."\n";

$G2_heat = $m->get('G2_HEAT');
if($G2_heat === FALSE) $G2_heat = 0;
if($G2_HEAT_C == 'AUTO')
{
    if($G2_heat == 0)
    {
        if($G2_air['avg'] < 14000) $G2_heat = 1;
    }
    else
    {
        if($G2_air['avg'] > 15000) $G2_heat = 0;
    }    
}
else
{
    $G2_heat = ($G2_HEAT_C == 'ON') ? 1 : 0;
}
$m->set('G2_HEAT', $G2_heat);
if($DEBUG) echo "G2 Heater: ".($G2_heat == 1 ? 'ON' : 'OFF')."\n";

$G1_vent = $m->get('G1_VENT');
if($G1_vent === FALSE) $G1_vent = 0;
if($G1_VENT_C == 'AUTO')
{
    if($G1_vent == 0)
    {
        if($G1_air['avg'] > 28000) $G1_vent = 1;
    }
    else
    {
        if($G1_air['avg'] < 25000) $G1_vent = 0;
    }
}
else
{
    $G1_vent = ($G1_VENT_C == 'ON') ? 1 : 0;
}
$m->set('G1_VENT', $G1_vent);
if($DEBUG) echo "G1 Vent: ".($G1_vent == 1 ? 'ON' : 'OFF')."\n";

$G2_vent = $m->get('G2_VENT');
if($G2_vent === FALSE) $G2_vent = 0;
if($G2_VENT_C == 'AUTO')
{
    if($G2_vent == 0)
    {
        if($G2_air['avg'] > 28000) $G2_vent = 1;
    }
    else
    {
        if($G2_air['avg'] < 25000) $G2_vent = 0;
    }
}
else
{
    $G2_vent = ($G2_VENT_C == 'ON') ? 1 : 0;
}
$m->set('G2_VENT', $G2_vent);
if($DEBUG) echo "G2 Vent: ".($G2_vent == 1 ? 'ON' : 'OFF')."\n";

exit(0);
?>
