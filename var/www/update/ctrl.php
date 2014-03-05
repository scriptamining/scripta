<?php 
require('version.php');
require('update.php');

$update = new AutoUpdate(true);
$update->currentVersionName = CURRENT_VERSION_NAME;
$update->currentVersion = CURRENT_VERSION; 
$update->updateUrl = REPOSITORY_URL; 

//Check for a new update
$latest = $update->checkUpdate();

if ($latest !== false) {
	if ($latest > $update->currentVersion) {
		//Install new update
		echo "An update is available.\nDo you want to download and install it now?\nMining activities will not be affected.\nCurrent Version: ".$update->currentVersionName."\nNew version: ".$update->latestVersionName;
	}
	else {
		//echo "Current Version is up to date";
		echo '';
	}
}
else {
	echo $update->getLastError();
}

?>