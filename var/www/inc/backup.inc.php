<?php

// Folder: where to store zip temporarily
// Name: filename to serve
// Data: path to data starting from Folder
function serve_zip($zipFolder,$zipName,$zipData){
  if(!@is_dir($zipFolder.'/'.$zipData)){
    return;
  }
  elseif(!file_exists($zipFolder.$zipName)){
    exec('cd '.$zipFolder.'/'.$zipData.' ; zip -r '.$zipFolder.'/'.$zipName.' ./');
  }
  header('Content-Type: application/zip');
  header('Content-Disposition: attachment; filename=' . basename($zipFolder.'/'.$zipName) . ';' );
  header('Content-Transfer-Encoding: binary');
  header('Content-Length: ' . filesize($zipFolder.'/'.$zipName));
  readfile($zipFolder.'/'.$zipName);
  unlink($zipFolder.'/'.$zipName);
}

// F: corresponds to the $_FILES var
// name: filename to store
function upload($f,$name){
  // In case of warnings: return plaintext error
  header('Content-type: text/plain');
  if(empty($f) || $f['error']){
    return 'File error, did you select a file?';
  }

  $tempFolder = '/opt/scripta/etc/backup/';

  if(!move_uploaded_file($f['tmp_name'], $tempFolder . 'temp' )){
    return 'Could not move file. Permissions problem?';
  }
  // Unzip and remove zipfile
  exec('cd '.$tempFolder.' ; unzip temp -d '.$name);
  unlink($tempFolder.'temp');
}

// Copy a file or folder
function secure_copy($src,$dst){
  // Check destination folder
  $dstDir=dirname($dst);
  $r['success']=false;
  $r['src']=pathinfo($dst,PATHINFO_BASENAME);
  $r['dstDir']=$dstDir;

  // Make dir if there isnt
  if (!@is_writable($dstDir)) {
    $r['mkDir']='Making destination dir!';
    @mkdir($dstDir,0777,true); 
  }

  // Predicting errors for debugging
  if (!@is_readable($src)) {
    $r['info'][]=array('type' => 'danger', 'text' => 'Source file is not readable');
  }
  elseif (is_file($src)&&is_dir($dst) || is_dir($src)&&is_file($dst)) {
    $r['info'][]=array('type' => 'danger', 'text' => 'File and folder mixup');
  }
  elseif (is_file($dst)) {
    $r['info'][]=array('type' => 'danger', 'text' => 'Destination file already exists');
  }

  // Copy it already!
  if (@is_file($src)) {
    $r['type']='file';
    if (!@copy($src, $dst)) {
      $r['files'][0]=array('file'=>pathinfo($dst,PATHINFO_BASENAME),'success'=>false);
    }
    else{
      $r['success']=true;
      $r['files'][0]=array('file'=>pathinfo($dst,PATHINFO_BASENAME),'success'=>true);
    }
  }
  elseif (@is_dir($src)) {
    $r['type']='dir';
    if (recurse_copy($src, $dst)) {
      $r['success']=true;
    }
  }

  return $r;
}

// Copy a folder recursively
function recurse_copy($src,$dst,$level=3) {
  $success=true;
  if($level==0) return $success;
  global $r;
  $dir = opendir($src); 
  @mkdir($dst); 
  while(false !== ( $file = readdir($dir))) { 
    if (( $file != '.' ) && ( $file != '..' )) { 
      if ( is_dir($src . '/' . $file) ) { 
        $c=recurse_copy($src . '/' . $file,$dst . '/' . $file,$level-1); 
      } 
      else {
        $c=@copy($src . '/' . $file,$dst . '/' . $file);
        $r['files'][]=array('file'=>pathinfo($dst,PATHINFO_BASENAME),'success'=>$c);
      }
      $success=$success&&$c;
    } 
  } 
  closedir($dir);
  return $success;
}
?>
