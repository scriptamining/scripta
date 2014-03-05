<?php
session_start();

if ( !isset($_SESSION['_logged_']) || $_SESSION['_logged_'] === false ) {
	die();
} 

/*
f_status gets values that people want to see in realtime
returns success, status data and errors
*/
header('Content-type: application/json');

include('inc/cgminer.inc.php');

// Miner data
//$r['summary'] = cgminer('summary', '')['SUMMARY'];
$devs=cgminer('devs');
$pools=cgminer('pools');

if(!empty($devs['data']['DEVS'])){
  $r['status']['devs'] = $devs['data']['DEVS'];
}
else{
  $r['status']['devs'] = array();
}
if(!empty($pools['data']['POOLS'])){
  $r['status']['pools'] = $pools['data']['POOLS'];
  $r['status']['minerUp'] = true;
  $r['status']['minerDown'] = false;
}
else{
  $r['status']['pools'] = array();
  $r['status']['minerUp'] = false;
  $r['status']['minerDown'] = true;
}

// Debug miner data
if(!empty($_REQUEST['dev']) && $r['status']['minerUp']){
  $r['status']['devs'][]=array('Name'=>'Hoeba','ID'=>0,'Temperature'=>rand(20,35),'MHS5s'=>rand(80000,100000),'MHSav'=>rand(90000,100000),'LongPoll'=>'N','Getworks'=>200,'Accepted'=>rand(70,200),'Rejected'=>rand(1,10),'HardwareErrors'=>rand(0,50),'Utility'=>1.2,'LastShareTime'=>time()-rand(0,10));
  $r['status']['devs'][]=array('Name'=>'Debug','ID'=>1,'Temperature'=>rand(20,35),'MHS5s'=>rand(40000,50000),'MHSav'=>rand(45000,50000),'LongPoll'=>'N','Getworks'=>1076,'Accepted'=>1324,'Rejected'=>1,'HardwareErrors'=>46,'Utility'=>1.2,'LastShareTime'=>time()-rand(0,40));
  $r['status']['devs'][]=array('Name'=>'Wut','ID'=>2,'Temperature'=>rand(20,35),'MHS5s'=>rand(6000,9000),'MHSav'=>rand(7000,8000),'LongPoll'=>'N','Getworks'=>1076,'Accepted'=>1324,'Rejected'=>1,'HardwareErrors'=>46,'Utility'=>1.2,'LastShareTime'=>time()-rand(0,300));
  $r['status']['devs'][]=array('Name'=>'Wut','ID'=>4,'Temperature'=>rand(20,35),'MHS5s'=>0,'MHSav'=>0,'LongPoll'=>'N','Getworks'=>400,'Accepted'=>0,'Rejected'=>0,'HardwareErrors'=>0,'Utility'=>0,'LastShareTime'=>time()-rand(400,900));
  $r['status']['devs'][]=array('Name'=>'More','ID'=>3,'Temperature'=>rand(20,35),'MHS5s'=>rand(500,1000),'MHSav'=>rand(600,800),'LongPoll'=>'N','Getworks'=>1076,'Accepted'=>1324,'Rejected'=>1,'HardwareErrors'=>46,'Utility'=>1.2,'LastShareTime'=>time()-rand(0,300));
  $r['status']['pools'][]=array('POOL'=>5,'URL'=>'http://stratum.mining.eligius.st:3334','Status'=>'Alive','Priority'=>9,'LongPoll'=>'N','Getworks'=>10760,'Accepted'=>50430,'Rejected'=>60,'Discarded'=>21510,'Stale'=>0,'GetFailures'=>0,'RemoteFailures'=>0,'User'=>'1BveW6ZoZmx31uaXTEKJo5H9CK318feKKY','LastShareTime'=>1375501281,'Diff1Shares'=>20306,'ProxyType'=>'','Proxy'=>'','DifficultyAccepted'=>20142,'DifficultyRejected'=>24,'DifficultyStale'=>0,'LastShareDifficulty'=>4,'HasStratum'=>true,'StratumActive'=>true,'StratumURL'=>'stratum.mining.eligius.st','HasGBT'=>false,'BestShare'=>40657);
}

$devices = 0;
$MHSav = 0;
$MHS5s = 0;
$Accepted = 0;
$Rejected = 0;
$HardwareErrors = 0;
$Utility = 0;

if(!empty($r['status']['devs'])){
  foreach ($r['status']['devs'] as $id => $dev) {
    $devices += $dev['MHS5s']>0?1:0; // Only count hashing devices
    $MHS5s += $dev['MHS5s'];
    $MHSav += $dev['MHSav'];
    $Accepted += $dev['Accepted'];
    $Rejected += $dev['Rejected'];
    $HardwareErrors += $dev['HardwareErrors'];
    $Utility += $dev['Utility'];
    $r['status']['devs'][$id]['TotalShares']=$dev['Accepted']+$dev['Rejected']+$dev['HardwareErrors'];
  }
}


$ret = explode(' ',$MHS5s);
$KHS5s = ($ret[0]/1024). " Kh/s";
$ret = explode(' ',$MHSav);
$KHSav = ($ret[0]/1024). " Kh/s";

$r['status']['dtot']=array(
  'devices'=>$devices,
  'MHS5s'=>$MHS5s,
  'MHSav'=>$MHSav,
  'KHS5s'=>$KHS5s,
  'KHSav'=>$KHSav,
  'Accepted'=>$Accepted,
  'Rejected'=>$Rejected,
  'HardwareErrors'=>$HardwareErrors,
  'Utility'=>$Utility,
  'TotalShares'=>$Accepted+$Rejected+$HardwareErrors);

// CPU intensive stuff
if(!empty($_REQUEST['all'])){
  $ret = sys_getloadavg();
  $r['status']['pi']['load'] = $ret[2];
  $ret = explode(' ', exec('cat /proc/uptime'));
  $r['status']['pi']['uptime'] = $ret[0];
  $r['status']['pi']['temp'] = exec('cat /sys/class/thermal/thermal_zone0/temp')/1000;

  // What other interesting stuff is in summary?
  $summary=cgminer('summary');
  if(!empty($summary['data']['SUMMARY'][0]['Elapsed'])){
    $r['status']['uptime'] = $summary['data']['SUMMARY'][0]['Elapsed'];
  }
  else{
    $r['status']['uptime'] = 0;
  }
}

$r['status']['time'] = time();

echo json_encode($r);
?>
