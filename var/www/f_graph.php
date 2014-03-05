<?php
session_start();
if ( !isset($_SESSION['_logged_']) || $_SESSION['_logged_'] === false ) {
        echo json_encode(false);
        die();
}

require('inc/miner.inc.php');
include('inc/settings.inc.php');

$ret =  create_graph('mhsav-hour.png', '-1h', 'Last Hour')
  && create_graph('mhsav-day.png', '-1d', 'Last Day')
  && create_graph('mhsav-week.png', '-1w', 'Last Week')
  && create_graph('mhsav-month.png', '-1m', 'Last Month')
  && create_graph('mhsav-year.png', '-1y', 'Last Year');

echo $ret;    
 
function create_graph($output, $start, $title){
  $rrd = '/opt/scripta/var/rrd/';
  $png = '/opt/scripta/http/rrd/';

  $options = array(
    '--slope-mode',
    '--start', $start,
    '--title='.$title,
    '--vertical-label=Hash per second',
    '--lower=0',
    '--color=BACK#fff',      
    '--color=CANVAS#fff',    
    '--color=SHADEB#fff',   
    '--color=SHADEA#fff',   
    'DEF:hashrate='.$rrd.'hashrate.rrd:hashrate:AVERAGE',
    'CDEF:realspeed=hashrate,1000,*',
    'AREA:realspeed#ddd',
    'LINE:realspeed#000'
    );

  return rrd_graph($png.$output, $options);
}

?>
