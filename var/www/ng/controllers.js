'use strict';

/* Controllers */

angular.module('Scripta.controllers', [])


// Main: stores status
.controller('CtrlMain', function($scope,$http,$timeout,$window,$filter) {
  // Settings
  $scope.settings={};
  $scope.settingsMaster={};
  // Pools
  $scope.pools={};
  $scope.options={};
  // Status
  $scope.status={};
  $scope.status.extra=true; // Request extra data
  $scope.title="Miner interface initialization";
  // Refresh
  $scope.intervalAuto = true; // Automatically adjust interval
  $scope.intervalMax = 20; // Default refresh rate
  // Live graph
  $scope.live=[];
  $scope.settings.liveMax=50;
  $scope.upLast=0;
  $scope.downLast=0;
  // Alerts
  Alertify.log.delay=10000;

  // Sync settings
  // Note: not possible to remove settings!
  $scope.sync = function(action,data,alert) {
    action = action || 'settings';
    data = data || 'load';
    $http.get('f_settings.php?'+action+'='+angular.toJson(data)).success(function(d){
      if(d.info){
        angular.forEach(d.info, function(v,k) {Alertify.log.create(v.type, v.text);});
      }
      if(action=='settings'){
        $scope.settings=angular.copy(d['data']);
      }
      else if(action=='pools'){
        $scope.pools=d['data'];
      }
      else if(action=='options'){
        $scope.options=d['data'];
      }
      else if(action=='timezone'){
        $scope.settings.date=d.data.date;
      }
    });
  }
  $scope.syncDelay = function(ms,action,data,alert) {
    action = action || 'settings';
    data = data || false;
    ms = ms || 1000;
    var syncNow = function(){
      $scope.sync(action,data,alert);
    }
    return $timeout(syncNow, ms);
  }

  // Sync settings
  $scope.sync('settings')

  // Get status and save in scope
  $scope.tick = function(once,all) {
    $http.get('f_status.php?'+($scope.settings.userDeveloper?'dev=1&':'')+(all||$scope.status.extra?'all=1':'')).success(function(d){
      if(d.info){
        angular.forEach(d.info, function(v,k) {Alertify.log.create(v.type, v.text);});
      }
      // Update status
      angular.forEach(d.status, function(v,k) {$scope.status[k]=v;});
      // Title
      $scope.title=$scope.status.minerDown?'Miner DOWN -':'['+$filter('mhs')($scope.status.dtot.MHS5s)+'h] ['+$scope.status.dtot.devices+' dev]';
      // Live Graphs
      $scope.live.push([Date.now(),1000000*$scope.status.dtot.MHS5s]);
      // Stop requesting extra data
      $scope.status.extra=false;
    })
    .error(function(){
      // Title
      $scope.title='Scripta DOWN -';
      // Live Graphs
      $scope.live.push([Date.now(),0]);
    })
    .then(function(){
      if($scope.live.length>$scope.settings.liveMax){
        $scope.live=$scope.live.slice(-$scope.settings.liveMax);
      }
      // Manage interval
      if($scope.interval<$scope.intervalMax){
        $scope.interval++;
      }
    });
  }

  $scope.intervalSet = function(num) {
    if(num<2){
      $scope.intervalAuto=!$scope.intervalAuto;
      if($scope.intervalAuto) $scope.interval=1;
    }
    else{
      $scope.intervalMax=num;
      $scope.interval=num;
    }
  };
  var count = function () {
    $timeout(count, 1000);
    if($scope.counter>0){
      $scope.counter--;
    }
    else{
      $scope.counter=$scope.interval-1;
      $scope.tick();
    }
  };
  count();
  
  $scope.$watch('title', function(b,a) {
    $window.document.title=b+' Scripta';
  });
  
  $scope.$watch('intervalAuto', function(b,a) {
    Alertify.log.info('Automatic refresh rate '+(b?'en':'dis')+'abled');
  });
  
  $scope.$watch('intervalMax', function(b,a) {
    Alertify.log.info('Refresh rate is now '+b);
    if($scope.counter>b){
      $scope.counter=0;
    }
    $scope.status.extra=true;
  });

  $scope.$watch('status.minerDown', function(b,a) {
    if(b){
      $scope.upLast=Date.now();
      Alertify.log.error('Miner seems down');
    }
    else{
      $scope.downLast=Date.now();
      Alertify.log.success('Miner is up!');
    }
    $scope.interval=1;
    $scope.counter=0;
    $scope.status.extra=true;
  });
})


.controller('CtrlStatus', function($scope,$http) {
  $scope.status.extra=true;
  $scope.num=0;

  $scope.graphUpdate = function() {
    $http.get('f_graph.php').success(function(d){
		
      if(d){
        Alertify.log.success("Graphs updated");
      } 
      else{
        Alertify.log.error("Update graph ended in error");
      }
      $scope.num++;
    }).error(function(){
      Alertify.log.error("Update graph ended in error: https enabled?");
    });
  }
})


.controller('CtrlMiner', function($scope,$http,$timeout) {
  $scope.status.extra=true;
  $scope.sync('pools');
  $scope.sync('options');

  $scope.minerCompat = function(command,parameter) {
    $http.get('f_minercompat.php').success(function(d){
      if(d.info){
        angular.forEach(d.info, function(v,k) {Alertify.log.create(v.type, v.text);});
      }
      if(d.data.pools){
        $scope.pools=d.data.pools;
      }
      if(d.data.options){
        $scope.options=d.data.options;
      }
    });
  }

  $scope.cgminer = function(command,parameter) {
    $scope.tick();

    var execute = function(){
      $http.get('f_miner.php?command='+(command || 'summary')+'&parameter='+parameter).success(function(d){
        if(d.info){
          angular.forEach(d.info, function(v,k) {Alertify.log.create(v.type, v.text);});
        }
        $scope.tick();
      });
    }
    $timeout(execute, 1000);
  };

 $scope.cgminerHardCtl = function(command) {
    $scope.tick();

    var execute = function(){
      $http.get('f_minerHardCtl.php?command='+(command)).success(function(d){
        if(d.info){
          angular.forEach(d.info, function(v,k) {Alertify.log.create(v.type, v.text);});
        }
        $scope.tick();
      });
    }
    $timeout(execute, 1000);
  };

 $scope.hostHardCtl = function(command) {
   $scope.tick();			
   var execute = function(){
    $http.get('f_hostHardCtl.php?command='+(command)+'&pass=p0c4t0p4').success(function(d){
        if(d.info){
          angular.forEach(d.info, function(v,k) {Alertify.log.create(v.type, v.text);});
        }
        $scope.tick();
      });
    }
    $timeout(execute, 1000);
};	
 
  $scope.poolAdd = function(a) {
    a = a || {};
    $scope.pools.push(a);
    $scope.poolForm.$setDirty()
  };
  $scope.poolRemove = function(index) {
    $scope.pools.splice(index,1);
    $scope.poolForm.$setDirty()
  };
  $scope.poolSave = function() {
    $scope.sync('pools',$scope.pools,1);
    $scope.poolForm.$setPristine();
  };
  $scope.poolBack = function() {
    $scope.sync('pools',0,1);
    $scope.poolForm.$setPristine();
  };

  $scope.optionAdd = function(a) {
    a = a || {};
    $scope.options.push(a);
    $scope.optionForm.$setDirty()
  };
  $scope.optionRemove = function(index) {
    $scope.options.splice(index,1);
    $scope.optionForm.$setDirty()
  };
  $scope.optionSave = function() {
    $scope.sync('options',$scope.options,1);
    $scope.optionForm.$setPristine();
  };
  $scope.optionBack = function() {
    $scope.sync('options',0,1);
    $scope.optionForm.$setPristine();
  };
})


.controller('CtrlSettings', function($scope) {
  $scope.status.extra=true;
})


.controller('CtrlBackup', function($scope,$http,$timeout) {
  $scope.thisFolder = '/opt/scripta/';
  $scope.backupFolder = '/opt/scripta/backup/';
  $scope.backupName = GetDateTime();
  $scope.backups = [];
  $scope.restoring = 0;
  $scope.items = [
  {selected:true,name:'etc/scripta.conf'},
  {selected:true,name:'etc/miner.pools.json'},
  {selected:true,name:'etc/miner.options.json'}
  ];
  
  $scope.addItem = function() {
    $scope.items.push({selected:true,name:$scope.newItem});
    $scope.newItem = '';
  };
  $scope.selItem = function() {
    var count = 0;
    angular.forEach($scope.items, function(item) {
      count += item.selected ? 1 : 0;
    });
    return count;
  };
  

  $scope.backupLocal = function() {
    var promise = $http.get('f_backup.php?name='+$scope.backupName+'&backup='+angular.toJson($scope.items)).success(function(d){
      if(d.info){
        angular.forEach(d.info, function(v,k) {Alertify.log.create(v.type, v.text);});
      }
      angular.forEach(d.data, function(v,k) {
        if(v.success){
          $scope.items[k].bak=true;
          $scope.items[k].selected=false;
        }
        else{
          $scope.items[k].fail=true;
        }
      });// Add to existing
      $scope.reload();
    });
    return promise;
  };

  $scope.backupExport = function() {
    $scope.backupLocal().then(function(){
      $scope.exportZip($scope.backupName);
    });
  };

  $scope.exportZip = function(name) {
    name=name||$scope.backups[$scope.restoring].dir;
    window.location.href='f_backup.php?export='+name;
  };

  $scope.choose = function(i) {
    $scope.restoring=i;
  };

  $scope.restore = function() {
    $http.get('f_backup.php?restore='+$scope.backups[$scope.restoring].dir).success(function(d){
      if(d.info){
        angular.forEach(d.info, function(v,k) {Alertify.log.create(v.type, v.text);});
      }
      $scope.syncDelay(300,'settings');
      $scope.syncDelay(600,'pools');
      $scope.syncDelay(900,'options');
    });
  };

  $scope.reload = function(wait) {
    wait=wait||0;
    var syncNow = function(){
      $http.get('f_backup.php').success(function(d){
        if(d.data){
          $scope.backups=d.data;
        }
      });
    }
    //return 
    $timeout(syncNow, wait);
  };
  $scope.reload();
});


function GetDateTime() {
  var now = new Date();
  return [[now.getFullYear(),AddZero(now.getMonth() + 1),AddZero(now.getDate())].join(''), [AddZero(now.getHours()), AddZero(now.getMinutes())].join('')].join('-');
}

function AddZero(num) {
  return (num >= 0 && num < 10) ? '0' + num : num + '';
}
