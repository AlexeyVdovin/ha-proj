<?php
header('Cache-Control: no-cache');
header('Cache-Control: max-age=0');
header('Cache-Control: no-store');

$string = file_get_contents("data.json");
echo $string;
?>