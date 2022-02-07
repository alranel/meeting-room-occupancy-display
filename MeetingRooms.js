// This room is just the room you want to use for testing purposes while 
// you're in Google Scripts. This setting will be ignored after you deploy
// the script, because the Arduino device will pass the requested room 
// when making the HTTP call. This way this script stays generic and can
// serve information related to multiple rooms.
var defaultRoom = '...@resource.calendar.google.com';

// This is your e-mail domain. It will be used to detect people from your
// organization and show their names as part as the meeting info.
var domain = 'yourdomain.com';

// This is your timezone offset. Example: CET=1, CEST=2
var timezone_nodst = 1;
var timezone_dst = 2;

Date.prototype.stdTimezoneOffset = function () {
    var jan = new Date(this.getFullYear(), 0, 1);
    var jul = new Date(this.getFullYear(), 6, 1);
    return Math.max(jan.getTimezoneOffset(), jul.getTimezoneOffset());
}
Date.prototype.isDstObserved = function () {
    return this.getTimezoneOffset() < this.stdTimezoneOffset();
}
Date.prototype.addHours = function(h) {
  this.setTime(this.getTime() + (h*60*60*1000));
  return this;
}

function doGet(req){
  if (!req) {
    req = { parameter: { room: defaultRoom } };
  }
  Logger.log(req.parameter['room']);
  var Cal = CalendarApp.getCalendarById(req.parameter['room']);
  if (!Cal) {
    return ContentService.createTextOutput("room not found");
  }
  var Now = new Date();
  var Later = new Date();
  Later.setHours(23);
  Later.setMinutes(59);
  Later.setSeconds(59);
  Logger.log(Now);
  Logger.log(Later);
  var events = Cal.getEvents(Now, Later);
  str = "";
  for (var i = 0; i < events.length; i++) {
    // adjust timezone for the display time
    var startTime = events[i].getStartTime();
    var endTime = events[i].getEndTime();
    if (startTime.isDstObserved()) {
      startTime.addHours(timezone_dst);
      endTime.addHours(timezone_dst);
    } else {
      startTime.addHours(timezone_nodst);
      endTime.addHours(timezone_nodst);
    }

    str += events[i].getStartTime().getTime()/1000
      + "|"
      + events[i].getEndTime().getTime()/1000
      + "|";
    str += startTime.toISOString().substring(11, 16)
      + "-"
      + endTime.toISOString().substring(11, 16)
      + "|";
    str += events[i].getGuestList(true)
      .filter(function (g) {return g.getEmail().endsWith('@' + domain); })
      .map(function (g) {return g.getEmail().replace('@' + domain, ''); })
      .join(", ");
    str += '\n';
  }
  Logger.log(str);
  return ContentService.createTextOutput(str);
}
