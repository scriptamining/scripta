<?php
if ( isset($_SESSION['_logged_']) and ($_SESSION['_logged_'] === true) )  {
	header('location: index.html');
	exit();
}

?>

<!DOCTYPE html>
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
  <link href="css/font-awesome.min.css" rel="stylesheet">  -->
  <link href="css/theme.css" rel="stylesheet">
  <link href="css/alertify.css" rel="stylesheet">
  <link href='http://fonts.googleapis.com/css?family=News+Cycle:400,700' rel='stylesheet' type='text/css'>
  <link href="css/custom.css" rel="stylesheet">
</head>
<body >
  <header class="navbar navbar-static-top navbar-default scripta-trigger" >
    <div class="container">
      <div class="navbar-header">
        <button class="navbar-toggle" type="button" >
          <span class="icon-bar"></span>
          <span class="icon-bar"></span>
          <span class="icon-bar"></span>
        </button>
        <a class="navbar-brand">
        	<img alt="scripta" src="img/scripta-logo-ext-152.png">
        </a>
      </div>
      <div class="navbar-collapse collapse">
        
        <div class="navbar-nav navbar-right">
			
        </div>
      </div>
    </div>
  </header>

    <div >
		<br/><br/><br/>
		<p class="text-center">
        <form name="formLogin" id="formLogin" class="form-horizontal"  method="post">
		<fieldset>
			<div class="form-group">
				<label for="userPassword2" class="control-label col-sm-3 col-md-2">Password</label>
				<div class="col-sm-3 col-md-4">
					<input type="password" placeholder="Password" id="userPassword" name="userPassword"  class="form-control">
				</div>
				<div class="btn-group">
					<button type="button" id="loginbutton" class="btn btn-default">Login</button>
					
				</div>
			</div>
		 </fieldset>
		 </form>
		</p>
    </div>
  </div>

  <footer>
    <div class="container"> 
      <hr />
      <p>
        <a href='http://http://www.lateralfactory.com/scripta/'>Scripta</a>, by <a href='http://www.lateralfactory.com'>Lateral Factory</a> under GPLv3 License
      </p>
LTC Donations welcome : Lcb3cy5nPnh3pQWPCpa55Zg8ShZj5kUHYC
    </div>
  </footer>
  <script src="js/alertify.min.js"></script>
  <script src="js/jquery.min.js"></script>
  <script src="js/highcharts.js"></script>
  <script src="js/bootstrap.min.js"></script>
  <script>
	$(document).ready(function() {
		$(document).keypress(function(e) {
			
			if(e.which == 13) {
				e.preventDefault();
			}
		});
		
		$('#loginbutton').click(function(e){
			e.preventDefault();
			
			var sData = $("#formLogin").serialize();
			$.ajax({
				type: "POST",
				url: "f_login.php",
				data: sData,
				success: function(returnMessage) {
					if (returnMessage == 1) 
						window.location = "index.php";
					else
						alert("Incorrect password");
					
				},
				error: function(returnMessage) {
					alert("Error");	
					window.location = "login.php";
				}
			});
		});
	});
  </script>
</body>
</html>


