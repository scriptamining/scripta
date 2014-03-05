<?php
ini_set("expect.loguser", "Off");
/*
host.inc.php
default command is summary
r.data
r.error
*/

/*
* 0 ---> Reboot
* 1 ---> Shutdown
*/

function hostHardCtl($op,$pass){
	openlog("myScriptLog", LOG_PID | LOG_PERROR, LOG_LOCAL0);
        syslog(LOG_WARNING, "Attempting to exec hostHardCtl");

	switch ($op) {
	case 0:
        	$cmd = '"/sbin/shutdown -r now"';
        	break;
  	case 1:
        	$cmd = '"/sbin/shutdown -h now"';
        	break;
  	default:
        	syslog(LOG_WARNING, "unknown command");
        	break;
	}

	//$cmd = '"ls -la /root"';	

	$cases = array (
   		array (0 => "Password: ", 1 => "PASSWORD")
	);

	$full_cmd = "su -c ".$cmd;
	syslog(LOG_WARNING, "execute: ". $full_cmd);	
        $stream = expect_popen($full_cmd);
	$ret = expect_expectl($stream, $cases);
	
	switch ($ret) {
	   case "PASSWORD":
        	fwrite ($stream, $pass."\n");
		syslog(LOG_WARNING, "pwd: ". $pass);
       		break;
    	   case EXP_TIMEOUT:
		syslog(LOG_WARNING, "EXP_TIMEOUT");	
		return false;
		break;
   	   case EXP_EOF:
		syslog(LOG_WARNING, "EXP_EOF");
		return false;
		break;
   	   default:
		syslog(LOG_WARNING, "EXP_TIMEOUT");
		return false;
        	break;
	}

	$out = '';
	while ($line = fgets($stream)) {
      		 $out .= $line;
		 syslog(LOG_WARNING, "ret: ". $line);
	}
	fclose ($stream);
        closelog();
	return $out;
}
?>

