'use strict';

/* Filters */

angular.module('Scripta.filters', [])
.filter('shortUrl', function() {
  return function(temp) {
    return temp.replace('//', '').split(':')[1];
  }
})
.filter('mhs', function() {
  return function(hs) {
    if(hs<1000){
      return hs+' M';
    }
    hs/=1000;
    return (hs<1000)?(hs).toPrecision(4)+' G':(hs/1000).toPrecision(4)+' T';
  }
})
.filter('duration', function() {
  return function(s) {
    if(!s) return 'loading';
    var d=Math.floor(s%60)+'s';
    if(s < 60){return d;} s/=60; d=Math.floor(s%60)+'m '+d;
    if(s < 60){return d;} s/=60; d=Math.floor(s%60)+'h '+d;
    if(s < 24){return d;} s/=24; d=Math.floor(s%24)+'d '+d;
    return d;
  }
});
