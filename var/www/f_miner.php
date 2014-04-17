<?php
session_start();

if ( !isset($_SESSION['_logged_']) || $_SESSION['_logged_'] === false ) {
	echo json_encode(false);
	die();
}

/*
f_copy issues a command to the api of the miner
returns success, command, miner response and possible errors
*/
header('Content-type: application/json');

include('inc/bfgminer.inc.php');

if (empty($_REQUEST['command']) && empty($_REQUEST['parameter'])) {
  $r = bfgminer();
}
elseif(empty($_REQUEST['parameter'])){
  $r = bfgminer($_REQUEST['command']);
}
else{
  $r = bfgminer($_REQUEST['command'],$_REQUEST['parameter']);
}

echo json_encode($r);
?>
