<?php

require_once('Mail.php');

/**********************************
 
Generic functions theat may be called from many differant places

*/

function sendEmail($settings, $subject, $body) {

    $mailSettings = array(
        'host' => $settings['alertSmtp'], 
        'debug' => false,
        'auth' => true,
        'username' => $settings['alertSmtpUser'],
        'password' => $settings['alertSmtpPwd'],
        'port' => $settings['alertSmtpPort'],
        
      );
  
    //$settings['alertDevice']

    $mail = Mail::factory('smtp', $mailSettings );

    $headers = array('From'=>$settings['alertEmail'], 'Subject'=>$subject);
    $esito = $mail->send($settings['alertEmail'], $headers, $body);
	if (PEAR::isError($esito)) { print($esito->getMessage());}else{print "end";}
}
