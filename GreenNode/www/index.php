<?php
$m = new Memcached();
$m->addServer('/run/memcached/memcached.socket', 0);
header('Cache-Control: no-cache');
header('Cache-Control: max-age=0');
header('Cache-Control: no-store');

// 1W serial # for temperature sensors
$G1_ground['id'] = $m->get('G1_GROUND');
$G1_air['id']    = $m->get('G1_AIR');
$G2_ground['id'] = $m->get('G2_GROUND');
$G2_air['id']    = $m->get('G2_AIR');

// print_r($G1_ground);
// print_r($G1_air);
// print_r($G2_ground);
// print_r($G2_air);

if(empty($G1_ground['id']) || empty($G1_air['id']) || empty($G2_ground['id']) || empty($G2_air['id']))
{
    echo "Error: Temperature sensors are not defined.\n";
    echo "G1 Ground: ".$G1_ground['id']."\n";
    echo "G1 Air:".$G1_air['id']."\n";
    echo "G2 Ground: ".$G2_ground['id']."\n";
    echo "G2 Air: ".$G2_air['id']."\n";
    die;
}

function get_sensor($m, $id)
{
    $a = explode(' ', $id);
    $s['id'] = $a[0];
    $s['T'] = $m->get($a[0]);
    $s['max'] = $m->get('max_'.$a[0]);
    $s['maxT'] = $m->get('maxt_'.$a[0]);
    $s['min'] = $m->get('min_'.$a[0]);
    $s['minT'] = $m->get('mint_'.$a[0]);
    return $s;
}

$G1_ground = get_sensor($m, $G1_ground['id']);
$G2_ground = get_sensor($m, $G2_ground['id']);
$G1_air = get_sensor($m, $G1_air['id']);
$G2_air = get_sensor($m, $G2_air['id']);

$G1_VENT_C = $m->get('G1_VENT_C'); // ON, OFF, AUTO
$G1_HEAT_C = $m->get('G1_HEAT_C'); // ON, OFF, AUTO
$G1_CIRC_C = $m->get('G1_CIRC_C'); // ON, OFF, AUTO
$G1_WATER_C = $m->get('G1_WATER_C'); // ON, OFF, AUTO
$G1_W_PERIOD = $m->get('G1_PERIOD'); // 10 < X < 1440
$G1_W_DURATION = $m->get('G1_DURATION'); // 0 < X < 60

$G2_VENT_C = $m->get('G2_VENT_C'); // ON, OFF, AUTO
$G2_HEAT_C = $m->get('G2_HEAT_C'); // ON, OFF, AUTO
$G2_CIRC_C = $m->get('G2_CIRC_C'); // ON, OFF, AUTO
$G2_WATER_C = $m->get('G2_WATER_C'); // ON, OFF, AUTO
$G2_W_PERIOD = $m->get('G2_PERIOD'); // 10 < X < 1440
$G2_W_DURATION = $m->get('G2_DURATION'); // 0 < X < 60

if($G1_VENT_C === FALSE) $G1_VENT_C = 2;
if($G1_HEAT_C === FALSE) $G1_HEAT_C = 2;
if($G1_CIRC_C === FALSE) $G1_CIRC_C = 2;
if($G1_WATER_C === FALSE) $G1_WATER_C = 2;
if($G1_W_PERIOD === FALSE) $G1_W_PERIOD = 180;
else if($G1_W_PERIOD < 10) $G1_W_PERIOD = 10;
else if($G1_W_PERIOD > 1440) $G1_W_PERIOD = 1440;
if($G1_W_DURATION === FALSE) $G1_W_DURATION = 180;
else if($G1_W_DURATION < 0) $G1_W_DURATION = 0;
else if($G1_W_DURATION > 60) $G1_W_DURATION = 60;

if($G2_VENT_C === FALSE) $G2_VENT_C = 2;
if($G2_HEAT_C === FALSE) $G2_HEAT_C = 2;
if($G2_CIRC_C === FALSE) $G2_CIRC_C = 2;
if($G2_WATER_C === FALSE) $G2_WATER_C = 2;
if($G2_W_PERIOD === FALSE) $G2_W_PERIOD = 180;
else if($G2_W_PERIOD < 10) $G2_W_PERIOD = 10;
else if($G2_W_PERIOD > 1440) $G2_W_PERIOD = 1440;
if($G2_W_DURATION === FALSE) $G2_W_DURATION = 180;
else if($G2_W_DURATION < 0) $G2_W_DURATION = 0;
else if($G2_W_DURATION > 60) $G2_W_DURATION = 60;

$G1_heat = $m->get('G1_HEAT');
$G2_heat = $m->get('G2_HEAT');
$G1_circ = $m->get('G1_CIRC');
$G2_circ = $m->get('G2_CIRC');
$G1_vent = $m->get('G1_VENT');
$G2_vent = $m->get('G2_VENT');
$G1_water = $m->get('G1_WATER');
$G1_water_T = $m->get('G1_WATER_T');
$G2_water = $m->get('G2_WATER');
$G2_water_T = $m->get('G2_WATER_T');

function temp_c($t)
{
    return ((int)($t/100 + 0.5))/10;
}

function time_f($t)
{
    return date('H:i', $t);
}

$UPTIME = file_get_contents('/tmp/uptime.txt');
?>
<html>
<head></head>
<body>
    <h2>G1</h2><a href="https://thingspeak.com/apps/matlab_visualizations/328861?width=1100&height=300">График температуры</a>
<table border="1">
<tr><th width="25%"> <?php echo time_f(time()); ?> </th><th width="25%">Сейчас</th><th width="25%">Min</th><th width="25%">Max</th></tr>
<?php echo '<tr><td>Воздух</td>'
    .'<td><b> '.temp_c($G1_air['T']).' C </b><br> </td>'
    .'<td><b><font color="blue"> '.temp_c($G1_air['min']).' C </font></b><br> '.time_f($G1_air['minT']).' </td>'
    .'<td><b><font color="red"> '.temp_c($G1_air['max']).' C </font></b><br> '.time_f($G1_air['maxT']).' </td>'
    .'</tr>'; ?>
<?php echo '<tr><td>Земля</td>'
    .'<td><b> '.temp_c($G1_ground['T']).' C </b></td>'
    .'<td><b><font color="blue"> '.temp_c($G1_ground['min']).' C </font></b><br> '.time_f($G1_ground['minT']).' </td>'
    .'<td><b><font color="red"> '.temp_c($G1_ground['max']).' C </font></b><br> '.time_f($G1_ground['maxT']).' </td>'
    .'</tr>'; ?>
</table>
<form method="get" action="set.php">
    <input name="g" type="hidden" value="1">
    <table>
  <tr><th>Циркуляция:</th><?php echo '<td><b>'.($G1_circ ? 'ON' : 'OFF').'</b></td>'
    .'<td><input type="submit" name="circ" value="'.(!$G1_circ ? 'ON' : 'OFF').'"</td>'
    .($G1_CIRC_C != 2 ? '<td><input type="submit" name="circ" value="AUTO"</td>' : '<td><b><font color="green">AUTO</font></b></td>'); ?> </tr>
  <tr></tr>
  <tr><th>Вентиляция:</th><?php echo '<td><b>'.($G1_vent ? 'ON' : 'OFF').'</b></td>'
    .'<td><input type="submit" name="vent" value="'.(!$G1_vent ? 'ON' : 'OFF').'"</td>'
    .($G1_VENT_C != 2 ? '<td><input type="submit" name="vent" value="AUTO"</td>' : '<td><b><font color="green">AUTO</font></b></td>'); ?> </tr>
  <tr></tr>
  <tr><th>Подогрев:</th><?php echo '<td><b>'.($G1_heat ? 'ON' : 'OFF').'</b></td>'
    .'<td><input type="submit" name="heat" value="'.(!$G1_heat ? 'ON' : 'OFF').'"</td>'
    .($G1_HEAT_C != 2 ? '<td><input type="submit" name="heat" value="AUTO"</td>' : '<td><b><font color="green">AUTO</font></b></td>'); ?> </tr>
  <tr><th>Полив:</th><?php echo '<td><b>'.($G1_water ? 'ON' : 'OFF').'</b></td>'
    .'<td><input type="submit" name="water" value="'.(!$G1_water ? 'ON' : 'OFF').'"</td>'
    .($G1_WATER_C != 2 ? '<td><input type="submit" name="water" value="AUTO"</td>' : '<td><b><font color="green">AUTO</font></b></td>'); ?> </tr>
    </table>
</form>
<form method="get" action="set.php">
    <input name="g" type="hidden" value="1">
    <table>
  <tr><th>Полив был:</th><?php echo '<td><b>'.time_f($G1_water_T).'</b></td>'
    .'<td> </td>'
    .'<td> </td>'; ?> </tr>
  <tr><th>Периодичность:</th><?php echo '<td><input type="text" name="period" size="4" minlength="1" maxlength="5" value="'.($G1_W_PERIOD).'"></td>'
    .'<td>мин</td>'
    .'<td> </td>'; ?> </tr>
  <tr><th>Длительность:</th><?php echo '<td><input type="text" name="duration" size="4" minlength="1" maxlength="5" value="'.($G1_W_DURATION).'"></td>'
    .'<td>мин</td>'
    .'<td> </td>'; ?> </tr>
  <tr><th><input type="submit" name="save" value="Запомнить"></th><?php echo '<td> </td>'
    .'<td> </td>'
    .'<td> </td>'; ?> </tr>
    </table>
</form>

    <h2>G2</h2><a href="https://thingspeak.com/apps/matlab_visualizations/350886?width=1100&height=300">График температуры</a>
<table border="1">
<tr><th width="25%"> <?php echo time_f(time()); ?> </th><th width="25%">Сейчас</th><th width="25%">Min</th><th width="25%">Max</th></tr>
<?php echo '<tr><td>Воздух</td>'
    .'<td><b> '.temp_c($G2_air['T']).' C </b><br> </td>'
    .'<td><b><font color="blue"> '.temp_c($G2_air['min']).' C </font></b><br> '.time_f($G2_air['minT']).' </td>'
    .'<td><b><font color="red"> '.temp_c($G2_air['max']).' C </font></b><br> '.time_f($G2_air['maxT']).' </td>'
    .'</tr>'; ?>
<?php echo '<tr><td>Земля</td>'
    .'<td><b> '.temp_c($G2_ground['T']).' C </b></td>'
    .'<td><b><font color="blue"> '.temp_c($G2_ground['min']).' C </font></b><br> '.time_f($G2_ground['minT']).' </td>'
    .'<td><b><font color="red"> '.temp_c($G2_ground['max']).' C </font></b><br> '.time_f($G2_ground['maxT']).' </td>'
    .'</tr>'; ?>
</table>
<form method="get" action="set.php">
    <input name="g" type="hidden" value="2">
    <table>
  <tr><th>Циркуляция:</th><?php echo '<td><b>'.($G2_circ ? 'ON' : 'OFF').'</b></td>'
    .'<td><input type="submit" name="circ" value="'.(!$G2_circ ? 'ON' : 'OFF').'"</td>'
    .($G2_CIRC_C != 2 ? '<td><input type="submit" name="circ" value="AUTO"</td>' : '<td><b><font color="green">AUTO</font></b></td>'); ?> </tr>
  <tr></tr>
  <tr><th>Вентиляция:</th><?php echo '<td><b>'.($G2_vent ? 'ON' : 'OFF').'</b></td>'
    .'<td><input type="submit" name="vent" value="'.(!$G2_vent ? 'ON' : 'OFF').'"</td>'
    .($G2_VENT_C != 2 ? '<td><input type="submit" name="vent" value="AUTO"</td>' : '<td><b><font color="green">AUTO</font></b></td>'); ?> </tr>
  <tr></tr>
  <tr><th>Подогрев:</th><?php echo '<td><b>'.($G2_heat ? 'ON' : 'OFF').'</b></td>'
    .'<td><input type="submit" name="heat" value="'.(!$G2_heat ? 'ON' : 'OFF').'"</td>'
    .($G2_HEAT_C != 2 ? '<td><input type="submit" name="heat" value="AUTO"</td>' : '<td><b><font color="green">AUTO</font></b></td>'); ?> </tr>
  <tr><th>Полив:</th><?php echo '<td><b>'.($G2_water ? 'ON' : 'OFF').'</b></td>'
    .'<td><input type="submit" name="water" value="'.(!$G2_water ? 'ON' : 'OFF').'"</td>'
    .($G2_WATER_C != 2 ? '<td><input type="submit" name="water" value="AUTO"</td>' : '<td><b><font color="green">AUTO</font></b></td>'); ?> </tr>
    </table>
</form>
<form method="get" action="set.php">
    <input name="g" type="hidden" value="2">
    <table>
  <tr><th>Полив был:</th><?php echo '<td><b>'.time_f($G2_water_T).'</b></td>'
    .'<td> </td>'
    .'<td> </td>'; ?> </tr>
  <tr><th>Периодичность:</th><?php echo '<td><input type="text" name="period" size="4" minlength="1" maxlength="5" value="'.($G2_W_PERIOD).'"></td>'
    .'<td>мин</td>'
    .'<td> </td>'; ?> </tr>
  <tr><th>Длительность:</th><?php echo '<td><input type="text" name="duration" size="4" minlength="1" maxlength="5" value="'.($G2_W_DURATION).'"></td>'
    .'<td>мин</td>'
    .'<td> </td>'; ?> </tr>
  <tr><th><input type="submit" name="save" value="Запомнить"></th><?php echo '<td> </td>'
    .'<td> </td>'
    .'<td> </td>'; ?> </tr>
    </table>
</form>
<p><?php echo $UPTIME; ?></p>
</body>
</html>
