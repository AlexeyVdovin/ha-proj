<?php
$m = new Memcached();
$m->addServer('localhost', 11211);


function set_mem($m, $n, $g)
{
    //echo "set_mem(m, $n, $g)";
    $t = trim($_GET[$g]);
    $val = array('ON', 'OFF', 'AUTO');
    $v = $m->get($n);
    if($v === FALSE) $v = 'AUTO';
    //echo "$n = $v => $t\n";
    if(in_array($t, $val) && $v != $t)
    {
        $m->set($n.'_C', $t);
        if($t != 'AUTO') $m->set($n, ($t == 'ON') ? 1 : 0);
    }
}

if(isset($_GET['g']) && ($_GET['g'] == 1 || $_GET['g'] == 2))
{
    //echo "Check.\n";
    if($_GET['g'] == 1)
    {
        //echo "G=1\n";
        var_dump(isset($_GET['circ']));
        if(isset($_GET['circ'])) set_mem($m, 'G1_CIRC', 'circ');
        if(isset($_GET['heat'])) set_mem($m, 'G1_HEAT', 'heat');
        if(isset($_GET['vent'])) set_mem($m, 'G1_VENT', 'vent');
    }
    else
    {
        //echo "G=2\n";
        if(isset($_GET['circ'])) set_mem($m, 'G2_CIRC', 'circ');
        if(isset($_GET['heat'])) set_mem($m, 'G2_HEAT', 'heat');
        if(isset($_GET['vent'])) set_mem($m, 'G2_VENT', 'vent');
    }
}
$url = (isset($_SERVER['HTTP_REFERER'])) ? $_SERVER['HTTP_REFERER'] : 'http://alexv-gw.linkpc.net/green/';
header("Location: $url");
//echo "URL: $url\n";
//print_r($_GET);
?>
