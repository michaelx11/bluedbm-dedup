var express = require('express');
var http = require('express');
var hbs = require('hbs');
var model = require('./model');

var app = express();

app.set('port', 8090);
app.set('views', __dirname + '/views');
app.set('view engine', 'html');
app.engine('html', require('hbs').__express);

app.use(express.cookieParser());
app.use(express.logger('dev'));
app.use(express.bodyParser({limit: '10gb'}));
app.use(express.methodOverride());
app.use(express.static(__dirname + '/public'));
app.use(express.session({ secret: 'SECRET' }));

app.use(app.router);

if ('development' == app.get('env')) {
  app.use(express.errorHandler());
}

app.get('/', function(req, res) {
  res.render('index.html');
});

app.post('/upload', function(req, res) {
  model.uploadFile(req.files.filedata, function(error, data) {
    if (error) {
      console.log("error: " + error);
    }
    res.end();
//    res.send(data);
  });
});

app.get('/list', function(req, res) {
  res.setHeader('Content-Type: text/plain'), 
  res.write(model.getList(req, res));
  res.end();
});

app.get('/download', function(req, res) {
  model.getFile(req, res);
});
/*
app.get('/:id', function(req, res) {
  model.getFile(req, res);
});
*/


app.listen(app.get('port'), function() {
  console.log('Express server listening on port ' + app.get('port'));
});
