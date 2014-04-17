<?php
session_start();

if ( !isset($_SESSION['_logged_']) || $_SESSION['_logged_'] === false ) {
	echo json_encode(false);
	die();
}

header('Content-type: application/json');

include('inc/bfgminer.inc.php');

//if ( !empty($_REQUEST['command']) ) {
  $r = bfgminerHardCtl($_REQUEST['command']);
  echo json_encode($r);
//}
?>
