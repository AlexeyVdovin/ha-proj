<?php
$m = new Memcached();
$m->addServer('localhost', 11211);
header('Cache-Control: no-cache');
header('Cache-Control: max-age=0');
header('Cache-Control: no-store');

function get_corr($m, $n)
{
    $v = $m->get($n.'_CORR');
    if($v === false) $v = "0,1.0";
    $c = explode(',', $v);
    return $c;
}

// 1W seril # for temperature sensors
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
    die;
}

function get_sensor($m, $id, $c)
{
    $s['id'] = $id;
    $s['T'] = (int)(($m->get($id)+$c[0])*$c[1]);
    $s['TT'] = $m->get('t_'.$id);
    $s['max'] = (int)(($m->get('max_'.$id)+$c[0])*$c[1]);
    $s['maxT'] = $m->get('maxt_'.$id);
    $s['min'] = (int)(($m->get('min_'.$id)+$c[0])*$c[1]);
    $s['minT'] = $m->get('mint_'.$id);
    $avg = $s['T'];
    $v = $m->get('avg_'.$id);
    if($v != FALSE)
    {
        $a = explode(',', $v);
        if(count($a) > 0) $avg = (int)((array_sum($a)/count($a) + $c[0])*$c[1]);
    }
    $s['avg'] = $avg;
    /*
    echo "<!---\n";
    echo "c=".$c[0].", ".$c[1]."\n";
    print_r($s);
    echo "--->\n";
    */
    return $s;    
}

$G1_ground = get_sensor($m, $G1_ground['id'], $G1_ground['corr']);
$G2_ground = get_sensor($m, $G2_ground['id'], $G2_ground['corr']);
$G1_air = get_sensor($m, $G1_air['id'], $G1_air['corr']);
$G2_air = get_sensor($m, $G2_air['id'], $G2_air['corr']);

$G1_FREEZ = $m->get('G1_FREEZ');   // 1, 0
$G1_VENT_C = $m->get('G1_VENT_C'); // ON, OFF, AUTO
$G1_HEAT_C = $m->get('G1_HEAT_C'); // ON, OFF, AUTO
$G1_CIRC_C = $m->get('G1_CIRC_C'); // ON, OFF, AUTO

$G2_FREEZ = $m->get('G2_FREEZ');   // 1, 0
$G2_VENT_C = $m->get('G2_VENT_C'); // ON, OFF, AUTO
$G2_HEAT_C = $m->get('G2_HEAT_C'); // ON, OFF, AUTO
$G2_CIRC_C = $m->get('G2_CIRC_C'); // ON, OFF, AUTO

if($G1_FREEZ === FALSE) $G1_FREEZ = 0;
if($G1_VENT_C === FALSE) $G1_VENT_C = 'AUTO';
if($G1_HEAT_C === FALSE) $G1_HEAT_C = 'AUTO';
if($G1_CIRC_C === FALSE) $G1_CIRC_C = 'AUTO';
if($G2_FREEZ === FALSE) $G2_FREEZ = 0;
if($G2_VENT_C === FALSE) $G2_VENT_C = 'AUTO';
if($G2_HEAT_C === FALSE) $G2_HEAT_C = 'AUTO';
if($G2_CIRC_C === FALSE) $G2_CIRC_C = 'AUTO';

$G1_heat = $m->get('G1_HEAT');
$G2_heat = $m->get('G2_HEAT');
$G1_circ = $m->get('G1_CIRC');
$G2_circ = $m->get('G2_CIRC');
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
<tr><th width="25%"> <?php echo time_l(time()); ?> </th><th width="25%">Сейчас</th><th width="25%">Min</th><th width="25%">Max</th></tr>
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
<form method="get" action="set.php">
    <input name="g" type="hidden" value="1">
    <table>
  <tr><th>Циркуляция:</th><?php echo '<td><b>'.($G1_circ ? 'ON' : 'OFF').'</b></td>'
    .'<td><input type="submit" name="circ" value="'.(!$G1_circ ? 'ON' : 'OFF').'"</td>'
    .($G1_CIRC_C != 'AUTO' ? '<td><input type="submit" name="circ" value="AUTO"</td>' : '<td><b><font color="green">AUTO</font></b></td>'); ?> </tr>
  <tr></tr>      
  <tr><th>Вентиляция:</th><?php echo '<td><b>'.($G1_vent ? 'ON' : 'OFF').'</b></td>'
    .'<td><input type="submit" name="vent" value="'.(!$G1_vent ? 'ON' : 'OFF').'"</td>'
    .($G1_VENT_C != 'AUTO' ? '<td><input type="submit" name="vent" value="AUTO"</td>' : '<td><b><font color="green">AUTO</font></b></td>'); ?> </tr>
  <tr></tr>
  <tr><th>Подогрев:</th><?php echo '<td><b>'.($G1_heat ? 'ON' : 'OFF').'</b></td>'
    .'<td><input type="submit" name="heat" value="'.(!$G1_heat ? 'ON' : 'OFF').'"</td>'
    .($G1_HEAT_C != 'AUTO' ? '<td><input type="submit" name="heat" value="AUTO"</td>' : '<td><b><font color="green">AUTO</font></b></td>'); ?> </tr>
    </table>     
</form>

    <h2>G2</h2>
<table border="1">
<tr><th width="25%"> <?php echo time_l(time()); ?> </th><th width="25%">Сейчас</th><th width="25%">Min</th><th width="25%">Max</th></tr>
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
<form method="get" action="set.php">
    <input name="g" type="hidden" value="2">
    <table>
  <tr><th>Циркуляция:</th><?php echo '<td><b>'.($G2_circ ? 'ON' : 'OFF').'</b></td>'
    .'<td><input type="submit" name="circ" value="'.(!$G2_circ ? 'ON' : 'OFF').'"</td>'
    .($G2_CIRC_C != 'AUTO' ? '<td><input type="submit" name="circ" value="AUTO"</td>' : '<td><b><font color="green">AUTO</font></b></td>'); ?> </tr>
  <tr></tr>      
  <tr><th>Вентиляция:</th><?php echo '<td><b>'.($G2_vent ? 'ON' : 'OFF').'</b></td>'
    .'<td><input type="submit" name="vent" value="'.(!$G2_vent ? 'ON' : 'OFF').'"</td>'
    .($G2_VENT_C != 'AUTO' ? '<td><input type="submit" name="vent" value="AUTO"</td>' : '<td><b><font color="green">AUTO</font></b></td>'); ?> </tr>
  <tr></tr>
  <tr><th>Подогрев:</th><?php echo '<td><b>'.($G2_heat ? 'ON' : 'OFF').'</b></td>'
    .'<td><input type="submit" name="heat" value="'.(!$G2_heat ? 'ON' : 'OFF').'"</td>'
    .($G2_HEAT_C != 'AUTO' ? '<td><input type="submit" name="heat" value="AUTO"</td>' : '<td><b><font color="green">AUTO</font></b></td>'); ?> </tr>
    </table>     
</form>

</body>
</html>