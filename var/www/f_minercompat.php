<?php
session_start();
if ( !isset($_SESSION['_logged_']) || $_SESSION['_logged_'] === false ) {
        header('Location: index.php');
        die();
}

/*
f_minercompat parses miner.conf into options and pools
*/
header('Content-type: application/json');
$configFolder='/opt/scripta/etc/';
$configMiner=$configFolder.'miner.conf';
$configPools=$configFolder.'miner.pools.json';
$configOptns=$configFolder.'miner.options.json';

$miner = json_decode(@file_get_contents($configMiner), true);

if(!empty($miner)&&is_array($miner)){
  foreach ($miner as $key => $value) {
  	if($key=='pools'){
    	$r['data']['pools']=$value;
  	}
  	else{
    	$r['data']['options'][]=array('key'=>$key,'value'=>$value);
  	}
  }
  file_put_contents($configPools, json_encode($r['data']['pools'], JSON_PRETTY_PRINT | JSON_NUMERIC_CHECK));
  file_put_contents($configOptns, json_encode($r['data']['options'], JSON_PRETTY_PRINT | JSON_NUMERIC_CHECK));
  $r['info'][]=array('type' => 'success', 'text' => 'Compatibility reload done');
}
else{
  $r['info'][]=array('type' => 'error', 'text' => 'miner.conf not found');
}

echo json_encode($r);
?>
