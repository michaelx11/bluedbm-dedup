var fs = require('fs');
var exec = require('child_process').exec;

exec('mkdir -p data');

// 100 mb
var SIZE_LIMIT = 4294967296 * 100;

// returns array of log entries {filename: "", index: ""}
function getLogEntries() {
  var logFile = fs.readFileSync('logfile.txt', {encoding: "utf-8"});
  var entries = logFile.trim().split("\n");
  var log = [];
  for (var i in entries) {
    var entry = entries[i].split(" -%%- ");
    var logEntry = {filename: entry[0], index: i};
    log.push(logEntry);
  }
  return log;
}

// returns index
function appendToLogFile(filename, filesize) {
  fs.appendFileSync("logfile.txt", filename + " -%%- " + filesize + "\n");
  return getLogEntries().length - 1;
}

// synchronous
exports.uploadFile = function(filedata) {
  if (!filedata) {
    return "Error!";
  }
  var filename = filedata.originalFilename;
  var path = filedata.path;
  var numBytes = filedata.size;

  if (!filename || !path) {
    return "Error!";
  }

  if (numBytes > SIZE_LIMIT) {
    return "Error!";
  }

  var entryIndex = appendToLogFile(filename, numBytes);
  exec('cp ' + path + ' data/' + entryIndex);
  console.log('uploaded: ' + filename + ' index: ' + entryIndex);
  return "index: " + entryIndex + ", url: store.haus/" + entryIndex;
}

function serveFile(id, filename, res) {
  res.setHeader('Content-disposition', 'attachment; filename=' + filename);
  var fileStream = fs.createReadStream('data/' + id);
  fileStream.pipe(res);
}

exports.getFile = function(req, res) {
  var id = req.params.id;
  if (!id) {
    res.status(404).send('File not found!');
    return;
  }

  var log = getLogEntries();
  if (id == 'head' | id == 'current') {
    if (log.length > 0) {
      id = log.length - 1;
    } else {
      id = -1;
    }
  }

  if (id >= 0 && id < log.length) {
    if (fs.existsSync('data/' + id)) {
      console.log('Served: ' + log[id].filename + ' at id: ' + id);
      serveFile(id, log[id].filename, res);
      return;
    } else {
      res.status(404).send('File too old!');
      return;
    }
  }
  res.status(404).send('File not found!');
}

exports.getList = function() {
  var logEntries = getLogEntries().reverse();
  var resultString = '';
  for (var i in logEntries) {
    var entry = logEntries[i];
    var index = logEntries.length - 1 - i;
    resultString += index + ': ' + entry.filename + '\n';
  }
  return resultString;
}

