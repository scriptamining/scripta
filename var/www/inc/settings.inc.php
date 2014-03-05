<?php

if (!isset($_SERVER['HTTPS']) and php_sapi_name() <> 'cli') {
	header('Location: https://' . $_SERVER["SERVER_NAME"] . $_SERVER['REQUEST_URI']);
}

$settings = json_decode(@file_get_contents('/opt/scripta/etc/scripta.conf'), true);
$settings['donateEnable'] = true;



if(isset($settings['userTimezone'])){
  $timezone = $settings['userTimezone'];
  ini_set( 'date.timezone', $timezone );
  putenv('TZ=' . $timezone);
  date_default_timezone_set($timezone);
}


	

function writeSettings($settings, $file = 'scripta.conf') {
	// Call this when you want settings to be saved with writeSettings($settings);
	// can be used to save to an alternat file name with writeSettings($settings, 'OtherFileName.conf);

	file_put_contents("/opt/scripta/etc/" . $file, json_encode($settings, JSON_PRETTY_PRINT | JSON_NUMERIC_CHECK));
}

?>
