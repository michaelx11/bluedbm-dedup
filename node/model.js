var fs = require('fs');
var http = require('http');
var exec = require('child_process').exec;

var BACKEND_COMMAND = "../backends/cpp/main";

exec('mkdir -p data');

// 500 mb
var SIZE_LIMIT = 4294967296 * 500;

// Index from filename -> file representation (id list, size)
var index = {};

function addFileToIndex(filename, fileRep) {
  index[filename] = fileRep;
}

function uploadBatchHelper(listData, cbError) {
  if (listData.length == 0) {
    cbError(false);
    return;
  }

  var reqDetails = listData.pop();
  var filename = reqDetails.filename;
  var path = reqDetails.path;
  var sizeBytes = reqDetails.sizeBytes;
  backendUpload(filename, path, sizeBytes, function(error, data) {
    if (error) {
      console.log("Error while executing upload batch, num left: " + listData.length);
      cbError(error);
      return;
    }

    uploadBatchHelper(listData, cbError);
  });
}

exports.uploadBatch = function(directory, cbError) {
  var files = fs.readdirSync(directory);
  var reqDetails = [];
  for (var i = 0; i < files.length; i++) {
    var dets = {filename: files[i], path: directory + "/" + files[i], sizeBytes: 0};
    reqDetails.push(dets);
  }
  uploadBatchHelper(reqDetails, cbError);
}

function backendUpload(filename, path, sizeBytes, cbErrorData) {
  console.log('backend upload call!');
  var options = {
    host: 'localhost',
    path: '/store?filename=' + filename + '&path=' + path + '&size=' + sizeBytes,
    port: 11000
  };

  var cbCalled = false;


  var req = http.get(options, function(res) {
    console.log('STATUS: ' + res.statusCode);
    console.log('HEADERS: ' + JSON.stringify(res.headers));

    // Buffer the body entirely for processing as a whole.
    var bodyChunks = [];
    res.on('data', function(chunk) {
      // You can process streamed parts here...
      bodyChunks.push(chunk);
    }).on('end', function() {
      var body = Buffer.concat(bodyChunks);
      console.log('BODY: ' + body);
      // ...and/or process the entire body here.
      if (!cbCalled) {
        cbErrorData(false, body);
        cbCalled = true;
      }
    })
  });
  req.on('error', function(e) {
    console.log('ERROR: ' + e.message);
    if (!cbCalled) {
      cbErrorData(true, false);
      cbCalled = true;
    }
  });
}

function backendDownload(filename, dstpath, cbErrorData) {
  console.log('backend download call!');
  var options = {
    host: 'localhost',
    path: '/load?filename=' + filename + '&path=' + dstpath,
    port: 11000
  };

  var cbCalled = false;


  var req = http.get(options, function(res) {
    console.log('STATUS: ' + res.statusCode);
    console.log('HEADERS: ' + JSON.stringify(res.headers));

    // Buffer the body entirely for processing as a whole.
    var bodyChunks = [];
    res.on('data', function(chunk) {
      // You can process streamed parts here...
      bodyChunks.push(chunk);
    }).on('end', function() {
      var body = Buffer.concat(bodyChunks);
      console.log('BODY: ' + body);
      // ...and/or process the entire body here.
      if (!cbCalled) {
        cbErrorData(false, body);
        cbCalled = true;
      }
    })
  });
  req.on('error', function(e) {
    console.log('ERROR: ' + e.message);
    if (!cbCalled) {
      cbErrorData(true, false);
      cbCalled = true;
    }
  });
}

exports.uploadFile = function uploadFile(filedata, cbErrorData) {
  console.log("uploading file\n");
  if (!filedata) {
    return "Error!";
  }
  var filename = filedata.originalFilename;
  var path = filedata.path;
  var numBytes = filedata.size;
  console.log(filename + " " + path + " " + numBytes);

  if (!filename || !path || !numBytes) {
    cbErrorData("Missing filename or path or size!", false);
    return;
  }

  console.log(path);
  backendUpload(filename, path, numBytes, cbErrorData);
}

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

/*
// returns index
function appendToLogFile(filename, filesize) {
  fs.appendFileSync("logfile.txt", filename + " -%%- " + filesize + "\n");
  return getLogEntries().length - 1;
}
*/

function serveFile(filename, path, res) {
  res.setHeader('Content-disposition', 'attachment; filename=' + filename);
  var fileStream = fs.createReadStream(path);
  fileStream.pipe(res);
}

exports.getFile = function(req, res) {
  var filename = req.query.filename;
  if (!filename) {
    res.status(404).send('File not found!');
    return;
  }

  var path = "/tmp/" + filename;

  backendDownload(filename, path, function(error, data) {
    if (error) {
      res.status(404).send(error);
    } else {
      if (fs.existsSync(path)) {
        console.log('Served: ' + filename + ' from path: ' + path);
        serveFile(filename, path, res);
      }
    }
  });
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

