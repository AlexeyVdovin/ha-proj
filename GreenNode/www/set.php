<?php
$m = new Memcached();
$m->addServer('/run/memcached/memcached.socket', 0);
header('Cache-Control: no-cache');
header('Cache-Control: max-age=0');
header('Cache-Control: no-store');

function startsWith($haystack, $needle)
{
     $length = strlen($needle);
     return (substr($haystack, 0, $length) === $needle);
}

function get_val($s)
{
    $arr = array('OFF' => 0, 'ON' => 1, 'AUTO' => 2);
    if(in_array($s, array('OFF', 'ON', 'AUTO')))
    {
        return $arr[$s];
    }
    return FALSE;
}

function check_val($min, $v, $max)
{
    if($v < $min) $v = $min;
    else if($v > $max) $v = $max;
    return $v;
}

function set_mem($m, $k1, $k2, $t)
{
  // echo "set_mem(m, $k1, $k2, $t)\n";
    $v = $m->get($k1);
    if($v === FALSE) $v = 2; // AUTO
  // echo "$k1 = $v => $t\n";
    if($t != $v)
    {
      // echo "Update: '".$k1." <- '$t'.\n";
        $m->set($k1, $t);
        if($k2 != FALSE && $t != 2) $m->set($k2, $t);
        if(($t == 2 || $t == 1) && ($k2 == 'G1_WATER' || $k2 == 'G2_WATER'))
        {
            $m->set($k2, $t == 1 ? 1 : 0);
            // $m->set($k2.'_T', time()); // Do not update last watering time!
        }
        if($k1 == "G1_PERIOD" || $k1 == "G1_DURATION")
        {
            $m->set('G1_WATER_T', time());
        }
        if($k1 == "G2_PERIOD" || $k1 == "G2_DURATION")
        {
            $m->set('G2_WATER_T', time());
        }

        // Update settings in config file.
        $f = file_get_contents('/var/www/store/settings.conf');
        $line = explode("\n", $f);
        $found = FALSE;
        foreach($line as $k => $l)
        {
            if(startsWith(trim($l), $k1))
            {
                $line[$k] = "$k1 = $t";
                $found = TRUE;
                break;
            }
        }
        if(!$found) array_push($line, "$k1 = $t");
      // print_r($line);
        $f = implode("\n", $line);
        file_put_contents('/var/www/store/settings.conf', $f);
    }
}

if(isset($_GET['g']) && ($_GET['g'] == 1 || $_GET['g'] == 2))
{
    //echo "Check.\n";
    if($_GET['g'] == 1)
    {
        //echo "G=1\n";
        if(isset($_GET['circ'])) set_mem($m, 'G1_CIRC_C', 'G1_CIRC', get_val(trim($_GET['circ'])));
        if(isset($_GET['heat'])) set_mem($m, 'G1_HEAT_C', 'G1_HEAT', get_val(trim($_GET['heat'])));
        if(isset($_GET['vent'])) set_mem($m, 'G1_VENT_C', 'G1_VENT', get_val(trim($_GET['vent'])));
        if(isset($_GET['water'])) set_mem($m, 'G1_WATER_C', 'G1_WATER', get_val(trim($_GET['water'])));
        if(isset($_GET['period'])) set_mem($m, 'G1_PERIOD', FALSE, check_val(10, trim($_GET['period']), 1440));
        if(isset($_GET['duration'])) set_mem($m, 'G1_DURATION', FALSE, check_val(0, trim($_GET['duration']), 60));
    }
    else
    {
        //echo "G=2\n";
        if(isset($_GET['circ'])) set_mem($m, 'G2_CIRC_C', 'G2_CIRC', get_val(trim($_GET['circ'])));
        if(isset($_GET['heat'])) set_mem($m, 'G2_HEAT_C', 'G2_HEAT', get_val(trim($_GET['heat'])));
        if(isset($_GET['vent'])) set_mem($m, 'G2_VENT_C', 'G2_VENT', get_val(trim($_GET['vent'])));
        if(isset($_GET['water'])) set_mem($m, 'G2_WATER_C', 'G2_WATER', get_val(trim($_GET['water'])));
        if(isset($_GET['period'])) set_mem($m, 'G2_PERIOD', FALSE, check_val(10, trim($_GET['period']), 1440));
        if(isset($_GET['duration'])) set_mem($m, 'G2_DURATION', FALSE, check_val(0, trim($_GET['duration']), 60));
    }
}
$url = (isset($_SERVER['HTTP_REFERER'])) ? $_SERVER['HTTP_REFERER'] : 'http:/kan-gw.linkpc.net:41080/green/';
header("Location: $url");
// echo "URL: $url\n";
// print_r($_GET);
?>
