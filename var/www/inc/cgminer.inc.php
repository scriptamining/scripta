<?php
/*
cgminer.inc.php
default command is summary
r.data
r.error
*/

function cgminerHardCtl($cmd){
openlog("myScriptLog", LOG_PID | LOG_PERROR, LOG_LOCAL0);
                syslog(LOG_WARNING, "Attempting to exec");
                closelog();
 switch ( $cmd ) {
	case '0':
		openlog("myScriptLog", LOG_PID | LOG_PERROR, LOG_LOCAL0);
		exec("/opt/scripta/startup/miner-stop.sh");
		syslog(LOG_WARNING, "Executed exec");
		closelog();
	break;
	case '1':
		exec("nohup /opt/scripta/startup/miner-start.sh > /dev/null 2>&1 &");
	break;
	}	
return true;
}


function cgminer($command='summary',$parameter=false){
  $c['command'] = $command;
  $c['parameter']=$parameter;

  // Setup socket
  $client = @stream_socket_client('tcp://127.0.0.1:4028', $errno, $errorMessage);

  // Socket failed
  if ($client === false) {
    $r['info'][]=array('type' => 'error', 'text' => 'Miner: '.$errno.' '.$errorMessage);
  }
  // Socket success
  else{
    fwrite($client, json_encode($c));
    $response = stream_get_contents($client);
    fclose($client);
    
    // Cleanup json
    $response = preg_replace('/[^[:alnum:][:punct:]]/','',$response);

    // Add api response
    $r['data'] = json_decode($response, true);
    $r['info'][]=array('type' => 'info', 'text' => 'Miner: '.$command);
  }

  return $r;
}
?>
