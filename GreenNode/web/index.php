<?php
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
    die;
}

function get_sensor($m, $id)
{
    $s['id'] = $id;
    $s['T'] = $m->get($id);
    $s['TT'] = $m->get('t_'.$id);
    $s['max'] = $m->get('max_'.$id);
    $s['maxT'] = $m->get('maxt_'.$id);
    $s['min'] = $m->get('min_'.$id);
    $s['minT'] = $m->get('mint_'.$id);
    $avg = $s['T'];
    $v = $m->get('avg_'.$id);
    if(!empty($v))
    {
        $a = explode(',', $v);
        if(count($a) > 0) $avg = (int)(array_sum($a)/count($a));
    }
    $s['avg'] = $avg;
    return $s;    
}

$G1_ground = get_sensor($m, $G1_ground['id']);
$G2_ground = get_sensor($m, $G2_ground['id']);
$G1_air = get_sensor($m, $G1_air['id']);
$G2_air = get_sensor($m, $G2_air['id']);

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

$G1_heater = $m->get('G1_HEATER');
$G2_heater = $m->get('G2_HEATER');
$G1_circ = $m->get('G1_CIRC');
$G2_circ = $m->get('G1_CIRC');
$G1_vent = $m->get('G1_VENT');
$G2_vent = $m->get('G2_VENT');

function temp_c($t)
{
    return ((int)($t/100 + 0.5))/10;    
}

function time_l($t)
{
    return strftime('%R', $t + 3 * 3600);
}
?>
<html>
<head></head>
<body>
    <h2>G1</h2>
<table border="1">
<tr><th width="25%"> </th><th width="25%">Сейчас</th><th width="25%">Min</th><th width="25%">Max</th></tr>
<?php echo '<tr><td>Воздух</td>'
    .'<td><b> '.temp_c($G1_air['T']).' C </b><br> </td>'
    .'<td><b><font color="blue"> '.temp_c($G1_air['min']).' C </font></b><br> '.time_l($G1_air['minT']).' </td>'
    .'<td><b><font color="red"> '.temp_c($G1_air['max']).' C </font></b><br> '.time_l($G1_air['maxT']).' </td>'
    .'</tr>'; ?>
<?php echo '<tr><td>Земля </td>'
    .'<td><b> '.temp_c($G1_ground['T']).' C </b></td>'
    .'<td><b><font color="blue"> '.temp_c($G1_ground['min']).' C </font></b><br> '.time_l($G1_ground['minT']).' </td>'
    .'<td><b><font color="red"> '.temp_c($G1_ground['max']).' C </font></b><br> '.time_l($G1_ground['maxT']).' </td>'
    .'</tr>'; ?>
</table>
<form action="/set.php">
    <table>
  <tr><th>Циркуляция:</th><?php echo '<td><b>'.($G1_circ ? 'ON' : 'OFF').'</b></td>'
    .'<td><button type="button">'.(!$G1_circ ? 'ON' : 'OFF').'</button></td>'
    .'<td><button type="button">AUTO</button></td>'; ?> </tr>
  <tr></tr>      
  <tr><th>Вентиляция:</th><?php echo '<td><b>'.($G1_vent ? 'ON' : 'OFF').'</b></td>'
    .'<td><button type="button">'.(!$G1_vent ? 'ON' : 'OFF').'</button></td>'
    .'<td><button type="button">AUTO</button></td>'; ?> </tr>
  <tr></tr>
  <tr><th>Подогрев:</th><?php echo '<td><b>'.($G1_heater ? 'ON' : 'OFF').'</b></td>'
    .'<td><button type="button">'.(!$G1_heater ? 'ON' : 'OFF').'</button></td>'
    .'<td><button type="button">AUTO</button></td>'; ?></tr>
    </table>     
</form>

    <h2>G2</h2>
<table border="1">
<tr><th width="25%"> </th><th width="25%">Сейчас</th><th width="25%">Min</th><th width="25%">Max</th></tr>
<?php echo '<tr><td>Воздух</td>'
    .'<td><b> '.temp_c($G2_air['T']).' C </b><br> </td>'
    .'<td><b><font color="blue"> '.temp_c($G2_air['min']).' C </font></b><br> '.time_l($G2_air['minT']).' </td>'
    .'<td><b><font color="red"> '.temp_c($G2_air['max']).' C </font></b><br> '.time_l($G2_air['maxT']).' </td>'
    .'</tr>'; ?>
<?php echo '<tr><td>Земля </td>'
    .'<td><b> '.temp_c($G2_ground['T']).' C </b></td>'
    .'<td><b><font color="blue"> '.temp_c($G2_ground['min']).' C </font></b><br> '.time_l($G2_ground['minT']).' </td>'
    .'<td><b><font color="red"> '.temp_c($G2_ground['max']).' C </font></b><br> '.time_l($G2_ground['maxT']).' </td>'
    .'</tr>'; ?>
</table>
<form action="/set.php">
    <table>
  <tr><th>Циркуляция:</th><?php echo '<td><b>'.($G2_circ ? 'ON' : 'OFF').'</b></td>'
    .'<td><button type="button">'.(!$G2_circ ? 'ON' : 'OFF').'</button></td>'
    .'<td><button type="button">AUTO</button></td>'; ?> </tr>
  <tr></tr>      
  <tr><th>Вентиляция:</th><?php echo '<td><b>'.($G2_vent ? 'ON' : 'OFF').'</b></td>'
    .'<td><button type="button">'.(!$G2_vent ? 'ON' : 'OFF').'</button></td>'
    .'<td><button type="button">AUTO</button></td>'; ?> </tr>
  <tr></tr>
  <tr><th>Подогрев:</th><?php echo '<td><b>'.($G2_heater ? 'ON' : 'OFF').'</b></td>'
    .'<td><button type="button">'.(!$G2_heater ? 'ON' : 'OFF').'</button></td>'
    .'<td><button type="button">AUTO</button></td>'; ?></tr>
    </table>     
</form>

</body>
</html>