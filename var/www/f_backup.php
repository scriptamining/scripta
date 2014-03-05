<?php
/*
- default: Get a list of all backups and their content
- backup: Copy list of items to backupFolder
- export: Zip a folder and serve as download
*/

session_start();
if ( !isset($_SESSION['_logged_']) || $_SESSION['_logged_'] === false ) {
		echo json_encode(false);
        die();
}

// Default backup folder
$backupFolder= '/opt/scripta/etc/backup/';
$baseFolder  = '/opt/scripta/';

include('inc/backup.inc.php');

// Zip a folder and serve as download
if (!empty($_REQUEST['export'])) {
  $zipFolder='/opt/scripta/etc/';
  $zipData='backup/'.$_REQUEST['export'];
  $zipName=$_REQUEST['export'].'.zip';

  serve_zip($zipFolder,$zipName,$zipData);
  exit;
}

header('Content-type: application/json');

// Upload a zipped backup
if (!empty($_FILES['file'])) {
  $name = empty($_REQUEST['name']) ? @date('Ymd-Hi') : $_REQUEST['name'];
  $r['error']=upload($_FILES['file'],$name);
  if(empty($r['error'])){
    header('Location: index.html#/backup');
    exit;
  }
}

// Copy list of items to backupFolder
elseif (!empty($_REQUEST['backup'])) {
  $items = json_decode($_REQUEST['backup'], true);
  if(!empty($items)&&is_array($items)&&!empty($_REQUEST['name'])){
    $r['info'][]=array('type' => 'success', 'text' => 'Backup saved');
    foreach ($items as $key => $value) {
      if($value['selected']){
        $r['data'][$key]=secure_copy($baseFolder.$value['name'],$backupFolder.$_REQUEST['name'].'/'.$value['name']);
      }
    }
  }
  else{
    $r['info'][]=array('type' => 'success', 'text' => 'Backup not saved');
  }
}

// Copy folder from backupFolder back to baseFolder
elseif (!empty($_REQUEST['restore']) && @is_dir($backupFolder.$_REQUEST['restore'])) {
  $restore = $_REQUEST['restore'];

  // Scan backup and remove . & ..
  $items = @scandir($backupFolder.$restore);
  if(!empty($items)){
    array_shift($items);
    array_shift($items);
    $r['info'][]=array('type' => 'success', 'text' => 'Restored!');
  }
  else{
    $r['info'][]=array('type' => 'danger', 'text' => 'Could not find backup');
  }
  foreach ($items as $key => $value) {
    // To return success: $r['data'][$key]=...
    secure_copy($backupFolder.$restore .'/'.$value,$baseFolder.$value);
  }
}

// Get a list of all backups and their content
else{
  // Scan backup folder and remove . & ..
  $items = @scandir($backupFolder);
  if(!empty($items)){
    array_shift($items);
    array_shift($items);
  }
  else{exit;}
  rsort($items);

  // Scan subfolders, in the future this should also return the files
  foreach ($items as $key => $value) {
    $r['data'][$key]['dir']=$value;
    $r['data'][$key]['items']=@scandir($backupFolder.'/'.$value);
    if(!empty($r['data'][$key]['items'])){
      array_shift($r['data'][$key]['items']);
      array_shift($r['data'][$key]['items']);
    }
  }
}

echo json_encode($r);
?>
