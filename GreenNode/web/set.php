<?php
$m = new Memcached();
$m->addServer('localhost', 11211);
header('Cache-Control: no-cache');
header('Cache-Control: max-age=0');
header('Cache-Control: no-store');

function startsWith($haystack, $needle)
{
     $length = strlen($needle);
     return (substr($haystack, 0, $length) === $needle);
}

function set_mem($m, $n, $g)
{
    //echo "set_mem(m, $n, $g)\n";
    $t = trim($_GET[$g]);
    $v = $m->get($n.'_C');
    if($v === FALSE) $v = 'AUTO';
    //echo "$n = $v => $t\n";
    if(in_array($t, array('OFF', 'ON', 'AUTO')) && ($t != $v))
    {
        //echo "Update: '".$n."_C' <- '$t'.\n";
        $m->set($n.'_C', $t);
        if($t != 'AUTO') $m->set($n, ($t == 'ON') ? 1 : 0);
        //echo "Ctr: ".(($t == 'ON') ? 1 : 0)."\n";

        // Update settings in config file.
        $f = file_get_contents('/var/www/store/settings.conf');
        $line = explode("\n", $f);
        $found = FALSE;
        foreach($line as $k=>$l) if(startsWith(trim($l), $n.'_C'))
        {
            $line[$k] = $n."_C=$t";
            break;
        }
        if(!$found) array_push($line, ($n."_C=$t"));
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
$url = (isset($_SERVER['HTTP_REFERER'])) ? $_SERVER['HTTP_REFERER'] : 'http://alexv-gw.linkpc.net:29980/green/';
header("Location: $url");
//echo "URL: $url\n";
//print_r($_GET);
?>
