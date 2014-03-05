'use strict';

/* Directives */
angular.module('Scripta.directives', [])

// Sets background color by interpolating between green and red.
// Thinking about oth interpolation functions or maybe more colors
.directive('statusItem', function() {
  return function(scope, element, attrs) {
    var i=attrs.statusItem;
    var g=attrs.good; // Threshold good: green
    var b=attrs.bad; // Threshold bad: red

    function update(){
      var x=2*(i-g)/(b-g);
      x= x<0 ?0:x;
      x= x>2 ?2:x;
      element.css('background',(b==g)?'#666':'rgb('+Math.round(Math.min(x, 1)*(217-92)+92)+','+Math.round((2 - Math.max(x, 1)) * (184-83)+83)+',85)');
      element.css('color','#fff');
    }

    scope.$watch(attrs.good,       function(v) {g=v;update();});
    scope.$watch(attrs.bad,        function(v) {b=v;update();});
    scope.$watch(attrs.statusItem, function(v) {i=v;update();});
  }
})
// Toggles .active based on $location.path()
.directive('menuActive', function($rootScope,$location) {
  return function(scope, element, attrs) {
    $rootScope.$on('$routeChangeStart', function (event, next, current) {
      (element.children()[0].hash === '#'+$location.path()) ? element.addClass('active') : element.removeClass('active');
    });
  }
})

.directive('graphLive', function () {
  return {
    restrict: 'C',
    scope: {
      live: '='
    },
    controller: function ($scope, $element, $attrs) {
    },
    link: function (scope, element, attrs) {
      var chart = new Highcharts.Chart({
        chart: {
          renderTo: attrs.id,
          type: 'areaspline',
          spacingLeft: 0,
          spacingRight: 0
        },
        colors:   ['rgb(0,0,0)'],
        legend:   {enabled: false},
        subtitle: {text: ''},
        title: {
          text: 'Hashrate',
          align: 'center',
          verticalAlign: 'bottom',
        },
        xAxis: {
          type: 'datetime',
          minPadding: 0,
          maxPadding: 0,
          tickPixelInterval: 120
        },
        yAxis: {
          tickPixelInterval: 30,
          title: {
            text: ''
          },
          opposite: true
        },
        tooltip: {
          formatter: function() {
            var hs=this.y/1000,h=this.y+' ';
            if(hs > 10){h=hs.toPrecision(4)+' k';}hs/=1000;
            if(hs > 10){h=hs.toPrecision(4)+' M';}hs/=1000;
            if(hs > 10){h=hs.toPrecision(4)+' G';}hs/=1000;
            if(hs > 10){h=hs.toPrecision(4)+' T';}
            return Highcharts.dateFormat('%Y-%m-%d %H:%M:%S', this.x) +'<br/>'+ h +'h/s';
          }
        },
        plotOptions: {
          areaspline: {
            fillOpacity: 0.1,
            marker: {
              enabled: false,
              states: {
                hover: {
                  enabled: true
                }
              }
            }
          }
        },
        series: [{
          name: 'hashrate',
          data: [[Date.now(),0]]
        }]
      });

var liveTrack=0;
scope.$watch('live', function (newlist) {
  var n=angular.copy(newlist);
  if(!n || !n.length)return;
  if(liveTrack<2){
    chart.series[0].setData(n);
  }
  else{
    chart.series[0].addPoint(n[n.length-1],true,n.length<chart.pointCount);
  }
  liveTrack=n.length;
}, true);
}
}
});
