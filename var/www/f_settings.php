<?php
session_start();

if ( !isset($_SESSION['_logged_']) || $_SESSION['_logged_'] === false ) {
	echo json_encode(false);
	die();
}

include('inc/ChromePhp.php');

/*
f_settings syncs settings in different files and always returns new state
returns settings and ['date']
*/
header('Content-type: application/json');
$configFolder='/opt/scripta/etc/';
$configScripta=$configFolder.'scripta.conf';
$configUipwd=$configFolder.'uipasswd';
$configOptns=$configFolder.'miner.options.json';
$configPools=$configFolder.'miner.pools.json';
$configMiner=$configFolder.'miner.conf';


// Set new password
if (isset($_REQUEST['pass'])) {
  $pass=json_decode($_REQUEST['pass']);
  if (strlen($pass) > 0) {
    file_put_contents($configUipwd,'scripta:' . md5($pass) );
    $r['info'][]=array('type' => 'success', 'text' => 'Web password saved');
  }
}

// Manage settings
elseif (!empty($_REQUEST['settings'])) {
  $newdata   = json_decode($_REQUEST['settings'], true);
  $r['data'] = json_decode(@file_get_contents($configScripta), true);

  // Sync current with new settings
  if(!empty($newdata)&&is_array($newdata)){
    foreach ($newdata as $key => $value) {
      $r['data'][$key]=$value;
    }
    file_put_contents($configScripta, json_encode($r['data'], JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES));
    $r['info'][]=array('type' => 'success', 'text' => 'Configuration saved');
  }
  // Load current settings
  else{
    $r['info'][]=array('type' => 'info', 'text' => 'Configuration loaded');
  }

  if(isset($r['data']['userTimezone'])){
    date_default_timezone_set($r['data']['userTimezone']);
    $r['data']['date'] = date('Y-m-d H:i:s');
  }
}

// Manage pools
elseif (!empty($_REQUEST['pools'])) {
  $newdata   = json_decode($_REQUEST['pools'], true);
  $r['data'] = json_decode(@file_get_contents($configPools), true);
    
  foreach ($r['data'] as $id => $p) 
  { 
    $r['data'][$id]['url'] = str_replace('stratum tcp','stratum+tcp',$p['url']);
  }
  $m=0;
  foreach ($newdata as $id => $p) 
  { 
    $newdata[$id]['url'] = str_replace('stratum tcp','stratum+tcp',$p['url']);
    if($p['prio'] > $m) $m=$p['prio'];
  }

  // reorder pool list based on priority
  if($m>0){
    $pl=array();
    for ($pp = 0; $pp <= $m; $pp++) {      
      foreach ($newdata as $id => $p){
        if($p['prio'] == $pp) $pl[]=$newdata[$id];
      }
    }
    $newdata = $pl;
  }      

  // Overwrite current with new pools
  if(!empty($newdata)&&is_array($newdata)){
    file_put_contents($configPools, json_encode($newdata, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES));
    minerConfigGenerate();
    $r['data']=$newdata;
    $r['info'][]=array('type' => 'success', 'text' => 'Pools config saved');
  }
  // Load current settings
  elseif(!empty($r['data'])&&is_array($r['data'])){
    $r['info'][]=array('type' => 'info', 'text' => 'Pools config loaded');
  }
  // Load new settings
  else{
    $r['data']=array(array('url'=>'empty'));
    $r['info'][]=array('type' => 'error', 'text' => 'Pools config not found');
  }
}

// Manage miner.conf
elseif (!empty($_REQUEST['options'])) {
  $newdata   = json_decode($_REQUEST['options'], true);
  $r['data'] = json_decode(@file_get_contents($configOptns), true);

  // Overwrite current with new config
  if(!empty($newdata)&&is_array($newdata)){
    file_put_contents($configOptns, json_encode($newdata, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES));
    minerConfigGenerate();
    $r['data']=$newdata;
    $r['info'][]=array('type' => 'success', 'text' => 'Miner options saved');
  }
  // Load current settings
  elseif(!empty($r['data'])&&is_array($r['data'])){
    $r['info'][]=array('type' => 'info', 'text' => 'Miner options loaded');
  }
  // Load new settings
  else{
    $r['data']=array(array('key'=>'algo','value'=>'c'));
    $r['info'][]=array('type' => 'error', 'text' => 'Miner options not found');
  }
}

// Set system timezone to what is stored in settings 
elseif (!empty($_REQUEST['timezone'])) {
  $timezone = json_decode($_REQUEST['timezone']);
  ini_set( 'date.timezone', $timezone );
  putenv('TZ=' . $timezone);
  date_default_timezone_set($timezone);
  $r['data']['date'] = date('Y-m-d H:i:s');
  $r['info'][]=array('type' => 'info', 'text' => 'Timezone is '.$timezone );
}

echo json_encode($r);

function minerConfigGenerate(){
  global $configOptns,$configPools,$configMiner;
  $options = json_decode(@file_get_contents($configOptns), true);

  // Angular objects ==> miner
  // {key:k,value:v} ==> {k:v}
  foreach ($options as $o) {
    if ($o['key']=='scan' || $o['key']=='set-device'){ 
      $miner[$o['key']]=array($o['value']);     
    }
    else{ 
      $miner[$o['key']]=$o['value'];
    }
  }

  $miner['pools']= json_decode(@file_get_contents($configPools), true);
  file_put_contents($configMiner, json_encode($miner, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES));
}
?>
