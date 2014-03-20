<?php
session_start();
if ( !isset($_SESSION['_logged_']) || $_SESSION['_logged_'] === false ) {
		header('Location: login.php');
        die();
}

?>

<DOCTYPE html>
<html lang="en" ng-app="Scripta">
<head> 
  <meta charset="utf-8">
  <title>Scr|pta</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <meta name="description" content="">
  <meta name="author" content="">
  <link rel="shortcut icon" type="image/x-icon" href="img/favicon.ico" />
  <link href="//netdna.bootstrapcdn.com/bootstrap/3.0.0/css/bootstrap.no-icons.min.css" rel="stylesheet">
  <link href="//netdna.bootstrapcdn.com/font-awesome/3.2.1/css/font-awesome.min.css" rel="stylesheet"> 
  <!-- <link href="css/bootstrap.no-icons.min.css" rel="stylesheet">
  <link href="css/font-awesome.min.css" rel="stylesheet"> -->
  <link href="css/theme.css" rel="stylesheet">
  <link href="css/alertify.css" rel="stylesheet">
  <link href='http://fonts.googleapis.com/css?family=News+Cycle:400,700' rel='stylesheet' type='text/css'>
  <link href="css/custom.css" rel="stylesheet">
</head>
<body ng-controller="CtrlMain">
  <header class="navbar navbar-static-top navbar-default scripta-trigger" role="banner">
    <div class="container">
      <div class="navbar-header">
        <button class="navbar-toggle" type="button" data-toggle="collapse" data-target=".navbar-collapse">
          <span class="icon-bar"></span>
          <span class="icon-bar"></span>
          <span class="icon-bar"></span>
        </button>
        <a ng-href="#/status" class="navbar-brand">
        	<img alt="scripta" src="img/scripta-logo-ext-152.png">
        </a>
      </div>
      <div class="navbar-collapse collapse">
        <ul class="nav navbar-nav">
          <li menu-active>
            <a ng-href="#/status">
              <b>{{title}}</b>
            </a>
          </li>
          <li menu-active><a ng-href="#/miner">Miner</a></li>
          <li menu-active><a ng-href="#/settings">Settings</a></li>
          <li menu-active><a ng-href="#/backup">Backup</a></li>
        </ul>
        <div class="navbar-nav navbar-right">

          <span class="navbar-text scripta-more">{{counter}}s</span>
          <button class="btn btn-danger navbar-btn ng-cloak" ng-show="downNow" title="Hopefully it's restarting">
            {{downTime}}s downtime
          </button>
          <button class="btn btn-default navbar-btn" ng-click="tick(1,1)" title="Auto refresh in {{counter}}s">
            <i class="icon-refresh" ng-class="{'icon-spin':counter<2}"></i>
          </button>
			<button class="btn btn-default navbar-btn"  title="Logoout" onclick="location.href='f_logout.php'">
           Logout
          </button> 
        </div>
      </div>
    </div>
  </header>

  <div class="container" ng-class="{down:status.minerDown}">
    <div ng-view>
      <h1 class="text-center">
        Loading angular.js the first time might take some seconds... Hang tight!
      </h1>
    </div>
  </div>

  <footer>
    <div class="container">
      <hr />
      <p>
        <span class="pull-right">
          Miner {{status.uptime|duration}}
          &nbsp;-&nbsp;
          Pi {{status.pi.uptime|duration}}
          &nbsp;-&nbsp;
          Temp {{status.pi.temp}} °C
          &nbsp;-&nbsp;
          Load {{status.pi.load|number:2}}
        </span>
        <a href='http://www.lateralfactory.com/scripta/'>Scripta</a>, by <a href='http://www.lateralfactory.com'>Lateral Factory</a> under GPLv3 License
      </p>
LTC Donations welcome : Lcb3cy5nPnh3pQWPCpa55Zg8ShZj5kUHYC	
    </div>
  </footer>
  <script src="js/alertify.min.js"></script>
  <script src="js/jquery.min.js"></script>
  <script src="js/highcharts.js"></script>
  <script src="js/bootstrap.min.js"></script>
  <script src="js/angular.min.js"></script>

  <script src="ng/app.js"></script>
  <script src="ng/services.js"></script>
  <script src="ng/controllers.js"></script>
  <script src="ng/filters.js"></script>
  <script src="ng/directives.js"></script>
  <script>
	$(document).ready(function() {
		
		$.ajax({
			type: "POST",
			url: "update/ctrl.php",
			success: function(returnMessage) {
				if (returnMessage != 0) {  
					var r=confirm(returnMessage);
					if (r==true) {
						$.ajax({
							type: "POST",
							url: "update/start.php",
							success: function(returnMessage) {
								alert(returnMessage);
								window.location = "index.php";
							}
						});
					}
				}
			},
			error: function(returnMessage) {
				alert("Error");	
				window.location = "index.php";
			}
		});
	

 	});

	function ctrl_host(cmd) {
		var retVal = prompt("Enter root password : ", "");
                if ( retVal === '' ) Alert("Password required!");
                else {
                        $.get('f_hostHardCtl.php?command='+cmd+'&pass='+retVal, function(data) {
              
				alert('msg: '+data);
                        });
                }
	}
  </script>
</body>
</html>
