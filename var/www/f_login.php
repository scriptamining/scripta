<?php
session_start();
if ( !isset($_SESSION['_logged_']) ) $_SESSION['_logged_'] = false;

$configFolder='/opt/scripta/etc/';
$configUipwd=$configFolder.'uipasswd';


if ( isset( $_POST['userPassword'] ) and !empty( $_POST['userPassword'] ) )  {

	$ret = @file_get_contents($configUipwd); 
		
	if (empty($ret) ) return json_encode(false);
	
	$data = explode('scripta:',$ret);

	$pwd = md5($_POST['userPassword']); 
	
	if ( $data[1] == $pwd ) {
		
		$_SESSION['_logged_'] = true;
		
		echo 1;
	}
	else  {
		
		echo 0;
	 }
}
else return json_encode(false);
	


?>
