<?php 
require('version.php');
require('update.php');

$update = new AutoUpdate(true);
$update->currentVersionName = CURRENT_VERSION_NAME;
$update->currentVersion = CURRENT_VERSION; 
$update->updateUrl = REPOSITORY_URL; 

//Check for a new update
echo "Download file...\n";
$latest = $update->checkUpdate();
if ($latest !== false) {
	if ($latest > $update->currentVersion) {
		//Install new update
		echo "Installing Update...\n";
		if ($update->update()) {
			echo "Update successful!";
		}
		else {
			echo "Update failed!";
		}
		
	}
	else {
		echo "Current Version is up to date";
	}
}
else {
	echo $update->getLastError();
}

?>
